# 1. 环境准备

## 1.1 安装 CANN 包

CANN（Compute Architecture for Neural Networks）是昇腾 AI 计算引擎，提供算子开发所需的工具链和运行时支持。

- **安装包链接**：[cann包 链接](https://ascend.devcloud.huaweicloud.com/artifactory/cann-run-mirror/software/master/)（用最新日期的）

安装 cann-toolkit 包：

```bash
# 安装 Toolkit 包
./Ascend-cann-toolkit_9.3.0~weekly.20261014.01_linux-x86_64.run  --full --install-path=${install_path}
```

## 1.2 安装 910b ops 算子包

**必须安装**，否则 ATC 会报 `E10501: Data op IR not registered`（内置算子 IR 缺失）。

```bash
# 安装 910b ops 包（版本需与 toolkit 匹配，同样取最新日期的）
./Ascend-cann-910b-ops_9.3.0~weekly.20261014.01_linux-x86_64.run --install-path=${install_path}
```

# 2. 注册安装 pypto 自定义算子

## 2.1 kernel.py

pypto kernel 函数：

```bash
import pypto


# the pypto compute a kernel author writes; JIT-built at deploy, run by the backend.
@pypto.frontend.jit(runtime_options={"run_mode": pypto.RunMode.SIM})
def add_kernel(
    input0: pypto.Tensor([...]),
    input1: pypto.Tensor([...]),
    output: pypto.Tensor([...]),
):
    pypto.set_vec_tile_shapes(1, 4, 1, 64)
    output.move(input0 + input1)
```

## 2.2 export_demo.py

修改原本基于 torch 的 Python 脚本，将普通的算子替换为 pypto 的自定义算子，并导出 ONNX 文件。

**定义 pypto 自定义算子函数：**

```bash
import argparse
import os
import pathlib as _pl
import sys as _sys
import tempfile

import torch

import pypto

# The exported_custom_op_litenpu helper library lives under the repo-root tools/ directory.
_sys.path.insert(0, str(_pl.Path(__file__).resolve().parents[5] / "tools"))  # <repo root>/tools
from exported_custom_op_litenpu.export.onnx_export import onnx_export_session, resolve_pypto_export_opset
from model import SHAPE, CustomModel


# trace the model and run torch.onnx.export inside the session.
def export_demo(model, path: str):
    pypto.set_codegen_options(support_dynamic_aligned=True)
    input_data0 = torch.rand(SHAPE, dtype=torch.float16)
    input_data1 = torch.rand(SHAPE, dtype=torch.float16)

    out_dir = os.path.dirname(path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    # base ai.onnx opset to export with
    opset = resolve_pypto_export_opset(model=model, example_inputs=(input_data0, input_data1))
    # register the custom ops (safe to call more than once), then
    # validate/save the model on exit.
    with onnx_export_session(path):
        torch.onnx.export(
            model, (input_data0, input_data1), path,
            input_names=["x0", "x1"], output_names=["y"],
            opset_version=opset, do_constant_folding=False, dynamo=False, report=True,
        )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=str, nargs="?", help="onnx model path (default: a temp dir)")
    args = parser.parse_args()
    # Optional so the repo examples sweep can run this bare (a fresh temp dir when omitted).
    path = args.path or os.path.join(tempfile.mkdtemp(prefix="pypto_add_"), "add.onnx")
    export_demo(CustomModel(), path=path)
```

## 2.3 build_so_and_setup.py

代码详见[链接](https://gitcode.com/cann/pypto/blob/master/tools/exported_custom_op_litenpu/deploy/build_so_and_setup.py)。

# 3. ATC 转换

在注册安装好 pypto 自定义算子之后，可以使用 ATC 工具将 ONNX 编译成 omc 文件，用于上板验证。

```bash
export CANN_INSTALL_PATH=/xxx/Ascend   # CANN 包的安装路径
source ${CANN_INSTALL_PATH}/cann/set_env.sh

# 导出 ONNX
python3 export_demo.py tmp/model.onnx

# 构造自定义算子工程
python3 build_so_and_setup.py tmp/model.onnx

export ASCEND_CUSTOM_OPP_PATH=/xxx/tmp/build_op_kernel_lib/build

# ONNX 转换 omc
atc --model=tmp/model.onnx \
    --framework=5 \
    --output=tmp/model_pto \
    --soc_version=KirinX90 > atc.log 2>&1
```

> 注意：  `--output` 无需写后缀，工具会自动生成 `model_pto.omc`。
