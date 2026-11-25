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

#include "log_add_exp_broadcast.h"
#include "log_add_exp_tiling.h"
#include "register/op_def_registry.h"
#include "tiling/platform/platform_ascendc.h"

namespace {

log_add_exp::BroadcastStatus BuildBroadcastInfoFromShapes(
    const gert::Shape &inputAShape,
    const gert::Shape &inputBShape,
    log_add_exp::BroadcastInfo &info)
{
    int64_t inputADims[log_add_exp::MAX_BROADCAST_RANK] = {};
    int64_t inputBDims[log_add_exp::MAX_BROADCAST_RANK] = {};
    const size_t inputARank = inputAShape.GetDimNum();
    const size_t inputBRank = inputBShape.GetDimNum();
    if (inputARank > log_add_exp::MAX_BROADCAST_RANK ||
        inputBRank > log_add_exp::MAX_BROADCAST_RANK) {
        return log_add_exp::BroadcastStatus::RANK_TOO_LARGE;
    }
    for (size_t i = 0; i < inputARank; ++i) {
        inputADims[i] = inputAShape.GetDim(i);
    }
    for (size_t i = 0; i < inputBRank; ++i) {
        inputBDims[i] = inputBShape.GetDim(i);
    }
    return log_add_exp::BuildBroadcastInfo(
        inputADims, inputARank, inputBDims, inputBRank, info);
}

}  // namespace

namespace optiling {
constexpr uint32_t BLOCK_SIZE = 32;
// Six queue buffers, three full-size calculation buffers, and one packed
// comparison-mask buffer. Ten full-size buffers is a conservative UB model.
constexpr uint32_t BUFFER_BLOCK_COUNT = 10;
constexpr uint32_t UB_USE_DIVISOR = 2;

static bool ValidateTilingContext(const gert::TilingContext* context)
{
    return context != nullptr && context->GetInputShape(0) != nullptr &&
           context->GetInputShape(1) != nullptr && context->GetInputDesc(0) != nullptr &&
           context->GetInputDesc(1) != nullptr && context->GetPlatformInfo() != nullptr;
}

static uint32_t ComputeTileLength(uint64_t ubSize)
{
    constexpr uint32_t FP16_BYTES = 2;
    const uint32_t alignNum = BLOCK_SIZE / FP16_BYTES;
    const uint32_t vectorAlignNum = alignNum * 8;
    uint64_t tileCapacity = (ubSize / UB_USE_DIVISOR) / (BUFFER_BLOCK_COUNT * FP16_BYTES);
    uint32_t tileLength =
        static_cast<uint32_t>(tileCapacity > UINT32_MAX ? UINT32_MAX : tileCapacity);
    return tileLength / vectorAlignNum * vectorAlignNum;
}

static void ComputeTaskLayout(
    const log_add_exp::BroadcastInfo &broadcastInfo,
    uint32_t tileLength,
    uint32_t availableCoreNum,
    uint32_t &tilesPerRow,
    uint32_t &taskCount,
    uint32_t &blockDim,
    uint32_t &tasksPerCore,
    uint32_t &taskRemain)
{
    tilesPerRow = 0;
    taskCount = 0;
    blockDim = 1;
    tasksPerCore = 0;
    taskRemain = 0;
    if (tileLength == 0) {
        return;
    }
    if (broadcastInfo.totalLength != 0) {
        if (broadcastInfo.needsBroadcast != 0) {
            tilesPerRow = broadcastInfo.rowLength / tileLength +
                (broadcastInfo.rowLength % tileLength == 0 ? 0 : 1);
            taskCount = broadcastInfo.rowCount * tilesPerRow;
        } else {
            taskCount = broadcastInfo.totalLength / tileLength +
                (broadcastInfo.totalLength % tileLength == 0 ? 0 : 1);
        }
    }
    blockDim = taskCount == 0 ? 1 : (availableCoreNum < taskCount ? availableCoreNum : taskCount);
    if (blockDim == 0) {
        blockDim = 1;
    }
    tasksPerCore = taskCount / blockDim;
    taskRemain = taskCount % blockDim;
}

static void FillTilingData(
    LogAddExpTilingData &tiling,
    const log_add_exp::BroadcastInfo &broadcastInfo,
    uint32_t tileLength,
    uint32_t tilesPerRow,
    uint32_t taskCount,
    uint32_t tasksPerCore,
    uint32_t taskRemain)
{
    tiling.set_tile_length(tileLength);
    tiling.set_total_length(broadcastInfo.totalLength);
    tiling.set_input_a_length(broadcastInfo.inputALength);
    tiling.set_input_b_length(broadcastInfo.inputBLength);
    tiling.set_rank(broadcastInfo.kernelRank);
    tiling.set_row_length(broadcastInfo.rowLength);
    tiling.set_tiles_per_row(tilesPerRow);
    tiling.set_task_count(taskCount);
    tiling.set_tasks_per_core(tasksPerCore);
    tiling.set_task_remain(taskRemain);
    tiling.set_needs_broadcast(broadcastInfo.needsBroadcast);
    tiling.set_output_dims(broadcastInfo.outputDims);
    tiling.set_input_a_strides(broadcastInfo.inputAStrides);
    tiling.set_input_b_strides(broadcastInfo.inputBStrides);
    tiling.set_reserved(0);
}

static ge::graphStatus SaveTilingData(
    gert::TilingContext* context, LogAddExpTilingData &tiling, uint32_t blockDim)
{
    context->SetBlockDim(blockDim);
    auto rawTilingData = context->GetRawTilingData();
    if (rawTilingData == nullptr) {
        return ge::GRAPH_FAILED;
    }
    tiling.SaveToBuffer(rawTilingData->GetData(), rawTilingData->GetCapacity());
    rawTilingData->SetDataSize(tiling.GetDataSize());

    size_t *currentWorkspace = context->GetWorkspaceSizes(1);
    if (currentWorkspace == nullptr) {
        return ge::GRAPH_FAILED;
    }
    currentWorkspace[0] = 0;
    return ge::GRAPH_SUCCESS;
}

static ge::graphStatus TilingFunc(gert::TilingContext* context)
{
    if (!ValidateTilingContext(context)) {
        return ge::GRAPH_FAILED;
    }

    const auto &inputAShape = context->GetInputShape(0)->GetStorageShape();
    const auto &inputBShape = context->GetInputShape(1)->GetStorageShape();
    log_add_exp::BroadcastInfo broadcastInfo;
    if (BuildBroadcastInfoFromShapes(inputAShape, inputBShape, broadcastInfo) !=
        log_add_exp::BroadcastStatus::SUCCESS) {
        return ge::GRAPH_FAILED;
    }

    if (context->GetInputDesc(0)->GetDataType() != ge::DT_FLOAT16 ||
        context->GetInputDesc(1)->GetDataType() != ge::DT_FLOAT16) {
        return ge::GRAPH_FAILED;
    }

    auto ascendcPlatform = platform_ascendc::PlatformAscendC(context->GetPlatformInfo());
    uint64_t ubSize = 0;
    ascendcPlatform.GetCoreMemSize(platform_ascendc::CoreMemType::UB, ubSize);
    uint32_t availableCoreNum = ascendcPlatform.GetCoreNum();
    if (availableCoreNum == 0) {
        availableCoreNum = 1;
    }

    const uint32_t tileLength = ComputeTileLength(ubSize);
    if (tileLength == 0) {
        return ge::GRAPH_FAILED;
    }

    uint32_t tilesPerRow = 0;
    uint32_t taskCount = 0;
    uint32_t blockDim = 1;
    uint32_t tasksPerCore = 0;
    uint32_t taskRemain = 0;
    ComputeTaskLayout(broadcastInfo, tileLength, availableCoreNum,
                      tilesPerRow, taskCount, blockDim, tasksPerCore, taskRemain);

    LogAddExpTilingData tiling;
    FillTilingData(tiling, broadcastInfo, tileLength, tilesPerRow, taskCount,
                   tasksPerCore, taskRemain);
    return SaveTilingData(context, tiling, blockDim);
}

}  // namespace optiling

