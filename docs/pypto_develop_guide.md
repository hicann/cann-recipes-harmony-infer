# 1. 环境准备

## 1.1 安装 CANN 包

CANN（Compute Architecture for Neural Networks）是昇腾 AI 计算引擎，提供算子开发所需的工具链和运行时支持。

- **安装包链接**：[cann包 链接](https://ascend.devcloud.huaweicloud.com/artifactory/cann-run-mirror/software/master/)（用最新日期的）

安装 cann-toolkit 包：

```bash
# 安装 Toolkit 包
./Ascend-cann-toolkit_9.3.0~weekly.20260916.01_linux-x86_64.run  --full --install-path=${install_path}
```

## 1.2 安装 pypto包

详见 [pypto 安装文档](https://gitcode.com/cann/pypto/blob/master/docs/zh/install/build_and_install.md)。

# 2. 注册安装 pypto 自定义算子

## 2.1 kernel.py

pypto kernel 函数：

```bash
import pypto

# ── KERNEL ── the pypto compute a kernel author writes; JIT-built at deploy, run by the backend.
@pypto.frontend.jit(runtime_options={"run_mode": pypto.RunMode.SIM})
def add_kernel(
    input0: pypto.Tensor([...]),
    input1: pypto.Tensor([...]),
    output: pypto.Tensor([...]),
):
    pypto.set_vec_tile_shapes(*add_infer_tileshape(input0, input1))
    output.move(input0 + input1)

def add_infer_tileshape(input0, input1) -> tuple[int, ...]:  # tiling hint (kernel-side); called by the kernel body
    return (1, 4, 1, 64)
```

## 2.2 export_demo.py

修改原本基于 torch 的 Python 脚本，将普通的算子替换为 pypto 的自定义算子，并导出 ONNX 文件。

**定义 pypto 自定义算子函数：**

```bash
import torch
import torch.nn as nn

import argparse
import os
import pathlib as _pl, sys as _sys

import pypto

# Demo helpers live in examples/04_custom_op (not in pypto — keeps the export call in the demo).
_sys.path.insert(0, str(_pl.Path(__file__).resolve().parents[3]))  # examples/04_custom_op
from lib.export.onnx_export import onnx_export_session, resolve_pypto_export_opset
from lib.run.demo_run import (
    add_run_args, validate_args, scenario_label, npu_device_id, move_inputs_to_npu,
    enable_eslmodel, print_outputs, pypto_run_context, RunCliError, RunEnvError,
)

# The kernel + its inference functions live in a sibling ``kernel.py`` (this demo's own folder).
_sys.path.insert(0, str(_pl.Path(__file__).resolve().parent))  # demos/onnx/add
from kernel import add_kernel

ONNX_OPSET_VERSION = 12  # base ai.onnx opset; used as the minimum ONNX version for the export
PYPTO_OP_TYPE__ADD = "Add"  # op name; the graph node type is "PyptoCustomOp" + this
SHAPE = (1, 8, 1, 64)  # 512 elems — kept small so the SIM run stays fast; tile (1, 4, 1, 64)

# ── RUN ── the CPU compute for this op, also used as the reference (made available to torch when this module is imported).
def add_torch(input0, input1):
    return input0 + input1

# ── shape/dtype inference ── output shape+dtype: drives inference both while tracing and in the deployed kernel.
def add_infer_shape(input0_shape: torch.Size, input1_shape: torch.Size) -> torch.Size:
    return input0_shape

def add_infer_dtype(input0_dtype: torch.dtype, input1_dtype: torch.dtype) -> torch.dtype:
    return input0_dtype

# ── EXPORT/core ── declare the op and its ONNX wiring as config; pypto generates the torch op + node.
# The kernel + infer/torch_defn block is passed FIRST, then the export wiring.
add_op = pypto.extensions.torch_custom_op.CustomOp(
    # the kernel + its inference/reference functions, passed by reference
    kernel=add_kernel,
    infer_shape=add_infer_shape,
    infer_dtype=add_infer_dtype,
    torch_defn=add_torch,
    # export wiring (torch op + framework spec), generated automatically during export
    # op_type names the COMPUTE in the ONNX graph (may repeat across demos); torch_op_qualname is the unique per-op torch-registry key.
    torch_op_qualname="pypto::add",
    onnx_spec=pypto.extensions.torch_custom_op.OnnxSymbolicSpec(op_type=PYPTO_OP_TYPE__ADD, opset_version=ONNX_OPSET_VERSION),
    # custom meta -> node attrs
    support_dynamic_aligned=True, version=1,
)

# ── EXPORT/ONNX ── trace the model and run torch.onnx.export inside the session.
class CustomModel(nn.Module):
    def forward(self, input0, input1):
        return torch.ops.pypto.add(input0, input1)
```

**导出 ONNX 函数：**

```bash
def export_demo(model, path: str):
    pypto.set_codegen_options(support_dynamic_aligned=True)
    input_data0 = torch.rand(SHAPE, dtype=torch.float16)
    input_data1 = torch.rand(SHAPE, dtype=torch.float16)

    out_dir = os.path.dirname(path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    opset = resolve_pypto_export_opset(model=model, example_inputs=(input_data0, input_data1))  # base ai.onnx opset to export with
    # ── EXPORT/ONNX (session) ── register the custom ops (safe to call more than once), then validate/save the model on exit.
    with onnx_export_session(path):
        torch.onnx.export(
            model, (input_data0, input_data1), path,
            input_names=["x0", "x1"], output_names=["y"],
            opset_version=opset, do_constant_folding=False, dynamo=False, report=True,
        )
```

## 2.3 build_so_and_setup_demo.py

代码详见[pypto代码仓 /examples](https://https://gitcode.com/cann/pypto/tree/master/examples)。

# 3. ATC 转换

在注册安装好 pypto 自定义算子之后，可以使用 ATC 工具将 ONNX 编译成 omc 文件，用于上板验证。

```bash
export CANN_INSTALL_PATH=/xxx/Ascend   # CANN 包的安装路径
source ${CANN_INSTALL_PATH}/cann/set_env.sh

# 导出 ONNX
python3 export_demo.py tmp/model.onnx

# 构造自定义算子工程
python3 ../build_so_and_setup_demo.py tmp/model.onnx

export ASCEND_CUSTOM_OPP_PATH=/xxx/tmp/build_op_kernel_lib/build

# ONNX 转换 omc
atc --model=tmp/model.onnx \
    --framework=5 \
    --output=tmp/model_pto \
    --soc_version=KirinX90 \
    --log=debug \
    --target=omc \
    --enable_single_stream=true > atc.log 2>&1
```
