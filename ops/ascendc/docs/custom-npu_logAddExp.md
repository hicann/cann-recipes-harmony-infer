# 应用场景说明

在酷狗音乐音频超分模型推理中，需要使用 LogAddExp 对两个张量进行逐元素计算。为避免直接计算 `log(exp(A) + exp(B))` 时产生数值溢出，本样例基于 Ascend C 实现数值稳定的 FP16 LogAddExp 算子。此方案已在酷狗音乐音频超分业务中得到应用实践并验证满足精度要求。

# LogAddExp 自定义算子样例说明

本样例提供 LogAddExp 的 Host 侧算子注册、Shape/DType 推导与 Tiling 实现、AI Core Kernel、ONNX 框架适配插件和精简测试脚本，可用于含自定义 LogAddExp 节点的 ONNX 模型转换与鸿蒙端侧部署。

# 算子描述

LogAddExp 的数学表达式为：

```text
LogAddExp(A, B) = log(exp(A) + exp(B))
                = max(A, B) + log(1 + exp(-abs(A - B)))
```

第二种形式可避免直接计算指数时产生溢出。该算子为逐元素运算，并支持从尾维对齐的广播。

# 算子规格描述

| 名称 | 角色 | 数据类型 | 形状 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| A | 输入 | fp16 | rank 0～4 | 非空 Tensor |
| B | 输入 | fp16 | rank 0～4 | 非空 Tensor，形状与 A 满足广播规则 |
| C | 输出 | fp16 | A、B 的广播结果 | `LogAddExp(A, B)` |

- `shape=[]` 表示含一个元素的标量。
- 不支持含零维度的空 Tensor、未知维度或 rank 大于 4 的输入。
- 特殊值处理由 `op_kernel/log_add_exp.cpp` 顶部的
  `LOGADDEXP_STRICT_SPECIAL` 宏控制。默认值为 `0`，此时输入需为有限值；
  置为 `1` 后启用 NaN 传播及正负 Inf 修正，其特殊值分类语义与
  PyTorch/NumPy LogAddExp 一致，但会降低算子性能，可根据模型情况选择。

# 支持的产品型号

- Kirin 9020 处理器系列产品

# 编译安装执行

参考[LogAddExp 算子工程说明](../src/log_add_exp_custom/README.md)。
