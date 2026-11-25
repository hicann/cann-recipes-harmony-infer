/*
 * Copyright (c) 2026 Tang Risheng
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "kernel_operator.h"

constexpr uint32_t MAX_BROADCAST_RANK = 4;
constexpr int32_t BUFFER_NUM = 2;
constexpr float NEG_ONE = -1.0f;
constexpr uint32_t VECTOR_BYTES = 256;
constexpr uint32_t MASK_BUFFER_ALIGNMENT = 32;

// ======================== 特殊值处理开关 ========================
// LOGADDEXP_STRICT_SPECIAL：是否对 ±Inf/NaN 做位级防御性修正。
//   0（默认）：不处理。有限值使用下述 FP16 多项式近似；
//              输入含 ±Inf/NaN 时行为未定义（dav-l310 向量单元会
//              饱和到 ±65504 或产生垃圾值），调用方需保证输入有限。
//   1：每个 tile 追加 9 组 CompareScalar+Select 位级修正，获得与
//      numpy 完全一致的 ±Inf/NaN 语义。
// 注：此开关必须直接改本文件后重新安装算子包。
#ifndef LOGADDEXP_STRICT_SPECIAL
#define LOGADDEXP_STRICT_SPECIAL 0
#endif


#if LOGADDEXP_STRICT_SPECIAL
template<typename T, typename U>
__aicore__ inline T BitCastScalar(U bits)
{
    static_assert(sizeof(T) == sizeof(U), "BitCastScalar requires equal sizes");
    T value;
    const uint8_t *src = reinterpret_cast<const uint8_t *>(&bits);
    uint8_t *dst = reinterpret_cast<uint8_t *>(&value);
    for (uint32_t i = 0; i < sizeof(T); ++i) {
        dst[i] = src[i];
    }
    return value;
}

template<typename T>
struct SpecialValueEncoding;

template<>
struct SpecialValueEncoding<half> {
    using BitsType = int16_t;
    static constexpr int16_t POS_INF_BITS = 0x7C00;   // 31744
    static constexpr int16_t NEG_INF_BITS = -1024;    // 0xFC00

    static __aicore__ inline half PosInf()
    {
        return BitCastScalar<half>(static_cast<uint16_t>(0x7C00));
    }

    static __aicore__ inline half NegInf()
    {
        return BitCastScalar<half>(static_cast<uint16_t>(0xFC00));
    }

    static __aicore__ inline half QuietNan()
    {
        return BitCastScalar<half>(static_cast<uint16_t>(0x7E00));
    }
};
#endif  // LOGADDEXP_STRICT_SPECIAL

template<typename TYPE_A, typename TYPE_B, typename TYPE_C>
class KernelLogAddExp {
public:
    __aicore__ inline KernelLogAddExp() {}

    __aicore__ inline void Init(
        GM_ADDR A,
        GM_ADDR B,
        GM_ADDR C,
        uint32_t inputALength,
        uint32_t inputBLength,
        uint32_t totalLength,
        uint32_t tileLength,
        uint32_t rank,
        uint32_t rowLength,
        uint32_t tilesPerRow,
        uint32_t tasksPerCore,
        uint32_t taskRemain,
        uint32_t needsBroadcast,
        const uint32_t *outputDims,
        const uint32_t *inputAStrides,
        const uint32_t *inputBStrides)
    {
        ASSERT(AscendC::GetBlockNum() != 0 && "block dim can not be zero!");

        this->totalLength = totalLength;
        this->tileLength = tileLength;
        this->rank = rank;
        this->rowLength = rowLength;
        this->tilesPerRow = tilesPerRow;
        this->needsBroadcast = needsBroadcast;
        for (uint32_t i = 0; i < MAX_BROADCAST_RANK; ++i) {
            this->outputDims[i] = outputDims[i];
            this->inputAStrides[i] = inputAStrides[i];
            this->inputBStrides[i] = inputBStrides[i];
        }

        const uint32_t blockIdx = AscendC::GetBlockIdx();
        this->coreTaskCount = tasksPerCore + (blockIdx < taskRemain ? 1 : 0);
        this->coreTaskStart = blockIdx * tasksPerCore + (blockIdx < taskRemain ? blockIdx : taskRemain);

        Gm_A.SetGlobalBuffer((__gm__ TYPE_A*)A, inputALength);
        Gm_B.SetGlobalBuffer((__gm__ TYPE_B*)B, inputBLength);
        Gm_C.SetGlobalBuffer((__gm__ TYPE_C*)C, totalLength);

        pipe.InitBuffer(Q_A, BUFFER_NUM, this->tileLength * sizeof(TYPE_A));
        pipe.InitBuffer(Q_B, BUFFER_NUM, this->tileLength * sizeof(TYPE_B));
        pipe.InitBuffer(Q_C, BUFFER_NUM, this->tileLength * sizeof(TYPE_C));
        pipe.InitBuffer(tmp1, this->tileLength * sizeof(TYPE_C));
        pipe.InitBuffer(tmp2, this->tileLength * sizeof(TYPE_C));
        pipe.InitBuffer(tmp3, this->tileLength * sizeof(TYPE_C));
#if LOGADDEXP_STRICT_SPECIAL
        const uint32_t maskBytes = (this->tileLength + 7U) / 8U;
        const uint32_t alignedMaskBytes =
            (maskBytes + MASK_BUFFER_ALIGNMENT - 1U) / MASK_BUFFER_ALIGNMENT * MASK_BUFFER_ALIGNMENT;
        pipe.InitBuffer(compareMask, alignedMaskBytes);
#endif

        this->scalarToVectorEventId = static_cast<int32_t>(
            GetTPipePtr()->FetchEventID(AscendC::HardEvent::S_V));
    }

    __aicore__ inline void Process()
    {
        for (uint32_t localTask = 0; localTask < this->coreTaskCount; ++localTask) {
            uint32_t inputAOffset = 0;
            uint32_t inputBOffset = 0;
            uint32_t outputOffset = 0;
            uint32_t length = 0;
            GetTaskInfo(
                this->coreTaskStart + localTask,
                inputAOffset,
                inputBOffset,
                outputOffset,
                length);
            CopyIn(inputAOffset, inputBOffset, length);
            Compute(length);
            CopyOut(outputOffset, length);
        }
    }

private:
    __aicore__ inline void GetTaskInfo(
        uint32_t taskIndex,
        uint32_t &inputAOffset,
        uint32_t &inputBOffset,
        uint32_t &outputOffset,
        uint32_t &length) const
    {
        if (this->needsBroadcast == 0) {
            outputOffset = taskIndex * this->tileLength;
            inputAOffset = outputOffset;
            inputBOffset = outputOffset;
            const uint32_t remain = this->totalLength - outputOffset;
            length = remain < this->tileLength ? remain : this->tileLength;
            return;
        }

        const uint32_t rowIndex = taskIndex / this->tilesPerRow;
        const uint32_t tileIndexInRow = taskIndex % this->tilesPerRow;
        const uint32_t columnOffset = tileIndexInRow * this->tileLength;
        const uint32_t remainInRow = this->rowLength - columnOffset;
        length = remainInRow < this->tileLength ? remainInRow : this->tileLength;
        outputOffset = rowIndex * this->rowLength + columnOffset;

        uint32_t remainingRowIndex = rowIndex;
        for (int32_t dim = static_cast<int32_t>(this->rank) - 2; dim >= 0; --dim) {
            const uint32_t coordinate = remainingRowIndex % this->outputDims[dim];
            remainingRowIndex /= this->outputDims[dim];
            inputAOffset += coordinate * this->inputAStrides[dim];
            inputBOffset += coordinate * this->inputBStrides[dim];
        }
        inputAOffset += columnOffset * this->inputAStrides[this->rank - 1];
        inputBOffset += columnOffset * this->inputBStrides[this->rank - 1];
    }

    __aicore__ inline void CopyIn(
        uint32_t inputAOffset,
        uint32_t inputBOffset,
        uint32_t length)
    {
        AscendC::LocalTensor<TYPE_A> inputA = Q_A.AllocTensor<TYPE_A>();
        AscendC::LocalTensor<TYPE_B> inputB = Q_B.AllocTensor<TYPE_B>();

        const uint32_t inputACount =
            (this->needsBroadcast != 0 && this->inputAStrides[this->rank - 1] == 0) ? 1 : length;
        const uint32_t inputBCount =
            (this->needsBroadcast != 0 && this->inputBStrides[this->rank - 1] == 0) ? 1 : length;
        AscendC::DataCopyExtParams inputACopyParams{
            1, static_cast<uint32_t>(inputACount * sizeof(TYPE_A)), 0, 0, 0};
        AscendC::DataCopyExtParams inputBCopyParams{
            1, static_cast<uint32_t>(inputBCount * sizeof(TYPE_B)), 0, 0, 0};
        AscendC::DataCopyPadExtParams<TYPE_A> inputAPadParams{false, 0, 0, static_cast<TYPE_A>(0)};
        AscendC::DataCopyPadExtParams<TYPE_B> inputBPadParams{false, 0, 0, static_cast<TYPE_B>(0)};
        AscendC::DataCopyPad(inputA, Gm_A[inputAOffset], inputACopyParams, inputAPadParams);
        AscendC::DataCopyPad(inputB, Gm_B[inputBOffset], inputBCopyParams, inputBPadParams);

        Q_A.EnQue(inputA);
        Q_B.EnQue(inputB);
    }

    __aicore__ inline void Compute(uint32_t length)
    {
        AscendC::LocalTensor<TYPE_A> inputA = Q_A.DeQue<TYPE_A>();
        AscendC::LocalTensor<TYPE_B> inputB = Q_B.DeQue<TYPE_B>();
        AscendC::LocalTensor<TYPE_C> output = Q_C.AllocTensor<TYPE_C>();

        const bool expandA =
            this->needsBroadcast != 0 && this->inputAStrides[this->rank - 1] == 0;
        const bool expandB =
            this->needsBroadcast != 0 && this->inputBStrides[this->rank - 1] == 0;
        if (expandA || expandB) {
            AscendC::PipeBarrier<PIPE_ALL>();
            TYPE_A scalarA = static_cast<TYPE_A>(0);
            TYPE_B scalarB = static_cast<TYPE_B>(0);
            if (expandA) {
                scalarA = inputA.GetValue(0);
            }
            if (expandB) {
                scalarB = inputB.GetValue(0);
            }
            AscendC::SetFlag<AscendC::HardEvent::S_V>(this->scalarToVectorEventId);
            AscendC::WaitFlag<AscendC::HardEvent::S_V>(this->scalarToVectorEventId);
            if (expandA) {
                AscendC::Duplicate(inputA, scalarA, length);
            }
            if (expandB) {
                AscendC::Duplicate(inputB, scalarB, length);
            }
            AscendC::PipeBarrier<PIPE_ALL>();
        }

        auto diffAB = tmp2.Get<TYPE_C>();
        auto absDiff = tmp3.Get<TYPE_C>();

        AscendC::Sub(diffAB, inputA, inputB, length);
        AscendC::Abs(absDiff, diffAB, length);
        AscendC::Muls(absDiff, absDiff, static_cast<TYPE_C>(NEG_ONE), length);
        AscendC::Exp(absDiff, absDiff, length);
        ComputeFiniteResult(output, inputA, inputB, length);

#if LOGADDEXP_STRICT_SPECIAL
        ApplySpecialValues(output, inputA, inputB, length);
#endif

        Q_A.FreeTensor(inputA);
        Q_B.FreeTensor(inputB);
        Q_C.EnQue<TYPE_C>(output);
    }

    __aicore__ inline void ComputeFiniteResult(
        const AscendC::LocalTensor<TYPE_C> &output,
        const AscendC::LocalTensor<TYPE_A> &inputA,
        const AscendC::LocalTensor<TYPE_B> &inputB,
        uint32_t length)
    {
        auto polynomial = tmp1.Get<TYPE_C>();
        auto residual = tmp2.Get<TYPE_C>();
        auto t = tmp3.Get<TYPE_C>();  // exp(-abs(A-B)), in [0,1].

        // log1p(t) ~= t + t^2 P(t), with an FP16 polynomial on [0,1].
        // Keep t and its small residual separate until adding max(A,B).
        // This avoids both 1+t rounding to 1 and premature cancellation.
        AscendC::Muls(polynomial, t, static_cast<TYPE_C>(0.0194854736328125f), length);
        AscendC::Adds(polynomial, polynomial, static_cast<TYPE_C>(-0.08135986328125f), length);
        AscendC::Mul(polynomial, polynomial, t, length);
        AscendC::Adds(polynomial, polynomial, static_cast<TYPE_C>(0.164794921875f), length);
        AscendC::Mul(polynomial, polynomial, t, length);
        AscendC::Adds(polynomial, polynomial, static_cast<TYPE_C>(-0.2425537109375f), length);
        AscendC::Mul(polynomial, polynomial, t, length);
        AscendC::Adds(polynomial, polynomial, static_cast<TYPE_C>(0.332763671875f), length);
        AscendC::Mul(polynomial, polynomial, t, length);
        AscendC::Adds(polynomial, polynomial, static_cast<TYPE_C>(-0.5f), length);
        AscendC::Mul(residual, t, t, length);
        AscendC::Mul(residual, residual, polynomial, length);
        AscendC::Max(output, inputA, inputB, length);
        AscendC::Add(output, output, t, length);
        AscendC::Add(output, output, residual, length);
    }

    // 在 dst 中把 input 位视图满足 (bits <mode> scalar) 的位置替换为 srcWhenTrue，
    // 其余位置取 srcWhenFalse。链式调用可实现 mask 合取，连续调用同常量可实现析取。
#if LOGADDEXP_STRICT_SPECIAL
    template<typename TIn>
    __aicore__ inline void SelectWhereBitsCmp(
        const AscendC::LocalTensor<TYPE_C> &dst,
        const AscendC::LocalTensor<TIn> &input,
        typename SpecialValueEncoding<TIn>::BitsType scalar,
        AscendC::CMPMODE mode,
        const AscendC::LocalTensor<TYPE_C> &srcWhenTrue,
        const AscendC::LocalTensor<TYPE_C> &srcWhenFalse,
        uint32_t alignedLength)
    {
        auto mask = compareMask.Get<uint8_t>();
        auto bitsView =
            input.template ReinterpretCast<typename SpecialValueEncoding<TIn>::BitsType>();
        AscendC::CompareScalar(mask, bitsView, scalar, mode, alignedLength);
        AscendC::PipeBarrier<PIPE_ALL>();
        AscendC::Select(
            dst,
            mask,
            srcWhenTrue,
            srcWhenFalse,
            AscendC::SELMODE::VSEL_TENSOR_TENSOR_MODE,
            alignedLength);
        AscendC::PipeBarrier<PIPE_ALL>();
    }

    __aicore__ inline void ApplySpecialValues(
        const AscendC::LocalTensor<TYPE_C> &output,
        const AscendC::LocalTensor<TYPE_A> &inputA,
        const AscendC::LocalTensor<TYPE_B> &inputB,
        uint32_t length)
    {
        using EncA = SpecialValueEncoding<TYPE_A>;
        using EncB = SpecialValueEncoding<TYPE_B>;
        using EncC = SpecialValueEncoding<TYPE_C>;
        const uint32_t compareAlignElements = VECTOR_BYTES / sizeof(TYPE_C);
        const uint32_t alignedLength =
            (length + compareAlignElements - 1U) / compareAlignElements * compareAlignElements;
        auto constBuf = tmp1.Get<TYPE_C>();
        auto chainBuf = tmp2.Get<TYPE_C>();

        // 任一输入为 +Inf（且非 NaN）时输出 +Inf；NaN 在最后覆盖，优先级最高。
        AscendC::Duplicate(constBuf, EncC::PosInf(), alignedLength);
        AscendC::PipeBarrier<PIPE_ALL>();
        SelectWhereBitsCmp(
            output, inputA, EncA::POS_INF_BITS, AscendC::CMPMODE::EQ, constBuf, output, alignedLength);
        SelectWhereBitsCmp(
            output, inputB, EncB::POS_INF_BITS, AscendC::CMPMODE::EQ, constBuf, output, alignedLength);

        // 仅当两个输入都为 -Inf 时输出 -Inf：链式 Select 实现 (A==-Inf) && (B==-Inf)。
        AscendC::Duplicate(constBuf, EncC::NegInf(), alignedLength);
        AscendC::PipeBarrier<PIPE_ALL>();
        SelectWhereBitsCmp(
            chainBuf, inputA, EncA::NEG_INF_BITS, AscendC::CMPMODE::EQ, constBuf, output, alignedLength);
        SelectWhereBitsCmp(
            output, inputB, EncB::NEG_INF_BITS, AscendC::CMPMODE::EQ, chainBuf, output, alignedLength);

        // NaN 传播：正 NaN 位视图 > POS_INF_BITS；
        // 负 NaN 位视图落在 (NEG_INF_BITS, 0) 开区间，用链式 Select 实现合取。
        AscendC::Duplicate(constBuf, EncC::QuietNan(), alignedLength);
        AscendC::PipeBarrier<PIPE_ALL>();
        SelectWhereBitsCmp(
            output, inputA, EncA::POS_INF_BITS, AscendC::CMPMODE::GT, constBuf, output, alignedLength);
        SelectWhereBitsCmp(
            chainBuf, inputA, EncA::NEG_INF_BITS, AscendC::CMPMODE::GT, constBuf, output, alignedLength);
        SelectWhereBitsCmp(
            output, inputA, 0, AscendC::CMPMODE::LT, chainBuf, output, alignedLength);
        SelectWhereBitsCmp(
            output, inputB, EncB::POS_INF_BITS, AscendC::CMPMODE::GT, constBuf, output, alignedLength);
        SelectWhereBitsCmp(
            chainBuf, inputB, EncB::NEG_INF_BITS, AscendC::CMPMODE::GT, constBuf, output, alignedLength);
        SelectWhereBitsCmp(
            output, inputB, 0, AscendC::CMPMODE::LT, chainBuf, output, alignedLength);
    }
#endif  // LOGADDEXP_STRICT_SPECIAL

    __aicore__ inline void CopyOut(uint32_t outputOffset, uint32_t length)
    {
        AscendC::LocalTensor<TYPE_C> output = Q_C.DeQue<TYPE_C>();
        AscendC::DataCopyExtParams outputCopyParams{
            1, static_cast<uint32_t>(length * sizeof(TYPE_C)), 0, 0, 0};
        AscendC::DataCopyPad(Gm_C[outputOffset], output, outputCopyParams);
        Q_C.FreeTensor(output);
    }

private:
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, BUFFER_NUM> Q_A;
    AscendC::TQue<AscendC::QuePosition::VECIN, BUFFER_NUM> Q_B;
    AscendC::TQue<AscendC::QuePosition::VECOUT, BUFFER_NUM> Q_C;
    AscendC::TBuf<AscendC::QuePosition::VECCALC> tmp1;
    AscendC::TBuf<AscendC::QuePosition::VECCALC> tmp2;
    AscendC::TBuf<AscendC::QuePosition::VECCALC> tmp3;
#if LOGADDEXP_STRICT_SPECIAL
    AscendC::TBuf<AscendC::QuePosition::VECCALC> compareMask;
#endif
    AscendC::GlobalTensor<TYPE_A> Gm_A;
    AscendC::GlobalTensor<TYPE_B> Gm_B;
    AscendC::GlobalTensor<TYPE_C> Gm_C;

    uint32_t totalLength;
    uint32_t tileLength;
    uint32_t rank;
    uint32_t rowLength;
    uint32_t tilesPerRow;
    uint32_t needsBroadcast;
    uint32_t coreTaskStart;
    uint32_t coreTaskCount;
    uint32_t outputDims[MAX_BROADCAST_RANK];
    uint32_t inputAStrides[MAX_BROADCAST_RANK];
    uint32_t inputBStrides[MAX_BROADCAST_RANK];
    int32_t scalarToVectorEventId;
};

extern "C" __global__ __aicore__ void log_add_exp(
    GM_ADDR A,
    GM_ADDR B,
    GM_ADDR C,
    GM_ADDR workspace,
    GM_ADDR tiling)
{
    GET_TILING_DATA(tiling_data, tiling);
    if (tiling_data.total_length == 0 || tiling_data.task_count == 0) {
        return;
    }

    KernelLogAddExp<DTYPE_A, DTYPE_B, DTYPE_C> op;
    op.Init(
        A,
        B,
        C,
        tiling_data.input_a_length,
        tiling_data.input_b_length,
        tiling_data.total_length,
        tiling_data.tile_length,
        tiling_data.rank,
        tiling_data.row_length,
        tiling_data.tiles_per_row,
        tiling_data.tasks_per_core,
        tiling_data.task_remain,
        tiling_data.needs_broadcast,
        tiling_data.output_dims,
        tiling_data.input_a_strides,
        tiling_data.input_b_strides);
    op.Process();
}
