#!/bin/bash
# Copyright (c) 2025 Huawei Technologies Co., Ltd. All Rights Reserved.
# Copyright (c) 2026 Tang Risheng
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -eo pipefail

if [[ -n "${BASE_LIBS_PATH:-}" ]]; then
    ASCEND_HOME_PATH="${BASE_LIBS_PATH}"
elif [[ -n "${ASCEND_HOME_PATH:-}" ]]; then
    ASCEND_HOME_PATH="${ASCEND_HOME_PATH}"
elif [[ -n "${ASCEND_AICPU_PATH:-}" ]]; then
    ASCEND_HOME_PATH="${ASCEND_AICPU_PATH}"
else
    ASCEND_HOME_PATH="/usr/local/Ascend/cann"
fi

if [[ ! -f "${ASCEND_HOME_PATH}/bin/setenv.bash" ]]; then
    echo "ERROR: CANN environment not found under ${ASCEND_HOME_PATH}."
    exit 1
fi

# CANN setenv.bash 内部存在 grep 无匹配等返回非零的管道，
# 在 set -eo pipefail 下会让 shell 静默退出，这里临时放宽。
set +e +o pipefail
source "${ASCEND_HOME_PATH}/bin/setenv.bash"
set -eo pipefail
export ASCEND_HOME_PATH
export ASCEND_TENSOR_COMPILER_INCLUDE="${ASCEND_HOME_PATH}/compiler/include"

MMPA_LIBRARY="${ASCEND_HOME_PATH}/x86_64-linux/lib64/libmmpa.so"
if [[ -f "${MMPA_LIBRARY}" ]]; then
    export LD_PRELOAD="${MMPA_LIBRARY}${LD_PRELOAD:+:${LD_PRELOAD}}"
fi

SCRIPT_DIR=$(realpath "$(dirname "$0")")
BUILD_DIR="${SCRIPT_DIR}/build_out"
TARGET="${1:-package}"
JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '8')

echo "using ASCEND_HOME_PATH: ${ASCEND_HOME_PATH}"
cd "${SCRIPT_DIR}"
cmake -S . -B "${BUILD_DIR}" --preset=default \
    -DASCEND_CANN_PACKAGE_PATH="${ASCEND_HOME_PATH}"
cmake --build "${BUILD_DIR}" --target binary -j"${JOBS}"
cmake --build "${BUILD_DIR}" --target "${TARGET}" -j"${JOBS}"

OS_ID=$(awk -F= '/^ID=/{gsub(/"/, "", $2); print $2}' /etc/os-release)
ARCH=$(uname -m)
RUN_PACKAGE="${BUILD_DIR}/custom_opp_${OS_ID}_${ARCH}.run"
if [[ ! -x "${RUN_PACKAGE}" ]]; then
    echo "ERROR: custom op run package not found: ${RUN_PACKAGE}"
    exit 1
fi

"${RUN_PACKAGE}" --quiet
echo "INFO: install custom op run package success!"
