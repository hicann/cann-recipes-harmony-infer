# ATC模型转换指南
ATC是异构计算架构CANN体系下的模型转换工具，它可以将开源框架的网络模型以及Ascend IR定义的单算子描述文件（JSON格式）转换为昇腾、麒麟AI处理器支持的.om格式离线模型。
##  环境准备
1. 参见[环境准备](./环境准备.md)进行环境搭建，并确保开发套件Ascend-cann-toolkit安装完成。该场景下ATC工具安装在`${install_path}/latest/bin`目录下；其中，`${install_path}`请替换为CANN软件安装目录，以root安装举例，安装目录为：`/usr/local/Ascend/ascend-toolkit`。

2. 设置公共环境变量
安装CANN软件后，使用CANN运行用户进行编译、运行时，需要以CANN运行用户登录环境，执行如下环境变量：
```bash
source ${install_path}/set_env.sh
export LD_LIBRARY_PATH=${install_path}/latest/<arch>-linux/devlib:$LD_LIBRARY_PATH
```
## 模型转换
### ONNX网络模型转换成离线模型
```bash
atc --model=$HOME/module/resnet50*.onnx --framework=5 --output=$HOME/module/out/onnx_resnet50 --soc_version=<soc_version>
```
   - --model：Resnet50网络模型文件所在路径。
   - --framework：原始框架类型，5表示ONNX。
   - --output：生成的离线模型路径。
   - --soc_version：Kirin AI处理器的型号，如kirinx90、kirin9030等。
### *.air格式的模型文件转换成离线模型
```bash
atc --model=$HOME/module/ResNet50.air --framework=1 --output=$HOME/module/out/ResNet50_air --soc_version=<soc_version>
```
   - --model：*.air格式的模型文件所在路径。
   - --framework：原始框架类型，1表示*.air格式的模型文件。
   - --output：生成的离线模型路径。
   - --soc_version：Kirin AI处理器的型号，如kirinx90、kirin9030等。
### 单算子描述文件转换成离线模型
单算子描述文件是基于Ascend IR定义的单个算子的定义文件，包括算子的输入、输出及属性等信息，借助该文件转换成适配昇腾、麒麟AI处理器的离线模型后，可以验证单算子的功能。详细配置请参考[单算子描述文件配置](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/devaids/atctool/atlasatc_16_0034.html)。
```bash
atc --singleop=$HOME/singleop/add.json --output=$HOME/singleop/out/op_model --soc_version=<soc_version>
```
   - --singleop：用于指定add.json单算子描述文件。
   - --output：转换后的离线模型存放路径。
   - --soc_version：Kirin AI处理器的型号，如kirinx90、kirin9030等。

### 附录1 ATC参数说明
# ATC参数

使用ATC工具转换模型之前，首先查看使用工具过程中的一些限制，然后借助本章节提供的参数概览功能，可以快速预览相关参数。


