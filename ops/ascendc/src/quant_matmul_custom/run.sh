# Copyright (c) 2025 Huawei Technologies Co., Ltd. All Rights Reserved.
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

#!/bin/bash
set -e

CURRENT_DIR=$(
    cd $(dirname ${BASH_SOURCE:-$0})
    pwd
)
cd $CURRENT_DIR

# 导出环境变量
if [ ! $ASCEND_INSTALL_PATH ]; then
    ASCEND_INSTALL_PATH=/usr/local/Ascend/latest
    source $ASCEND_INSTALL_PATH/bin/setenv.bash
fi
export ASCEND_TENSOR_COMPILER_INCLUDE=$ASCEND_INSTALL_PATH/compiler/include

function main() {
    # 1. 构建自定义算子包
    rm -rf build_out
    bash build.sh
    if [ $? -ne 0 ]; then
        echo "ERROR: build custom op run package failed!"
        return 1
    fi

    # 2. 安装自定义算子包
    cd build_out
    OS_ID=$(cat /etc/os-release | grep "^ID=" | awk -F= '{print $2}')
    OS_ID=$(echo $OS_ID | sed -e 's/^"//' -e 's/"$//')
    arch=$(uname -m)
    ./custom_opp_${OS_ID}_${arch}.run --quiet
    if [ $? -ne 0 ]; then
        echo "ERROR: install custom op run package failed!"
        return 1
    fi
    echo "INFO: install custom op run package success!"
}
IS_LIBRARY=$1
main $IS_LIBRARY