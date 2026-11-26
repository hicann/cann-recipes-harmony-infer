#!/bin/bash
# Copyright (c) 2025, HUAWEI CORPORATION.  All rights reserved.
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
#
# SIM 仿真模式专用脚本（由 run.sh 拆分而来）
# 用法:
#   bash run_sim.sh -v Kirin9030 [选项]
# 选项:
#   -v, --soc-version   SoC 型号: Kirin9030 / KirinX90 / Kirin9020 /Kirin9050（默认 Kirin9030）
#   -i, --install-path  CANN 安装路径（默认取 $ASCEND_HOME_PATH）
#   -p, --prof          运行后执行 msopprof 仿真性能分析（结果存 ./prof_result）
#   -o, --prof-output   性能分析结果目录（默认 ./prof_result）
#   --skip-build        跳过编译+数据生成（复用已有 add_sim 与 input/*.bin）
#   --skip-verify       跳过精度校验
#
# 示例:
#   bash run_sim.sh -v Kirin9030                    # 仿真 + 精度校验
#   bash run_sim.sh -v Kirin9030 -p                 # 仿真 + 精度校验 + 性能分析
#   bash run_sim.sh -v Kirin9030 -p --skip-build    # 复用仿真 dump, 仅重新分析

export PRINT_TIK_MEM_ACCESS=FALSE

CURRENT_DIR=$(
    cd $(dirname ${BASH_SOURCE:-$0})
    pwd
); cd $CURRENT_DIR

declare -A VersionMap
#kirin
VersionMap["KirinX90"]="KirinX90"
VersionMap["Kirin9020"]="Kirin9020"
VersionMap["Kirin9030"]="Kirin9030"
VersionMap["Kirin9050"]="Kirin9050"
#迁移算子修改点：FILE_NAME控制编译获得的二进制名
FILE_NAME="add"
RUN_MODE="sim"

SHORT=v:,i:po:
LONG=soc-version:,install-path:,prof,prof-output:,skip-build,skip-verify
OPTS=$(getopt -a --options $SHORT --longoptions $LONG -- "$@")
eval set -- "$OPTS"

SOC_VERSION="Kirin9030"
PROF_ENABLED=0
PROF_OUTPUT="./prof_result"
SKIP_BUILD=0
SKIP_VERIFY=0

while :; do
    case "$1" in
    -v | --soc-version)
        SOC_VERSION="$2"
        shift 2
        ;;
    -i | --install-path)
        ASCEND_INSTALL_PATH="$2"
        shift 2
        ;;
    -p | --prof)
        PROF_ENABLED=1
        shift
        ;;
    -o | --prof-output)
        PROF_OUTPUT="$2"
        shift 2
        ;;
    --skip-build)
        SKIP_BUILD=1
        shift
        ;;
    --skip-verify)
        SKIP_VERIFY=1
        shift
        ;;
    --)
        shift
        break
        ;;
    *)
        echo "[ERROR] Unexpected option: $1"
        exit 1
        ;;
    esac
done

if [ -n "$ASCEND_INSTALL_PATH" ]; then
    _ASCEND_INSTALL_PATH=$ASCEND_INSTALL_PATH
elif [ -n "$ASCEND_HOME_PATH" ]; then
    _ASCEND_INSTALL_PATH=$ASCEND_HOME_PATH
else
    if [ -d "$HOME/Ascend/ascend-toolkit/latest" ]; then
        _ASCEND_INSTALL_PATH=$HOME/Ascend/ascend-toolkit/latest
    else
        _ASCEND_INSTALL_PATH=/usr/local/Ascend/ascend-toolkit/latest
    fi
fi

if [[ " ${!VersionMap[*]} " != *" $SOC_VERSION "* ]]; then
    echo "ERROR: SOC_VERSION should be in [${!VersionMap[*]}]"
    exit 1
fi
_SOC_VERSION=${VersionMap[$SOC_VERSION]}

