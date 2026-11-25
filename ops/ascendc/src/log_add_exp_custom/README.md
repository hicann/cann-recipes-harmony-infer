# LogAddExp 自定义算子

## 简述

`LogAddExp` 是基于 Ascend C 实现的 FP16 逐元素算子，目标设备为 Kirin 9020 处理器。算子按照 PyTorch/NumPy 规则支持广播，并采用数值稳定公式：

```text
LogAddExp(A, B) = max(A, B) + log(1 + exp(-abs(A - B)))
```

本样例来源于酷狗音乐音频超分业务实践。

## 算子规格

| 名称 | 角色 | 数据类型 | 形状 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| A | 输入 | fp16 | rank 0～4 | 非空 Tensor |
| B | 输入 | fp16 | rank 0～4 | 非空 Tensor，形状与 A 满足广播规则 |
| C | 输出 | fp16 | A、B 的广播结果 | `LogAddExp(A, B)` |

- 广播从尾维开始对齐，每个对齐维必须相等或其中一个为 `1`。
- `shape=[]` 表示含一个元素的标量；不支持含零维度的空 Tensor。
- 可选支持 NaN 传播，以及 `+Inf`、`-Inf` 的 LogAddExp 语义：由
  `op_kernel/log_add_exp.cpp` 顶部的 `LOGADDEXP_STRICT_SPECIAL` 宏控制。
  默认值为 `0`，此时输入需为有限值；置为 `1` 后启用特殊值修正，其
  特殊值分类语义与 PyTorch/NumPy LogAddExp 一致，但会降低算子性能，可根据模型情况选择。
- 输入 A、B 必须同时为 fp16。

## 支持的产品型号

- Kirin 9020 处理器系列产品

## 目录结构

```text
log_add_exp_custom
├── build_and_install.sh
├── CMakeLists.txt
├── CMakePresets.json
├── framework
│   ├── CMakeLists.txt
│   └── onnx_plugin
│       ├── CMakeLists.txt
│       └── log_add_exp_plugin.cpp
├── op_host
│   ├── CMakeLists.txt
│   ├── log_add_exp.cpp
│   ├── log_add_exp_broadcast.h
│   └── log_add_exp_tiling.h
├── op_kernel
│   ├── CMakeLists.txt
│   └── log_add_exp.cpp
└── test
    ├── create_onnx.py
    ├── gen_data.py
    └── test_broadcast_utils.cpp
```

## 操作步骤

### 1. 环境准备

参考[环境准备](../../../../docs/quick_install.md)完成环境搭建，确保 Ascend-cann-toolkit 已安装。编译前确认 `CMakePresets.json` 中的 `ASCEND_CANN_PACKAGE_PATH` 指向正确的 toolkit 安装路径。

### 2. 编译安装

```bash
chmod +x build_and_install.sh
./build_and_install.sh
```

### 3. 生成单算子模型和测试数据

```bash
cd test
python3 create_onnx.py
python3 gen_data.py --strict-special 0
```

`--strict-special` 应与 Kernel 中的 `LOGADDEXP_STRICT_SPECIAL` 宏取值一致：
值为 `0` 时生成有限随机输入；值为 `1` 时生成包含正负 Inf、正负
qNaN/sNaN 的特殊值输入。严格模式测试数据可通过以下命令生成：

```bash
python3 gen_data.py --strict-special 1
```

脚本生成两个输入文件和 `golden.bin`，`create_onnx.py` 生成
`LogAddExp.onnx`。生成物仅用于本地验证，不应提交到代码仓。

随后可通过 ATC 工具转换测试模型，并调用鸿蒙维测接口完成精度验证，详细流程参考[算子工程编译安装指南](../README.md)。

### 4. 广播元数据单元测试

该测试不依赖 CANN，可在支持 C++11 的环境中执行：

```bash
c++ -std=c++11 -Wall -Wextra -Werror \
  test/test_broadcast_utils.cpp \
  -o /tmp/test_broadcast_utils
/tmp/test_broadcast_utils
```