namespace ge {
static ge::graphStatus InferShape(gert::InferShapeContext* context)
{
    if (context == nullptr) {
        return GRAPH_FAILED;
    }
    const auto input_a_shape = context->GetInputShape(0);
    const auto input_b_shape = context->GetInputShape(1);
    auto output_shape = context->GetOutputShape(0);

    if (input_a_shape == nullptr || input_b_shape == nullptr || output_shape == nullptr) {
        return GRAPH_FAILED;
    }

    log_add_exp::BroadcastInfo broadcastInfo;
    if (BuildBroadcastInfoFromShapes(*input_a_shape, *input_b_shape, broadcastInfo) !=
        log_add_exp::BroadcastStatus::SUCCESS) {
        return GRAPH_FAILED;
    }

    output_shape->SetDimNum(broadcastInfo.logicalRank);
    for (uint32_t i = 0; i < broadcastInfo.logicalRank; ++i) {
        output_shape->SetDim(i, broadcastInfo.outputDims[i]);
    }
    return GRAPH_SUCCESS;
}
static ge::graphStatus InferDataType(gert::InferDataTypeContext* context)
{
    if (context == nullptr) {
        return GRAPH_FAILED;
    }
    const ge::DataType inputAType = context->GetInputDataType(0);
    const ge::DataType inputBType = context->GetInputDataType(1);
    if (inputAType != ge::DT_FLOAT16 || inputBType != ge::DT_FLOAT16) {
        return GRAPH_FAILED;
    }
    context->SetOutputDataType(0, ge::DT_FLOAT16);
    return GRAPH_SUCCESS;
}
}  // namespace ge


namespace ops {
class LogAddExp : public OpDef {
public:
    explicit LogAddExp(const char* name) : OpDef(name)
    {
        this->Input("A")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16})
            .Format({ge::FORMAT_ND});
        this->Input("B")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16})
            .Format({ge::FORMAT_ND});
        this->Output("C")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16})
            .Format({ge::FORMAT_ND});

        this->SetInferShape(ge::InferShape);
        this->SetInferDataType(ge::InferDataType);

        this->AICore().SetTiling(optiling::TilingFunc);
        this->AICore().AddConfig("kirin9020");
    }
};

OP_ADD(LogAddExp);
}  // namespace ops
