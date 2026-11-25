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

#include "../op_host/log_add_exp_broadcast.h"

#include <stdexcept>
#include <string>

#include <cstdint>
#include <iostream>
#include <vector>

namespace {

void CheckFailed(const char *expr, const char *file, int line)
{
    throw std::runtime_error(
        std::string(file) + ":" + std::to_string(line) + ": check failed: " + expr);
}

log_add_exp::BroadcastStatus Build(
    const std::vector<int64_t> &shapeA,
    const std::vector<int64_t> &shapeB,
    log_add_exp::BroadcastInfo &info)
{
    const int64_t *dimsA = shapeA.empty() ? nullptr : shapeA.data();
    const int64_t *dimsB = shapeB.empty() ? nullptr : shapeB.data();
    return log_add_exp::BuildBroadcastInfo(
        dimsA, shapeA.size(), dimsB, shapeB.size(), info);
}

#define CHECK(cond) \
    ((cond) ? static_cast<void>(0) : CheckFailed(#cond, __FILE__, __LINE__))

void AssertDims(
    const uint32_t actual[log_add_exp::MAX_BROADCAST_RANK],
    const std::vector<uint32_t> &expected)
{
    for (size_t i = 0; i < expected.size(); ++i) {
        CHECK(actual[i] == expected[i]);
    }
}

uint32_t ReferenceInputOffset(
    const log_add_exp::BroadcastInfo &info,
    const uint32_t inputDims[log_add_exp::MAX_BROADCAST_RANK],
    uint32_t outputOffset)
{
    uint32_t rawInputStrides[log_add_exp::MAX_BROADCAST_RANK] = {};
    uint32_t ignoredLength = 0;
    CHECK(
        log_add_exp::BuildContiguousStrides(
            inputDims, info.kernelRank, rawInputStrides, ignoredLength) ==
        log_add_exp::BroadcastStatus::SUCCESS);

    uint32_t inputOffset = 0;
    uint32_t remainingOutputOffset = outputOffset;
    for (uint32_t dim = 0; dim < info.kernelRank; ++dim) {
        const uint32_t coordinate = remainingOutputOffset / info.outputStrides[dim];
        remainingOutputOffset %= info.outputStrides[dim];
        if (inputDims[dim] != 1) {
            inputOffset += coordinate * rawInputStrides[dim];
        }
    }
    return inputOffset;
}

void VerifyBroadcastTasks(
    const std::vector<int64_t> &shapeA,
    const std::vector<int64_t> &shapeB,
    uint32_t tileLength)
{
    if (tileLength == 0) {
        throw std::invalid_argument("tileLength must be non-zero");
    }
    log_add_exp::BroadcastInfo info;
    CHECK(Build(shapeA, shapeB, info) == log_add_exp::BroadcastStatus::SUCCESS);
    CHECK(info.totalLength > 0);
    CHECK(info.needsBroadcast != 0);

    const uint32_t tilesPerRow =
        info.rowLength / tileLength + (info.rowLength % tileLength == 0 ? 0 : 1);
    const uint32_t taskCount = info.rowCount * tilesPerRow;
    std::vector<uint32_t> coverage(info.totalLength, 0);
    for (uint32_t task = 0; task < taskCount; ++task) {
        const uint32_t rowIndex = task / tilesPerRow;
        const uint32_t tileIndexInRow = task % tilesPerRow;
        const uint32_t columnOffset = tileIndexInRow * tileLength;
        const uint32_t length =
            info.rowLength - columnOffset < tileLength ? info.rowLength - columnOffset : tileLength;
        const uint32_t outputOffset = rowIndex * info.rowLength + columnOffset;

        uint32_t inputAOffset = 0;
        uint32_t inputBOffset = 0;
        uint32_t remainingRowIndex = rowIndex;
        for (int32_t dim = static_cast<int32_t>(info.kernelRank) - 2; dim >= 0; --dim) {
            const uint32_t coordinate = remainingRowIndex % info.outputDims[dim];
            remainingRowIndex /= info.outputDims[dim];
            inputAOffset += coordinate * info.inputAStrides[dim];
            inputBOffset += coordinate * info.inputBStrides[dim];
        }
        inputAOffset += columnOffset * info.inputAStrides[info.kernelRank - 1];
        inputBOffset += columnOffset * info.inputBStrides[info.kernelRank - 1];

        for (uint32_t i = 0; i < length; ++i) {
            const uint32_t flatOutputOffset = outputOffset + i;
            ++coverage[flatOutputOffset];
            const uint32_t actualAOffset =
                inputAOffset + (info.inputAStrides[info.kernelRank - 1] == 0 ? 0 : i);
            const uint32_t actualBOffset =
                inputBOffset + (info.inputBStrides[info.kernelRank - 1] == 0 ? 0 : i);
            CHECK(
                actualAOffset ==
                ReferenceInputOffset(info, info.inputADims, flatOutputOffset));
            CHECK(
                actualBOffset ==
                ReferenceInputOffset(info, info.inputBDims, flatOutputOffset));
        }
    }
    for (uint32_t count : coverage) {
        CHECK(count == 1);
    }
}

void TestSameShapeFastPath()
{
    log_add_exp::BroadcastInfo info;
    CHECK(Build({2, 3}, {2, 3}, info) == log_add_exp::BroadcastStatus::SUCCESS);
    CHECK(info.logicalRank == 2);
    CHECK(info.kernelRank == 2);
    CHECK(info.totalLength == 6);
    CHECK(info.needsBroadcast == 0);
    AssertDims(info.outputDims, {2, 3});
    AssertDims(info.inputAStrides, {3, 1});
    AssertDims(info.inputBStrides, {3, 1});
}

void TestDifferentRank()
{
    log_add_exp::BroadcastInfo info;
    CHECK(Build({2, 3, 4}, {4}, info) == log_add_exp::BroadcastStatus::SUCCESS);
    CHECK(info.totalLength == 24);
    CHECK(info.rowLength == 4);
    CHECK(info.rowCount == 6);
    CHECK(info.needsBroadcast == 1);
    AssertDims(info.outputDims, {2, 3, 4});
    AssertDims(info.inputAStrides, {12, 4, 1});
    AssertDims(info.inputBStrides, {0, 0, 1});
}

void TestBothInputsBroadcast()
{
    log_add_exp::BroadcastInfo info;
    CHECK(Build({2, 1, 4}, {1, 3, 1}, info) == log_add_exp::BroadcastStatus::SUCCESS);
    CHECK(info.totalLength == 24);
    AssertDims(info.outputDims, {2, 3, 4});
    AssertDims(info.inputAStrides, {4, 0, 1});
    AssertDims(info.inputBStrides, {0, 1, 0});

    for (uint32_t i = 0; i < 2; ++i) {
        for (uint32_t j = 0; j < 3; ++j) {
            for (uint32_t k = 0; k < 4; ++k) {
                const uint32_t offsetA =
                    i * info.inputAStrides[0] +
                    j * info.inputAStrides[1] +
                    k * info.inputAStrides[2];
                const uint32_t offsetB =
                    i * info.inputBStrides[0] +
                    j * info.inputBStrides[1] +
                    k * info.inputBStrides[2];
                CHECK(offsetA == i * 4 + k);
                CHECK(offsetB == j);
            }
        }
    }
}

void TestScalar()
{
    log_add_exp::BroadcastInfo info;
    CHECK(Build({}, {2, 3}, info) == log_add_exp::BroadcastStatus::SUCCESS);
    CHECK(info.logicalRank == 2);
    CHECK(info.inputALength == 1);
    CHECK(info.totalLength == 6);
    CHECK(info.needsBroadcast == 1);
    AssertDims(info.inputAStrides, {0, 0});

    CHECK(Build({}, {}, info) == log_add_exp::BroadcastStatus::SUCCESS);
    CHECK(info.logicalRank == 0);
    CHECK(info.kernelRank == 1);
    CHECK(info.totalLength == 1);
    CHECK(info.needsBroadcast == 0);
}

void TestLeadingOneDoesNotNeedBroadcastKernel()
{
    log_add_exp::BroadcastInfo info;
    CHECK(Build({1, 3}, {3}, info) == log_add_exp::BroadcastStatus::SUCCESS);
    CHECK(info.totalLength == 3);
    CHECK(info.inputALength == 3);
    CHECK(info.inputBLength == 3);
    CHECK(info.needsBroadcast == 0);
}

void TestZeroElementTensorRejected()
{
    log_add_exp::BroadcastInfo info;
    CHECK(Build({0}, {1}, info) == log_add_exp::BroadcastStatus::ZERO_ELEMENT_TENSOR);
    CHECK(Build({2, 0}, {1}, info) == log_add_exp::BroadcastStatus::ZERO_ELEMENT_TENSOR);
    CHECK(Build({2, 0, 3}, {1, 1, 3}, info) == log_add_exp::BroadcastStatus::ZERO_ELEMENT_TENSOR);
    CHECK(Build({1}, {4, 3, 2, 0}, info) == log_add_exp::BroadcastStatus::ZERO_ELEMENT_TENSOR);
}

void TestInvalidShapes()
{
    log_add_exp::BroadcastInfo info;
    CHECK(Build({2, 3}, {4}, info) == log_add_exp::BroadcastStatus::INCOMPATIBLE_SHAPE);
    CHECK(Build({-1, 3}, {1, 3}, info) == log_add_exp::BroadcastStatus::INVALID_DIMENSION);
    CHECK(
        Build({1, 1, 1, 1, 1}, {1}, info) ==
        log_add_exp::BroadcastStatus::RANK_TOO_LARGE);
    CHECK(Build({65536, 65536}, {1}, info) == log_add_exp::BroadcastStatus::SIZE_OVERFLOW);
    CHECK(
        Build({static_cast<int64_t>(UINT32_MAX) + 1}, {1}, info) ==
        log_add_exp::BroadcastStatus::SIZE_OVERFLOW);
}

void TestRankBoundary()
{
    log_add_exp::BroadcastInfo info;
    CHECK(
        Build({2, 1, 3, 1}, {1, 7, 1, 8}, info) ==
        log_add_exp::BroadcastStatus::SUCCESS);
    CHECK(info.logicalRank == 4);
    AssertDims(info.outputDims, {2, 7, 3, 8});

    CHECK(
        Build({1, 1, 1, 1, 1}, {1}, info) ==
        log_add_exp::BroadcastStatus::RANK_TOO_LARGE);
}

void TestKernelTaskMapping()
{
    VerifyBroadcastTasks({2, 1, 4}, {1, 3, 1}, 3);
    VerifyBroadcastTasks({}, {17}, 4);
    VerifyBroadcastTasks({1, 5}, {7, 1}, 2);
    VerifyBroadcastTasks({2, 3, 4}, {4}, 5);
    VerifyBroadcastTasks({2, 1, 3, 1}, {1, 4, 1, 5}, 3);
}

} // namespace

int main()
{
    try {
        TestSameShapeFastPath();
        TestDifferentRank();
        TestBothInputsBroadcast();
        TestScalar();
        TestLeadingOneDoesNotNeedBroadcastKernel();
        TestZeroElementTensorRejected();
        TestInvalidShapes();
        TestRankBoundary();
        TestKernelTaskMapping();
    } catch (const std::exception &error) {
        std::cerr << "broadcast utility tests failed: " << error.what() << std::endl;
        return 1;
    }
    std::cout << "broadcast utility tests passed" << std::endl;
    return 0;
}
