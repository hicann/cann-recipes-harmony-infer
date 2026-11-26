# 开发准备



## 环境准备

- 使用Ubuntu 64位运行[Tools下载](#tools下载)中的ATC模型转换工具。

- 推荐使用[Ubuntu 22.04](https://mirrors.ustc.edu.cn/ubuntu-releases/22.04/)及以上版本、MacOS 10.14及以上版本、Windows 10及以上版本安装应用开发环境[DevEco Studio](https://developer.huawei.com/consumer/cn/deveco-studio/)。

- 准备训练好的使用CANN包中ATC工具生成的离线模型或者从Model Zoo中选择合适的模型。


## Tools下载

| Tools名称 | Tools说明 | Tools下载 |
| -------- | -------- | -------- |
| CANN基础包 | CANN基础包包含ATC、AscendC/PyPTO算子开发工具。<br>ATC工具：模型转换工具,将三方模型转为om或者omc。<br>AscendC/PyPTO自定义算子开发：提供AscendC/PyPTO算子开发、性能调优等能力。<br> | [Ascend-cann-toolkit-9.2.0-linux-x86_64-mobile-station.run](https://kiri-obs.obs.cn-north-4.myhuaweicloud.com/Cann%20Large%20Model%20Foundation%208.5.0.rc004/Ascend-cann-toolkit_9.2.0_linux-x86_64-mobile-station.run) |
| CANN扩展包 | CANN扩展包包含模型dopt量化、模型profiling和dump能力，以及ATC扩展能力(支持内置算子编译)。<br>轻量化工具（tools_dopt）：对原始模型进行轻量化，以减少模型体积及加快模型推理速度。 | [CANN-Kit-tools-7.0.1.0](https://kiri-obs.obs.cn-north-4.myhuaweicloud.com/Cann%20Large%20Model%20Foundation%208.5.0.rc004/CANN-Kit-tools-7.0.1.0.run) |
| 平台插件包<br>包名：<br>Kirin9020 | Kirin9020内置AscendC算子包。 | [Kirin9020-plugin-next_7.0.1.0_linux-x86_64.run](https://kiri-obs.obs.cn-north-4.myhuaweicloud.com/Cann%20Large%20Model%20Foundation%208.5.0.rc004/kirin9020.run) |
| 平台插件包<br>包名：<br>KirinX90 | KirinX90内置AscendC算子包。 | [KirinX90-plugin-next_7.0.1.0_linux-x86_64.run](https://kiri-obs.obs.cn-north-4.myhuaweicloud.com/Cann%20Large%20Model%20Foundation%208.5.0.rc004/kirinx90.run) |
| 平台插件包<br>包名：<br>Kirin9030 | Kirin9030内置AscendC算子包。 | [Kirin9030-plugin-next_7.0.1.0_linux-x86_64.run](https://kiri-obs.obs.cn-north-4.myhuaweicloud.com/Cann%20Large%20Model%20Foundation%208.5.0.rc004/kirin9030.run) |
| 平台插件包<br>包名：<br>Kirin9050 | Kirin9050内置AscendC算子包。 | [Kirin9050-plugin-next_7.0.1.0_linux-x86_64.run](https://kiri-obs.obs.cn-north-4.myhuaweicloud.com/Cann%20Large%20Model%20Foundation%208.5.0.rc004/kirin9050.run) |

## CANN工具包安装流程
 
当前CANN工具包包含两个安装包：**基础包**`Ascend-cann-toolkit_9.2.0_linux-x86_64-mobile-station.run`与**扩展包**`CANN-Kit-tools-7.0.1.0.run`，需要先安装基础包，再安装扩展包。
 
1. **安装基础包**

 
   从[Tools下载](#tools下载)中下载`Ascend-cann-toolkit_9.2.0_linux-x86_64-mobile-station.run`。该run包直接安装即可，其提供了多个工具，包含ATC模型转换、AscendC算子开发、PyPTO算子开发及配套的mskl、msprof算子开发工具。安装方式如下：

   ```bash
   chmod +x Ascend-cann-toolkit_9.2.0_linux-x86_64-mobile-station.run
   # 安装命令
   ./Ascend-cann-toolkit_9.2.0_linux-x86_64-mobile-station.run --install --force --install-path=${install_path}
   ```

   ${install_path}：表示指定安装路径，默认安装在`/usr/local/Ascend`目录。

2. **安装扩展包**
 
   基础包安装完成后，再安装`CANN-Kit-tools-7.0.1.0.run`扩展包。
 
   ```bash
   chmod +x CANN-Kit-tools-7.0.1.0.run
   # 安装命令
   ./CANN-Kit-tools-7.0.1.0.run --full --install-path=${install_path}
   ```
 
   - ${install_path}：表示指定安装路径，需与基础包安装路径保持一致。
 
3. **算子插件包安装**

   若要使用对应平台的内置AscendC算子，需要单独下载算子run包，以Kirin9030为例，下载kirin9030.run包后，安装命令如下：

   ```bash
   chmod +x kirin9030.run
   ./kirin9030.run --install-path=${install_path}
   ```

   - ${install_path}：表示指定安装路径，默认安装在`/usr/local/Ascend`目录。

4. **配置环境变量**

   根据实际场景，选择合适的命令。

   ```bash
   # 默认路径安装，以root用户为例（非root用户，将/usr/local替换为${HOME}）
   source /usr/local/Ascend/cann-${cann_version}/set_env.sh
   # 指定路径安装
   # source ${install_path}/cann-${cann_version}/set_env.sh
   ```