|ATC参数名称   |参数简述     |是否必选   |默认值   |
| ------------ | ------------ | ------------ | ------------ |
|--help或--h   |显示帮助信息。   |否   |不涉及   |
|--mode        |运行模式。       |否   |0   |
|--model   |	原始模型文件路径与文件名。。   |是   |不涉及   |
|--framework   |原始框架类型。   |是   |不涉及   |
|--weight   |权重文件路径与文件名。   |否   |不涉及   |
|--input_format | 输入数据格式。 | 否 | Caffe、MindSpore、ONNX默认为NCHW；TensorFlow默认为NHWC |
|--input_shape| 模型输入数据的shape。 | 否 | 不涉及 |
|--dynamic_batch_size| 设置动态batch档位参数，适用于执行推理时，每次处理图片数量不固定的场景。 | 否 | 不涉及 |
|--dynamic_image_size| 设置输入图片的动态分辨率参数。适用于执行推理时，每次处理图片宽和高不固定的场景。 | 否 | 不涉及 |
|--dynamic_dims| 设置ND格式下动态维度的档位。适用于执行推理时，每次处理任意维度的场景。 | 否 | 不涉及 |
|--om| 需要转换为JSON格式的离线模型或原始模型文件的路径和文件名。 | 否 | 不涉及 |
|--singleop| 单算子定义文件，将单个算子JSON文件转换成适配昇腾AI处理器的离线模型。 | 否 | 不涉及 |
|--distributed_cluster_build| 用于分布式部署的模型编译开关，使能该参数后，生成的离线模型将用于分布式部署。 | 否 | 默认为0 |
|--cluster_config| 指定目标执行环境的逻辑拓扑结构配置文件。 | 否 | 不涉及 |
| --enable_graph_parallel | 是否对原始模型进行自动切分。 | 否 | 默认为0 |
| --graph_parallel_option_path | 对原始模型进行切分时，切分策略配置文件路径。 | 否 | 不涉及 |
| --shard_model_dir | 指定切片模型文件所在路径。 | 否 | 不涉及 |
| --model_relation_config | 表达多个切片模型间的数据关联和分布式通信组关系的配置文件。 | 否 | 不涉及 |
| --output | ● 如果是开源框架的网络模型，存放转换后的离线模型的路径以及文件名。<br>● 如果是单算子描述文件，存放转换后的单算子模型的路径。 | 是 | 不涉及 |
| --output_type | 指定网络输出数据类型或指定某个输出节点的输出类型。 | 否 | 不涉及 |
| --check_report | 预检结果保存文件路径和文件名。 | 否 | 执行atc命令当前路径生成 check_result.json |
| --json | 离线模型或原始模型文件转换为JSON格式文件的路径和文件名。 | 否 | 不涉及 |
| --host_env_os | 若模型编译环境的操作系统及其架构与模型运行环境不一致时，则需使用本参数设置模型运行环境的操作系统类型。 | 否 | 执行**atc --hel**p命令查看“--host_env_os”参数的默认值或查看${INSTALL_DIR}/opp/scene.info文件中的取值 |
| --host_env_cpu | 若模型编译环境的操作系统及其架构与模型运行环境不一致时，则需使用本参数设置模型运行环境的操作系统架构。 | 否 | 执行**atc --help**命令查看“--host_env_cpu”参数的默认值或查看${INSTALL_DIR}/opp/scene.info文件中的取值 |
| --soc_version | 模型转换时指定芯片版本。 | 是 | 不涉及 |
| --core_type | 设置网络模型使用的Core类型，若网络模型中包括Cube算子，则只能使用 AiCore。 | 否 | AiCore |
| --aicore_num | 设置模型编译时使用的aicore数目。 | 否 | 默认值为最大值 |
| --virtual_type | 是否支持离线模型在算力分组生成的虚拟设备上运行。 | 否 | 0 |
| --out_nodes | 指定输出节点。 | 否 | 不涉及 |
| --input_fp16_nodes | 指定输入数据类型为FP16的输入节点名称。 | 否 | 不涉及 |
| --insert_op_conf | 插入算子的配置文件路径与文件名，例如aipp预处理算子。 | 否 | 不涉及 |
| --external_weight | 生成om离线模型时，是否将原始网络中的Const/Constant节点的权重保存在单独的文件中，同时将节点类型转换为 FileConstant类型。 | 否 | 0 |
| --op_name_map | 扩展算子（非标准算子）映射配置文件路径和文件名，不同的网络中某扩展算子的功能不同，可以指定该扩展算子到具体网络中实际运行的扩展算子的映射。 | 否 | 不涉及 |
| --is_input_adjust_hw_layout | 用于指定网络输入数据类型是否为FP16，数据格式是否为NC1HWC0。 | 否 | false |
| --is_output_adjust_hw_layout | 用于指定网络输出的数据类型是否为FP16，数据格式是否为NC1HWC0。 | 否 | false |
| --buffer_optimize | 是否开启数据缓存优化。 | 否 | l2_optimize |
| --disable_reuse_memory | 内存复用开关。 | 否 | 0 |
| --enable_scope_fusion_passes | 指定编译时需要生效的融合规则列表。 | 否 | 不涉及 |
| --fusion_switch_file | 融合开关配置文件路径以及文件名。 | 否 | 不涉及 |
| --enable_single_stream | 是否使能模型推理顺序单流串行执行。 | 否 | false |
| --enable_small_channel | 是否使能small channel的优化，使能后在channel<=4的首层卷积会有性能收益。 | 否 | 0 |
| --quant_dumpable | 是否采集量化算子的dump数据。 | 否 | 0 |
| --compression_optimize_conf | 压缩优化功能配置文件路径以及文件名。 | 否 | 不涉及 |
| --mdl_bank_path | 加载模型调优后自定义知识库的路径。 | 否 | ${HOME}/Ascend/latest/data/aoe/custom/graph/<soc_version> |
| --sparsity | 使能全局稀疏特性。 | 否 | 0 |
| --ac_parallel_enable | 动态shape图中，是否允许AI CPU算子和AI Core算子并行运行。 | 否 | 0 |
| --oo_level | 图编译多级选项优化。 | 否 | O3 |
| --oo_constant_folding | 是否开启常量折叠优化。 | 否 | true |
| --oo_dead_code_elimination | 是否开启死边消除优化。 | 否 | true |
| --tiling_schedule_optimize | Tiling下沉调度优化开关。 | 否 | 0 |
| --topo_sorting_mode | 对算子进行图模式编译时，可选择的不同的图遍历模式。 | 否 | 1 |
| --precision_mode | 设置网络模型的精度模式。 | 否 | force_fp16 |
| --precision_mode_v2 | 设置网络模型的精度模式。与7.3.3.1 --precision_mode不能同时使用，推荐使用--precision_mode_v2。 | 否 | fp16 |
| --op_precision_mode | 设置具体某个算子的精度模式，通过该参数可以为不同的算子设置不同的精度模式。 | 否 | 不涉及 |
| --modify_mixlist | 混合精度场景下，修改算子使用混合精度名单。 | 否 | 不涉及 |
| --optypelist_for_implmode | 设置optype列表中算子的实现模式，算子实现模式包括high_precision、 high_performance两种。 | 否 | 不涉及 |
| --keep_dtype | 通过配置文件指定原始模型中特定算子的数据类型在模型编译过程中保持不变。 | 否 | 不涉及 |
| --customize_dtypes | 模型编译时自定义算子的计算精度。 | 否 | 不涉及 |
| --op_bank_path | 加载AOE调优后自定义知识库的路径。 | 否 | 默认自定义知识库路径 $HOME/Ascend/latest/data/aoe/custom/op |
| --allow_hf32 | 是否启用HF32自动代替FP32数据类型的功能。 | 否 | 针对Conv类算子，启用 FP32转换为 HF32；针对 Matmul类算子，关闭 FP32转换为 HF32。 |
| --is_weight_clip | 是否对浮点类型权重数据进行裁剪。 | 否 | 1 |
| --dump_mode | 是否生成带shape信息的JSON文件。 | 否 | 0 |
| --log | 设置ATC模型转换过程中显示日志的级别。 | 否 | null |
| --display_model_info | 模型编译时或对已有的离线模型，查询模型占用的关键资源信息、编译与运行环境等信息。 | 否 | 0 |
| --op_compiler_cache_mode | 用于配置算子编译磁盘缓存模式。 | 否 | disable |
| --op_compiler_cache_dir | 用于配置算子编译磁盘缓存的目录。 | 否 | $HOME/atc_data |
| --op_debug_level | TBE算子编译debug功能开关。 | 否 | 0 |
| --op_debug_config | 使能Global Memory（DDR）内存检测功能的配置文件路径及文件名。 | 否 | 不涉及 |
| --debug_dir | 用于配置保存模型转换、网络迁移过程中算子编译生成的调试相关过程文件的路径，包括算子.o/.json/.cce等文件。 | 否 | ./kernel_meta |
| --atomic_clean_policy | 是否集中清理网络中所有memset算子（含有memset属性的算子都是memset算子）占用的内存。 | 否 | 0 |
| --deterministic | 是否开启确定性计算。 | 否 | 0 |
| --status_check | 控制编译算子时是否添加溢出检测逻辑。 | 否 | 0 |
| --export_compile_stat | 配置图编译过程中是否生成算子融合信息（包括图融合和UB融合）的结果文件 fusion_result.json。 | 否 | 1 |


