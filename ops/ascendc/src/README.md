# 算子工程编译安装指南
本项目开始前，请参考[环境准备](../../../docs/common/环境准备.md)进行环境搭建，并确保开发套件Ascend-cann-toolkit安装完成。
> **说明**：编译前请确认CMakePresets.json里`ASCEND_CANN_PACKAGE_PATH`字段设置为正确的toolkit包安装路径，一般为`${install_path}/latest`。
# 编译安装
- 切换至需要具体编译的算子目录，执行相应build.sh脚本即可完成该算子的编译。
- 编译成功后会在build_out目录生成对应的custom_opp_ubuntu_x86_64.run，执行./custom_opp_ubuntu_x86_64.run命令进行安装即可。
# 算子调试
- 算子编译安装完成后，可选择cpu和npu仿真调试运行，详细步骤请参考[Kernel直调样例运行](../KernelInvocationSample/README.md)。

- 此外，您还可以通过ATC工具转换包含该自定义算子的模型，然后调用鸿蒙运行态接口完成模型在鸿蒙设备上的推理运行。详细步骤请参考harmony_infer目录下的[应用部署介绍](../../../harmony_infer/README.md)。