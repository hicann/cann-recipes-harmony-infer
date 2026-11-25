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

#ifndef OP_HOST_LOG_ADD_EXP_BROADCAST_H
#define OP_HOST_LOG_ADD_EXP_BROADCAST_H

#include <cstddef>
#include <cstdint>
#include <limits>

namespace log_add_exp {

constexpr size_t MAX_BROADCAST_RANK = 4;

enum class BroadcastStatus {
    SUCCESS = 0,
    RANK_TOO_LARGE,
    INVALID_DIMENSION,
    ZERO_ELEMENT_TENSOR,
    INCOMPATIBLE_SHAPE,
    SIZE_OVERFLOW,
};

struct BroadcastInfo {
    uint32_t logicalRank;
    uint32_t kernelRank;
    uint32_t inputALength;
    uint32_t inputBLength;
    uint32_t totalLength;
    uint32_t rowLength;
    uint32_t rowCount;
    uint32_t needsBroadcast;
    uint32_t inputADims[MAX_BROADCAST_RANK];
    uint32_t inputBDims[MAX_BROADCAST_RANK];
    uint32_t outputDims[MAX_BROADCAST_RANK];
    uint32_t inputAStrides[MAX_BROADCAST_RANK];
    uint32_t inputBStrides[MAX_BROADCAST_RANK];
    uint32_t outputStrides[MAX_BROADCAST_RANK];
};

inline void ResetBroadcastInfo(BroadcastInfo &info)
{
    info.logicalRank = 0;
    info.kernelRank = 1;
    info.inputALength = 1;
    info.inputBLength = 1;
    info.totalLength = 1;
    info.rowLength = 1;
    info.rowCount = 1;
    info.needsBroadcast = 0;
    for (size_t i = 0; i < MAX_BROADCAST_RANK; ++i) {
        info.inputADims[i] = 1;
        info.inputBDims[i] = 1;
        info.outputDims[i] = 1;
        info.inputAStrides[i] = 0;
        info.inputBStrides[i] = 0;
        info.outputStrides[i] = 0;
    }
}

inline BroadcastStatus BuildContiguousStrides(
    const uint32_t dims[MAX_BROADCAST_RANK],
    uint32_t rank,
    uint32_t strides[MAX_BROADCAST_RANK],
    uint32_t &length)
{
    uint64_t running = 1;
    for (int32_t i = static_cast<int32_t>(rank) - 1; i >= 0; --i) {
        if (running > std::numeric_limits<uint32_t>::max()) {
            return BroadcastStatus::SIZE_OVERFLOW;
        }
        strides[i] = static_cast<uint32_t>(running);
        if (dims[i] == 0) {
            return BroadcastStatus::ZERO_ELEMENT_TENSOR;
        }
        running *= dims[i];
        if (running > std::numeric_limits<uint32_t>::max()) {
            return BroadcastStatus::SIZE_OVERFLOW;
        }
    }
    length = static_cast<uint32_t>(running);
    return BroadcastStatus::SUCCESS;
}

inline BroadcastStatus ValidateInputDims(
    const int64_t *dims,
    size_t rank,
    size_t slotOffset,
    uint32_t outDims[MAX_BROADCAST_RANK])
{
    for (size_t i = 0; i < rank; ++i) {
        const int64_t dim = dims[i];
        if (dim < 0) {
            return BroadcastStatus::INVALID_DIMENSION;
        }
        if (static_cast<uint64_t>(dim) > std::numeric_limits<uint32_t>::max()) {
            return BroadcastStatus::SIZE_OVERFLOW;
        }
        if (dim == 0) {
            return BroadcastStatus::ZERO_ELEMENT_TENSOR;
        }
        outDims[slotOffset + i] = static_cast<uint32_t>(dim);
    }
    return BroadcastStatus::SUCCESS;
}

inline BroadcastStatus MergeOutputDims(BroadcastInfo &info)
{
    for (uint32_t i = 0; i < info.kernelRank; ++i) {
        const uint32_t dimA = info.inputADims[i];
        const uint32_t dimB = info.inputBDims[i];
        if (dimA == dimB) {
            info.outputDims[i] = dimA;
        } else if (dimA == 1) {
            info.outputDims[i] = dimB;
        } else if (dimB == 1) {
            info.outputDims[i] = dimA;
        } else {
            return BroadcastStatus::INCOMPATIBLE_SHAPE;
        }
    }
    return BroadcastStatus::SUCCESS;
}

inline BroadcastStatus BuildAllStrides(
    BroadcastInfo &info, uint32_t *inputARawStrides, uint32_t *inputBRawStrides)
{
    BroadcastStatus status = BuildContiguousStrides(
        info.inputADims, info.kernelRank, inputARawStrides, info.inputALength);
    if (status != BroadcastStatus::SUCCESS) {
        return status;
    }
    status = BuildContiguousStrides(
        info.inputBDims, info.kernelRank, inputBRawStrides, info.inputBLength);
    if (status != BroadcastStatus::SUCCESS) {
        return status;
    }
    return BuildContiguousStrides(
        info.outputDims, info.kernelRank, info.outputStrides, info.totalLength);
}

inline void ApplyBroadcastStrides(
    BroadcastInfo &info, const uint32_t *inputARawStrides, const uint32_t *inputBRawStrides)
{
    for (uint32_t i = 0; i < info.kernelRank; ++i) {
        info.inputAStrides[i] =
            (info.inputADims[i] == 1 && info.outputDims[i] != 1) ? 0 : inputARawStrides[i];
        info.inputBStrides[i] =
            (info.inputBDims[i] == 1 && info.outputDims[i] != 1) ? 0 : inputBRawStrides[i];
    }
}

inline BroadcastStatus BuildBroadcastInfo(
    const int64_t *inputADims,
    size_t inputARank,
    const int64_t *inputBDims,
    size_t inputBRank,
    BroadcastInfo &info)
{
    ResetBroadcastInfo(info);
    if (inputARank > MAX_BROADCAST_RANK || inputBRank > MAX_BROADCAST_RANK) {
        return BroadcastStatus::RANK_TOO_LARGE;
    }

    info.logicalRank = static_cast<uint32_t>(inputARank > inputBRank ? inputARank : inputBRank);
    info.kernelRank = info.logicalRank == 0 ? 1 : info.logicalRank;

    BroadcastStatus status = ValidateInputDims(inputADims, inputARank,
        info.kernelRank - inputARank, info.inputADims);
    if (status != BroadcastStatus::SUCCESS) {
        return status;
    }
    status = ValidateInputDims(inputBDims, inputBRank,
        info.kernelRank - inputBRank, info.inputBDims);
    if (status != BroadcastStatus::SUCCESS) {
        return status;
    }
    status = MergeOutputDims(info);
    if (status != BroadcastStatus::SUCCESS) {
        return status;
    }

    uint32_t inputARawStrides[MAX_BROADCAST_RANK] = {};
    uint32_t inputBRawStrides[MAX_BROADCAST_RANK] = {};
    status = BuildAllStrides(info, inputARawStrides, inputBRawStrides);
    if (status != BroadcastStatus::SUCCESS) {
        return status;
    }
    ApplyBroadcastStrides(info, inputARawStrides, inputBRawStrides);

    info.needsBroadcast =
        (info.inputALength != info.totalLength || info.inputBLength != info.totalLength) ? 1 : 0;
    info.rowLength = info.outputDims[info.kernelRank - 1];
    if (info.rowLength == 0) {
        return BroadcastStatus::ZERO_ELEMENT_TENSOR;
    }
    info.rowCount = info.totalLength / info.rowLength;
    return BroadcastStatus::SUCCESS;
}

}  // namespace log_add_exp

#endif  // OP_HOST_LOG_ADD_EXP_BROADCAST_H
