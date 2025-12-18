# 算子工程编译安装指南
本项目开始前，请参考[环境准备](../../../docs/quick_install.md)进行环境搭建，并确保开发套件Ascend-cann-toolkit安装完成。
> **说明**：编译前请确认各算子目录CMakePresets.json里`ASCEND_CANN_PACKAGE_PATH`字段设置为正确的toolkit包安装路径，一般为`${install_path}/latest`。
# 编译安装
执行相应算子工程目录下的build.sh脚本即可完成该算子的编译安装。
# 算子调试
- 参考[AddKernelInvocation](../KernelInvocationSample/README.md)样例，将kernel实现部分拷贝到add_custom.cpp，执行npu仿真调试。

- 您还可以执行相应算子test目录下的create_onnx.py和gen_data.py，生成相应的onnx单算子测试模型、输入输出数据后，通过ATC工具转换测试模型，最后调用鸿蒙维测接口完成单算子的性能和精度验证。详细流程请参考[维测调优](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/cannkit-debugging-and-optimization)。