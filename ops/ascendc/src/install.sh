#!/bin/bash
SHORT=v:,i:,p:,
LONG=soc-version:,install-path:,op-path:,
OPTS=$(getopt -a --options $SHORT --longoptions $LONG -- "$@")
eval set -- "$OPTS"

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
    -p | --op-path)
        OP_PATH="$2"
        shift 2
        ;;
    --)
        shift
        break
        ;;
    *)
        echo "[ERROR]: Unexpected option: $1"
        break
        ;;
    esac
done

VERSION_LIST="KirinX90 Kirin9030"
if [[ " $VERSION_LIST " != *" $SOC_VERSION "* ]]; then
    echo "[ERROR]: SOC_VERSION should be in [$VERSION_LIST]"
    exit -1
fi

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
source $_ASCEND_INSTALL_PATH/bin/setenv.bash
export ASCEND_HOME_PATH=$_ASCEND_INSTALL_PATH

last_part=$(basename "$OP_PATH")
outPath=$last_part"_Op"
rm -rf $outPath

# Generate the op framework
msopgen gen -i $OP_PATH/*.json -c ai_core-${SOC_VERSION} -lan cpp -f onnx -out $outPath 
# Copy op implementation files to CustomOp
cp -rf $OP_PATH/* $outPath

# Build CustomOp project
(cd $outPath && bash build.sh)