if  [ $_SOC_VERSION"x" = "KirinX90x" ] || [ $_SOC_VERSION"x" = "Kirin9020x" ] || [ $_SOC_VERSION"x" = "Kirin9030x" ] || [ $_SOC_VERSION"x" = "Kirin9050x" ]; then
    CORE_TYPE="AiCore"
fi

# in case of running op in simulator, use stub so instead
export LD_LIBRARY_PATH=$_ASCEND_INSTALL_PATH/x86_64-linux/simulator/${_SOC_VERSION}/lib:$LD_LIBRARY_PATH
if [ ! $CAMODEL_LOG_PATH ]; then
    export CAMODEL_LOG_PATH=./sim_log
fi
rm -rf "$CAMODEL_LOG_PATH"
mkdir -p "$CAMODEL_LOG_PATH"

if [ "$SKIP_BUILD" -eq 0 ]; then
    set -e
    rm -rf build *_cpu *_sim *_npu cceprint npuchk *.vcd
    mkdir -p build
    cmake -B build \
        -Dsmoke_testcase=${FILE_NAME} \
        -DASCEND_PRODUCT_TYPE=${_SOC_VERSION} \
        -DASCEND_CORE_TYPE=${CORE_TYPE} \
        -DASCEND_RUN_MODE=${RUN_MODE} \
        -DASCEND_INSTALL_PATH=${_ASCEND_INSTALL_PATH}
    cmake --build build --target ${FILE_NAME}_${RUN_MODE}
    if [ $? -ne 0 ]; then
        echo "ERROR: compile op on ${RUN_MODE} failed!"
        exit -1
    fi
    echo "INFO: compile op on ${RUN_MODE} succeed!"

    rm -rf input/*.bin output/*.bin
    python3 scripts/gen_data.py
else
    if [ ! -x "./${FILE_NAME}_${RUN_MODE}" ]; then
        echo "ERROR: --skip-build 指定但 ./${FILE_NAME}_${RUN_MODE} 不存在, 请先正常执行一次!"
        exit 1
    fi
    echo "INFO: skip build, reuse ./${FILE_NAME}_${RUN_MODE}"
fi

# msopprof要求仿真dump目录不带group/other权限
chmod go-w . $CAMODEL_LOG_PATH

# 仿真产物 dump 到 $CAMODEL_LOG_PATH
./${FILE_NAME}_${RUN_MODE}
if [ $? -ne 0 ]; then
    echo "ERROR: execute op on ${RUN_MODE} failed!"
    exit -1
fi
echo "INFO: execute op on ${RUN_MODE} succeed!"

if [ "$SKIP_VERIFY" -eq 0 ]; then
    python3 scripts/verify_result.py output/output_z.bin output/golden.bin
fi

# ============ 性能分析 (可选, -p 开启) ============
# 注意: 仿真 dump 文件在 $CAMODEL_LOG_PATH (默认 ./sim_log), 勿删
if [ "$PROF_ENABLED" -eq 1 ]; then
    MSOPPROF=""
    if [ -x "$_ASCEND_INSTALL_PATH/bin/msopprof" ]; then
        MSOPPROF=$_ASCEND_INSTALL_PATH/bin/msopprof
    elif command -v msopprof >/dev/null 2>&1; then
        MSOPPROF=msopprof
    else
        echo "WARN: msopprof not found, skip profiling."
    fi
    if [ -n "$MSOPPROF" ]; then
        rm -rf ${PROF_OUTPUT}
        ${MSOPPROF} simulator --export=$CAMODEL_LOG_PATH --output=${PROF_OUTPUT} --soc-version=${_SOC_VERSION}
        if [ $? -eq 0 ]; then
            echo "INFO: profiling results saved to ${PROF_OUTPUT}/OPPROF_*/simulator/"
            echo "      - trace.json           (Chrome Trace, 可用 chrome://tracing / perfetto.dev 打开)"
            echo "      - core*.instr_exe_*.csv (指令级周期统计)"
        else
            echo "WARN: msopprof export failed!"
        fi
    fi
fi

# 注意: 此脚本不再删除 sim_log/*.vcd, 以便保留性能分析产物; 如需清理请手动执行:
#   rm -rf sim_log *.vcd
