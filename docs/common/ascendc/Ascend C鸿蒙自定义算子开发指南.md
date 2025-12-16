# AscendC自定义算子开发指南

[toc]

# 1、AscendC简介

## 概述

AscendC是CANN Kit针对算子开发场景推出的编程语言，遵循C和C++标准规范，匹配开发者开发习惯；通过多层接口抽象、自动并行计算、孪生调试等关键技术，提高算子开发效率，助力AI开发者低成本完成算子开发和模型调优部署。

![(zh-cn_image)](https://alliance-communityfile-drcn.dbankcdn.com/FileServer/getFile/cmtyPub/011/111/111/0000000000011111111.20251215183850.71020205189028050355819483523473:50001231000000:2800:432F5A5CCC99834545DCA96EFDB558262A0EC3089D478D90FC6A71E3C3DA852B.png)

使用AscendC进行自定义算子开发的突出优势有：

- 遵循C/C++编程规范，匹配开发者开发习惯

- 自动并行调度，获得最优执行性能

- 结构化核函数编程，简化算子开发逻辑

- CPU/NPU孪生调试，提升算子调试效率

开发者可以通过[AscendC主页](https://www.hiascend.com/zh/ascend-c)了解更详细的内容。


## 使用须知

当前AscendC支持的产品型号为：Kirin9020/KirinX90系列处理器产品。


# 2、环境准备

进行环境准备前，你需要了解如下基本概念，以便更好的理解后续操作。


- 开发环境：指编译开发代码的环境。

- 运行环境：指运行算子、推理程序等的Linux环境。运行环境必须连接上带有Kirin AI处理器的设备，如手机、平板等。

- 开发环境与运行环境合设场景：开发环境和运行环境在同一台机器上，开发者使用连接上Kirin AI处理器的机器作为运行环境，同时在该环境上进行代码开发与编译。

- 开发环境与运行环境分设场景：开发环境和运行环境不在同一台机器上，开发者使用连接上Kirin AI处理器的机器作为运行环境；使用其他独立机器进行代码开发与编译，作为开发环境。 
  > **说明：**
  >
  > 开发运行环境需要满足以下要求：
  >
  > ubuntu版本大于等于22.04，ubuntu架构为x86_64， python版本在3.7与3.10之间（包含），gcc/g++版本大于等于7.0。
  > 
  > - 设备连接与调试参考[hdc](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/hdc)。


进行自定义算子开发前，需要完成驱动及CANN软件包的安装，请参考[《CANN软件安装指南》](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/softwareinst/instg/instg_0000.html)完成环境准备。

1. 安装驱动固件（仅昇腾设备需要），安装步骤请参见“安装NPU驱动和固件”章节。
2. 安装CANN软件包，可参考“快速安装CANN”完成快速安装，可参考其他章节了解更多场景的安装步骤。
    > **说明：**
    > 
    > 安装CANN软件后，使用CANN运行用户进行编译、运行时，需要以CANN运行用户登录环境，执行source ${install_path}/set_env.sh命令设置环境变量。其中${install_path}为CANN软件的安装目录，例如：/usr/local/Ascend/ascend-toolkit。

安装cmake。使用AscendC算子工程([Kernel直调工程](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0051.html)和[自定义算子工程](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0058.html)）要求安装cmake(版本为3.16及以上)，如果版本不符合要求，可以参考如下示例安装满足要求的版本。
示例：安装3.16.0版本的cmake（x86_64架构）。
```shell
mkdir -p cmake-3.16 && wget -qO- "https://cmake.org/files/v3.16/cmake-3.16.0-Linux-x86_64.tar.gz" | tar --strip-components=1 -xz -C cmake-3.16
export PATH=`pwd`/cmake-3.16/bin:$PATH
```

> **说明：**
    > 
    > 对于Ascend C算子的开发，并非必须安装驱动固件。在非昇腾设备上，可以利用CPU仿真环境先行进行算子开发和测试，并在准备就绪后，利用昇腾设备进行加速计算。非昇腾设备的安装请参考[《CANN软件安装指南》](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/softwareinst/instg/instg_0000.html)中“附录B：常用操作 > 在非昇腾设备上安装CANN”章节。



# 3、快速入门

本节以一个简单算子为例，带开发者体验从算子工程创建、代码编写、编译部署到运行验证的开发全流程，让开发者对算子开发工程有个宏观的认识，此处我们以输入是动态shape的Add算子实现为例，为了与内置Add算子区分，定义算子类型为AddCustom。


## 工程创建

CANN软件包中提供了工程创建工具msOpGen，开发者可以输入算子原型定义文件生成AscendC算子开发工程。

1. 编写AddCustom算子的原型定义json文件。
   假设AddCustom算子的原型定义文件命名为add_custom.json，存储路径为: $HOME/sample，文件内容如下。

    ```json
    [ 
        { 
            "op": "AddCustom", 
            "input_desc": [ 
                { 
                    "name": "x", 
                    "param_type": "required", 
                    "format": [ 
                        "ND" 
                    ], 
                    "type": [ 
                        "fp16" 
                    ] 
                }, 
                { 
                    "name": "y", 
                    "param_type": "required", 
                    "format": [ 
                        "ND" 
                    ], 
                    "type": [ 
                        "fp16" 
                    ] 
                } 
            ], 
            "output_desc": [ 
                { 
                    "name": "z", 
                    "param_type": "required", 
                    "format": [ 
                        "ND" 
                    ], 
                    "type": [ 
                        "fp16" 
                    ] 
                } 
            ] 
        } 
    ]
    ```

2. 注意先设置环境变量，执行**source ${install_path}/ddk/** **tools/tools_ascendc/set_ascendc_env.sh**命令，其中${install_path}为tools包的解压目录。

3. 使用msOpGen工具生成AddCustom算子的开发工程。
    ```shell
    msopgen gen -i $HOME/sample/add_custom.json -c ai_core-<soc_version> -out   $HOME/sample/AddCustom
    ```

    - -i：算子原型定义文件add_custom.json所在路径。
    - -c：ai_core-_&lt;soc_version&gt;_代表算子在AI Core上执行，_&lt;soc_version&gt;_为Kirin AI处理器的型号，可在运行环境通过命令进行查询:



        ```shell
        hdc -t ${target} shell param get ohos.boot.chiptype
        ```

        target：设备的SN码，可以通过hdc list targets获取当前运行环境上所有设备的SN码。

    样例：
    ```
    msopgen gen -i ./add_custom.json -c ai_core-kirin9020 -out ./AddCustom    
    ```
    
    基于同系列的AI处理器型号创建的算子工程，其基础能力通用。命令执行完后，会在$HOME/sample目录下生成算子工程目录AddCustom，工程中包含算子实现的模板文件，编译脚本等，如下所示。
    ```text
    AddCustom 
    ├── build_devices.sh // 开发者无需关注，在线编译场景预留，编译device侧交付件脚本
    ├── build.sh         // 编译入口脚本 
    ├── cmake  
    │   ├── config.cmake 
    │   ├── util        // 算子工程编译所需脚本及公共编译文件存放目录 
    ├── CMakeLists.txt   // 算子工程的CMakeLists.txt 
    ├── CMakePresets.json // 编译配置项 
    ├── framework        // 算子插件实现文件目录，单算子模型文件的生成不依赖算子适配插件，无需关注 
    ├── op_host                      // host侧实现文件 
    │   ├── add_custom_tiling.h    // 算子tiling定义文件 
    │   ├── add_custom.cpp         // 算子原型注册、shape推导、信息库、tiling实现等内容文件 
    │   ├── CMakeLists.txt 
    ├── op_kernel                   // kernel侧实现文件 
    │   ├── CMakeLists.txt    
    │   ├── add_custom.cpp        // 算子核函数实现文件  
    ├── scripts                     // 自定义算子工程打包相关脚本所在目录
    ```

    > **说明：**
    >
    > 上述目录结构中的粗体文件op_host/add_custom_tiling.h、op_host/add_custom.cpp、op_kernel/add_custom.cpp为后续算子开发过程中需要修改的文件，其他文件无需修改。
    

## 算子核函数实现

在工程存储目录的"AddCustom/op_kernel/add_custom.cpp"文件中实现算子的核函数，完整的样例代码开发者可以在[add_custom.cpp](https://gitcode.com/HarmonyOS_Samples/cannkit_samplecode_add_custom_cpp/blob/master/FrameworkLaunch/AddCustom/op_kernel/add_custom.cpp)中查看，下面介绍关键实现代码。

算子核函数实现代码的内部调用关系示意图如下。

**图1** 核函数调用关系图  
![](https://alliance-communityfile-drcn.dbankcdn.com/FileServer/getFile/cmtyPub/011/111/111/0000000000011111111.20251215183857.20013491577714889222610497743724:50001231000000:2800:16CAFA7B2C254E688B316520F2E79EC6B8A2CD3E8D46CF4E3165A1A28842DDE4.png)

由此可见除了Init函数完成初始化外，Process中完成了对流水任务 **：** 搬入、计算、搬出的调用，开发者可以重点关注三个流水任务的实现。

1. 进行**核函数的定义，** 并在核函数中调用算子类的Init和Process函数。
    ```cpp
    extern "C" __global__ __aicore__ void add_custom(GM_ADDR x, GM_ADDR y, GM_ADDR z, GM_ADDR workspace, GM_ADDR tiling) 
    { 
        // 获取Host侧传入的Tiling参数 
        GET_TILING_DATA(tiling_data, tiling); 
        // 初始化算子类 
        KernelAdd op; 
        // 算子类的初始化函数，完成内存初始化相关工作 
        op.Init(x, y, z, tiling_data.totalLength, tiling_data.tileNum); 
        // 完成算子实现的核心逻辑 
        op.Process(); 
    }
    ```

2. 定义KernelAdd算子类，其具体成员及成员函数实现如下。
    ```cpp
    #include "kernel_operator.h" 
    constexpr int32_t BUFFER_NUM = 2; 
    class KernelAdd { 
    public: 
        __aicore__ inline KernelAdd() {} 
        // 初始化函数，完成内存初始化相关操作 
        __aicore__ inline void Init(GM_ADDR x, GM_ADDR y, GM_ADDR z, uint32_t totalLength, uint32_t tileNum) 
        { 
            // 使用获取到的TilingData计算得到singleCoreSize(每个核上总计算数据大小)、tileNum（每个核上分块个数）、singleTileLength（每个分块大小）等变量 
            this->blockLength = totalLength / AscendC::GetBlockNum(); 
            this->tileNum = tileNum; 
            this->tileLength = this->blockLength / tileNum / BUFFER_NUM; 
            
            // 获取当前核的起始索引 
            xGm.SetGlobalBuffer((__gm__ DTYPE_X*)x + this->blockLength * AscendC::GetBlockIdx(), this->blockLength); 
            yGm.SetGlobalBuffer((__gm__ DTYPE_Y*)y + this->blockLength * AscendC::GetBlockIdx(), this->blockLength); 
            zGm.SetGlobalBuffer((__gm__ DTYPE_Z*)z + this->blockLength * AscendC::GetBlockIdx(), this->blockLength); 
            // 通过Pipe内存管理对象为输入输出Queue分配内存 
            pipe.InitBuffer(inQueueX, BUFFER_NUM, this->tileLength * sizeof(DTYPE_X)); 
            pipe.InitBuffer(inQueueY, BUFFER_NUM, this->tileLength * sizeof(DTYPE_Y)); 
            pipe.InitBuffer(outQueueZ, BUFFER_NUM, this->tileLength * sizeof(DTYPE_Z)); 
        } 
        // 核心处理函数，实现算子逻辑，调用私有成员函数CopyIn、Compute、CopyOut完成矢量算子的三级流水操作 
        __aicore__ inline void Process() 
        { 
            int32_t loopCount = this->tileNum * BUFFER_NUM; 
            for (int32_t i = 0; i < loopCount; i++) { 
                CopyIn(i); 
                Compute(i); 
                CopyOut(i); 
            } 
        } 
    
    
    private: 
        // 搬入函数，完成CopyIn阶段的处理，被核心Process函数调用 
        __aicore__ inline void CopyIn(int32_t progress) 
        { 
            // 从Queue中分配输入Tensor 
            AscendC::LocalTensor<DTYPE_X> xLocal = inQueueX.AllocTensor<DTYPE_X>(); 
            AscendC::LocalTensor<DTYPE_Y> yLocal = inQueueY.AllocTensor<DTYPE_Y>(); 
            // 将GlobalTensor数据拷贝到LocalTensor 
            AscendC::DataCopy(xLocal, xGm[progress * this->tileLength], this->tileLength); 
            AscendC::DataCopy(yLocal, yGm[progress * this->tileLength], this->tileLength); 
            // 将LocalTensor放入VECIN（代表矢量编程中搬入数据的逻辑存放位置）的Queue中 
            inQueueX.EnQue(xLocal); 
            inQueueY.EnQue(yLocal); 
        } 
        // 计算函数，完成Compute阶段的处理，被核心Process函数调用 
        __aicore__ inline void Compute(int32_t progress) 
        { 
            // 将Tensor从队列中取出，用于后续计算 
            AscendC::LocalTensor<DTYPE_X> xLocal = inQueueX.DeQue<DTYPE_X>(); 
            AscendC::LocalTensor<DTYPE_Y> yLocal = inQueueY.DeQue<DTYPE_Y>(); 
            // 从Queue中分配输出Tensor 
            AscendC::LocalTensor<DTYPE_Z> zLocal = outQueueZ.AllocTensor<DTYPE_Z>(); 
            // 调用Add接口进行计算 
            AscendC::Add(zLocal, xLocal, yLocal, this->tileLength); 
            // 将计算结果LocalTensor放入到VecOut的Queue中 
            outQueueZ.EnQue<DTYPE_Z>(zLocal); 
            // 释放输入Tensor 
            inQueueX.FreeTensor(xLocal); 
            inQueueY.FreeTensor(yLocal); 
        } 
        // 搬出函数，完成CopyOut阶段的处理，被核心Process函数调用 
        __aicore__ inline void CopyOut(int32_t progress) 
        { 
            // 从VecOut的Queue中取出输出Tensor 
            AscendC::LocalTensor<DTYPE_Z> zLocal = outQueueZ.DeQue<DTYPE_Z>(); 
            // 将输出Tensor拷贝到GlobalTensor中 
            AscendC::DataCopy(zGm[progress * this->tileLength], zLocal, this->tileLength); 
            // 将不再使用的LocalTensor释放 
            outQueueZ.FreeTensor(zLocal); 
        } 
    
    
    private: 
        //Pipe内存管理对象 
        AscendC::TPipe pipe; 
        //输入数据Queue队列管理对象，QuePosition为VECIN 
        AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX, inQueueY;  
        //输出数据Queue队列管理对象，QuePosition为VECOUT 
        AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueZ; 
        //管理输入输出Global Memory内存地址的对象，其中xGm, yGm为输入，zGm为输出 
        AscendC::GlobalTensor<DTYPE_X> xGm; 
        AscendC::GlobalTensor<DTYPE_Y> yGm; 
        AscendC::GlobalTensor<DTYPE_Z> zGm; 
        // 每个核上总计算数据大小 
        uint32_t blockLength; 
        // 每个核上总计算数据分块个数 
        uint32_t tileNum; 
        // 每个分块大小 
        uint32_t tileLength; 
    };
    ```


## Host侧算子实现

核函数开发并验证完成后，下一步就是进行Host侧的实现，对应“AddCustom/op_host”目录下的add_custom_tiling.h文件与add_custom.cpp文件。下面简要介绍下两个文件的关键实现，完整的样例代码可参见[add_custom_tiling.h](https://gitcode.com/HarmonyOS_Samples/cannkit_samplecode_add_custom_cpp/blob/master/FrameworkLaunch/AddCustom/op_host/add_custom_tiling.h)与[add_custom.cpp](https://gitcode.com/HarmonyOS_Samples/cannkit_samplecode_add_custom_cpp/blob/master/FrameworkLaunch/AddCustom/op_host/add_custom.cpp)。

1. 修改“add_custom_tiling.h”文件，在此文件中增加粗体部分的代码，进行Tiling参数的定义。
    ```cpp
    #ifndef ADD_CUSTOM_TILING_H 
    #define ADD_CUSTOM_TILING_H 
    #include "register/tilingdata_base.h" 
    namespace optiling { 
    BEGIN_TILING_DATA_DEF(AddCustomTilingData) 
    // AddCustom算子使用了2个tiling参数：totalLength与tileNum 
    TILING_DATA_FIELD_DEF(uint32_t, totalLength);     // 总计算数据量 
    TILING_DATA_FIELD_DEF(uint32_t, tileNum);         // 每个核上总计算数据分块个数 
    END_TILING_DATA_DEF; 
    
    // 注册tiling数据到对应的算子 
    REGISTER_TILING_DATA_CLASS(AddCustom, AddCustomTilingData) 
    } 
    #endif // ADD_CUSTOM_TILING_H
    ```

2. 修改“add_custom.cpp”文件，进行Tiling的实现。
   修改“TilingFunc”函数，实现Tiling上下文的获取，并通过上下文获取输入输出shape信息，并根据shape信息设置TilingData，序列化保存TilingData，并设置TilingKey。

    ```cpp
    namespace optiling { 
    const uint32_t BLOCK_DIM = 1; 
    const uint32_t TILE_NUM = 8; 
    static ge::graphStatus TilingFunc(gert::TilingContext* context) 
    { 
        AddCustomTilingData tiling; 
        uint32_t totalLength = context->GetInputShape(0)->GetOriginShape().GetShapeSize(); 
        context->SetBlockDim(BLOCK_DIM); 
        tiling.set_totalLength(totalLength); 
        tiling.set_tileNum(TILE_NUM); 
        tiling.SaveToBuffer(context->GetRawTilingData()->GetData(), context->GetRawTilingData()->GetCapacity()); 
        context->GetRawTilingData()->SetDataSize(tiling.GetDataSize()); 
        size_t *currentWorkspace = context->GetWorkspaceSizes(1); 
        currentWorkspace[0] = 0; 
        return ge::GRAPH_SUCCESS; 
    } 
    } // namespace optiling
    ```
    
3. 在“add_custom.cpp”文件中实现AddCustom算子的shape推导。
   Add算子的输出shape等于输入shape，所以直接将输入shape赋给输出shape，当前msOpGen工具生成的代码“InferShape”函数无需修改。
    
4. 修改“add_custom.cpp”文件中的算子原型注册，此函数为入口函数。
    ```cpp
    namespace ops { 
    class AddCustom : public OpDef { 
    public: 
        explicit AddCustom(const char* name) : OpDef(name) 
        {  
            // Add算子的第一个输入 
            this->Input("x") 
                .ParamType(REQUIRED)    // 代表输入必选 
                .DataType({ ge::DT_FLOAT16, ge::DT_FLOAT, ge::DT_INT32 })   // 输入支持的数据类型 
                .Format({ ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND });   // 输入支持的数据格式 
            // Add算子的第二个输入 
            this->Input("y") 
                .ParamType(REQUIRED) 
                .DataType({ ge::DT_FLOAT16, ge::DT_FLOAT, ge::DT_INT32 }) 
                .Format({ ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND }); 
            this->Output("z") 
                .ParamType(REQUIRED) 
                .DataType({ ge::DT_FLOAT16, ge::DT_FLOAT, ge::DT_INT32 }) 
                .Format({ ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND }); 
            // 关联InferShape函数 
            this->SetInferShape(ge::InferShape); 
            // 关联Tiling函数 
            this->AICore() 
                .SetTiling(optiling::TilingFunc); 
            // 注册算子支持的AI处理器型号，请替换为实际支持的AI处理器型号,如kirin9020 
            this->AICore().AddConfig("kirinxxx"); 
        } 
    }; 
    // 结束算子注册 
    OP_ADD(AddCustom); 
    } // namespace ops
    ```


## 算子工程编译部署

编译AddCustom工程，生成自定义算子安装包，并将其安装到算子库中。

1. 编译自定义算子工程，构建生成自定义算子包。
    在算子工程AddCustom目录下执行如下命令，进行算子工程编译。
    ```shell
    ./build.sh
    ```
    
    编译成功后，会在当前目录下创建build_out目录，在build_out/autogen目录下生成自定义算子交付件。
    
2. 自定义算子安装包部署。
   在执行编译的同时，会将交付件安装到DDK安装目录${DDK_INSTALL_PATH}下的指定目录。

    ```shell
    ${DDK_INSTALL_PATH}/tools/platform
    ```
    
    查看部署后的目录结构，如下所示：

    ```text
    platform                            // 平台插件目录
    ├── kirin9020                       // Kirin AI处理器类型
    │   ├── config
    │   │   └── npu_custom_opinfo.json  // 算子信息库
    │   ├── lib64
    │   │   └── libcustom_op.so         // host侧二进制文件
    │   ├── ops
    │   │   └── impl
    │   │       ├── custom              // kernel交付件
    │   │       │   ├── add_custom.cpp
    │   │       │   ├── add_custom.py
    │   │       │   └── op_proto.h
    │   │       └── impl
    │   └── simulator
    └── README.md
    ```



# 4、算子调试调优

## 4.1、算子调试概述

### 工具介绍


#### 使用说明

介绍AscendC算子调测工具的调测流程和调测功能等，并提供命令行方式，以指导开发者在不同场景下对AscendC算子发起Tiling、精度、性能调测。

**表1** 工具指引

| 使用场景 | 操作指引 | 说明 |
| -------- | -------- | -------- |
| AscendC算子调测工具是什么 | [调测工具简介](#43调测功能介绍) | 阐述了工具的基本概念、关键调测功能、调测方式和流程等，帮助开发者快速了解和使用工具。 |
| 算子调测过程中涉及哪些配置和调测功能 | [数据准备与配置说明](#42数据准备和配置说明) | 阐述命令行方式下算子调测需要的**输入/标杆数据**、**算子信息json文件配置**等。 |


#### 使用约束

仅支持对静态Shape算子调测，暂不支持动态Shape算子调测。


### 基本概念

**表2** 基本概念

| 概念 | 描述 |
| -------- | -------- |
| 标准自定义算子工程场景(customize) | 该场景下，开发者需按照“工程创建-&gt;算子实现-&gt;编译部署-&gt;算子调用”的流程完成算子开发和调用。基于工程代码框架完成算子核函数的开发和Tiling实现，通过工程编译脚本完成算子编译部署。Tiling开发基于CANN提供的编程框架进行，后续更易于借助框架功能实现单算子API调用、图模式算子调用等。 |
| 输入数据和标杆数据 | 使用AscendC调测工具时，需要提供算子输入数据及标杆数据（Golden数据）。<br/>- 输入数据：固定shape算子运行时的输入数据，bin格式。<br/>- 标杆数据：根据输入数据计算出来的真值数据，用于与输出数据进行精度比对，bin格式。 |
| 算子json配置文件 | 用于描述算子的输入、输出及属性信息，以json格式存储。该文件中的参数个数、顺序、数据类型等信息，必须与算子原型相匹配。<br/>工具支持以下配置格式：<br/>[固定输入/输出顺序的算子json配置](#423固定输入输出顺序的算子json配置)：其输入/输出参数信息单独在inputs、outputs、attrs配置项中设置，不支持输入/输出参数交叉设置。 |
| Tiling | 描述Kirin AI处理器上算子的输入/输出数据切分、分块计算、多核并行等策略，主要是为了满足片上存储限制和计算pipeline的需求，最大化计算并行性和数据局部性（data locality 或 data reuse），从而发挥硬件的极致性能。 |
| CAModel | CAModel(Cycle Accurate Model)仿真器主要用于生成算子性能仿真流水数据。 |
| CCEC | CCEC(Cube-based Computing Engine Compiler)工具基于毕昇编译器，通过自动拼接编译选项构建的一种编译方式。其中，毕昇编译器是一款专为达芬奇SOC设计的编译器，支持异构编程扩展，可将开发者编写的昇腾算子代码编译成二进制可执行文件和动态库等形式，详细介绍请参考《[毕昇编译器使用指南](https://www.hiascend.com/document/detail/zh/canncommercial/82RC1/index/index.html)》。 |
| CCEprint/Intrilog | intrilog：落盘算子运行中的CCE指令流。 |
| 原地算子(in-place op) | 一般指原地更新操作类算子，即算子的输入和输出为同一地址，算子在计算完成后，把原有的输出结果直接覆盖在输入的地址上，以减少不必要的内存占用。 |

输入数据和标杆数据可通过numpy生成，样例如下。
```python
import numpy as np
def gen_data_simple(): 
    input_x = np.random.uniform(-100, 100, [8, 2048]).astype(np.float16)
    input_y = np.random.uniform(-100, 100, [8, 2048]).astype(np.float16)
    golden = (input_x + input_y).astype(np.float16)
    input_x.tofile("./input/input_x.bin")     # 通过numpy生成的随机输入数据x
    input_y.tofile("./input/input_y.bin")     # 通过numpy生成的随机输入数据y
    golden.tofile("./output/golden.bin")      # 根据输入计算出来的真值数据，即标杆数据
```


### 运行流程

若采用工具命令行方式对单算子调测，其运行流程如图1所示。

**图1** 基于命令行的工具运行流程
![alt text](https://alliance-communityfile-drcn.dbankcdn.com/FileServer/getFile/cmtyPub/011/111/111/0000000000011111111.20251215183900.85214262072511646947492832791648:50001231000000:2800:F636DDAA4EA2BED5A4C6C9FBA236953F8EAA2AF4681797F38A0113B32B51417C.png)

1. 准备环境：使用本工具进行算子调测前，请先完成基础环境搭建，详细操作参见[环境准备](#2环境准备)。

2. 准备数据：准备好bin格式的输入数据和标杆数据文件，该数据是调测工具的必要输入。

3. 配置json文件：根据工具提供的算子信息json配置样例配置待测算子信息，该文件是调测工具的必要输入。

4. 通过命令行方式发起算子调测。
    目前工具支持的算子调测能力如下，请按需选择：

    - Tiling调测：对Tiling函数调测，生成Tiling结果，验证Tiling函数功能。
    - 精度调试：通过CPU仿真或者simulator仿真进行核函数精度调试，支持自动精度比对、printf/PRINTF/DumpTensor/assert打印等功能。
    - 性能调优：通过simulator仿真获取算子执行circle数，评估算子性能。

## 4.2、数据准备和配置说明

### 4.2.1、输入数据和标杆数据准备

使用AscendC调测工具进行算子调测前，必须提供算子的[输入数据和标杆数据](#421输入数据和标杆数据准备)。


- 输入数据（input数据）：固定shape算子运行时的输入数据，bin格式。

- 标杆数据（golden数据）：根据输入数据计算出来的真值数据，用于与输出数据进行精度比对，bin格式。


本章节提供多种方式来准备数据，开发者可以根据实际情况选择合适的方式。


- **方式1：采用外部提供的input/golden数据。**
    开发者采用外部数据源进行算子调测，一般适合首次调测场景。
    
    在算子信息json配置文件中，将gen_data配为false，data_file配为输入数据、标杆数据对应的路径（必须为绝对路径），示例如下。
    
    ```json
    {
        "gen_data": false,
        "inputs": [
            {
                "data_file": "/path/to/input_data_q.bin"
            }
        ],
        "outputs": [
            {
                "data_file": "/path/to/golden_data_out.bin"
            }
        ]
    }
    ```
    
    若外部提供的数据是.pt文件，需要转换为.bin文件，以PyTorch为例：
    
    ```python
    import torch 
    model = torch.load('model.pt')   # 加载.pt文件
    torch.save(model, 'model.bin')   # 将模型参数保存为二进制文件
    ```
    
- **方式2：采用已生成过的input/golden数据，无需重新生成。**
    当开发者需要多次重跑，建议直接使用历史生成的数据，避免重复生成，导致调测耗时过长。
    
    在算子信息json配置文件中，将gen_data配为false，data_file配为输入数据、标杆数据对应的bin文件名（不含路径信息，默认在当前路径下查找数据文件），示例如下。
    
    ```json
    {
        "gen_data": false,
        "inputs": [
            {
                "data_file": "input_data_q.bin"
            }
        ],
        "outputs": [
            {
                "data_file": "golden_data_out.bin"
            }
        ]
    }
    ```
    
- **方式3：采用工具随机生成input数据。**
    开发者未准备任何数据，可采用工具随机生成的input数据，生成的bin数据文件默认在当前路径下的对应算子的data文件夹中。该场景下没有golden数据，因此不支持精度比对。
    
    在算子信息json配置文件中，将gen_data配为true，data_script配为空字符串或null，data_file配为输入数据、标杆数据对应的bin文件名（不含路径信息，默认在当前路径下查找数据文件），示例如下。
    
    ```json
    "data_script": "",       
    "gen_data": true,
     // ... 
    "shape": [1, 16384],
    "data_file": "sample.bin",
    ```
    
- **方式4：采用脚本生成input/golden数据。**
    开发者自行准备数据生成脚本（一般是.py文件），同时配置算子信息json配置文件。
    
    将gen_data配为true，data_script配为数据生成脚本路径（以FlashAttentionScore算子为例），data_file配为输入数据、标杆数据对应的bin文件名（不含路径信息，默认在当前路径下查找数据文件），示例如下。
    
    ```json
    "data_script": "${fa_case_dir}/flash_attention_score_golden.py",       
    "gen_data": true,
     // ... 
    "data_file": "sample.bin",
    ```
    
    生成数据时，会调用该脚本，并固定传入以下参数：
    
    sys.argv[1]：输入的算子json配置文件。
    
    sys.argv[2]：输出bin文件路径（工作路径下的data文件夹，例如debug_workspace/FlashAttentionScore/data/）。

    - 开发者需要根据输入的sys.argv[1]读取算子json配置文件内容，获取生成数据所需的算子shape信息、数据文件的文件名，并在脚本中适配生成的文件名。
    - 开发者需要根据输入的sys.argv[2]设置存放input/output数据文件的路径，将生成的文件保存到该路径下。

### 4.2.2、算子json配置模板获取

#### 标准自定义算子工程场景

本场景一般通过工具一键生成对应的算子json配置模板，命令行示例如下。

```shell
ascendebug json convert --binary-op-json ${custom_code_path}/build_out/op_kernel/binary/${chip_version}/gen/${op_type}_${hash}_param.json --converted-json ${op_config_json_file}
```

- --binary-op-json：指标准自定义算子工程中的算子信息库json文件。其中${custom_code_path} 表示标准自定义算子工程代码根目录，${chip_version}表示芯片类型，${op_type}表示算子名，${hash}表示根据算子定义生成的hash值。

- --converted-json：指生成的算子信息json配置模板。

该算子json配置模板默认采用**固定输入/输出顺序**格式，具体格式和参数的介绍请参见[固定输入/输出顺序的算子json配置](#423固定输入输出顺序的算子json配置)。

### 4.2.3、固定输入/输出顺序的算子json配置

#### json配置说明

固定输入/输出顺序的算子json配置文件中“inputs”和“outputs”参数按照“**输入/输出规则排布**”，所以要求Kernel入口函数的参数也是按照该规则排布。

> **说明：**
>
> - **输入/输出规则排布**（所有输出参数排布在输入参数之后）：例如Kernel入口函数的参数排布为
>   ```cpp
>   extern "C" __global__ __aicore__ void add_custom(GM_ADDR input1, GM_ADDR input2, GM_ADDR output)
>   ```
>
> - **标准自定义算子工程场景**的开发人员一般按此方式配置算子json文件。

以AddCustom算子为例，对应的json配置示例如下，参数说明参见表1。

```shell
{ 
    "op_type": "AddCustom", 
    "data_script": "./add_golden.py", 
    "gen_data": true, 
    "inputs": [ 
        { 
            "name": "x", 
            "dtype": "int32", 
            "format": "ND", 
            "ignore": false, 
            "shape": [32], 
            "param_type": "required", 
            "data_file": "x.bin" 
        }, 
        { 
            "name": "y", 
            "dtype": "int32", 
            "format": "ND", 
            "ignore": false, 
            "shape": [32], 
            "param_type": "required", 
            "data_file": "y.bin" 
        } 
    ], 
    "outputs": [ 
        { 
            "name": "z", 
            "dtype": "int32", 
            "format": "ND", 
            "ignore": false, 
            "shape": [32], 
            "param_type": "required", 
            "data_file": "golden_z.bin" 
        } 
    ], 
    "attrs": [ 
        { 
            "name": "mask", 
            "dtype": "list_int", 
            "value": [0,0] 
        }, 
        { 
            "name": "repeatTimes", 
            "dtype": "int", 
            "value": 1 
        }, 
        // ...
    ] 
}
```

**表1** 固定输入/输出顺序的算子json全量参数说明

| 配置项 | 数据类型 | 参数说明 | 取值说明 | 是否必选 |
| -------- | -------- | -------- | -------- | -------- |
| op_type | string | 算子名。 | 与待调测算子严格匹配。 | 是 |
| data_script | string | 数据生成脚本（python），用于生成输入和标杆数据。 | 根据实际情况设置，如"/home/flash_attention_golden.py"。<br/>**说明：** 若无数据生成脚本，填写空字符或null。 | 否 |
| gen_data | bool | 是否根据data_script生成输入和标杆数据。 | - true：采用脚本生成数据。<br/>- false：不采用脚本生成数据，默认false。 | 是 |
| inputs /<br/>outputs | name | string | 核函数输入/输出的参数名。 | 根据实际情况设置（若通过算子二进制模板文件生成，不建议修改）。 | 是 |
| inputs /<br/>outputs | dtype | string | 输入/输出的数据类型。 | 目前支持bool、int、int8、int16、int32、int64、uint8、uint16、uint32、uint64、float16、float32、float64、bfloat16。<br/>**说明：** 算子json配置时dtype仅允许输入一种数据类型，不支持多种数据类型。 | 是 |
| inputs /<br/>outputs | format | string | 输入/输出的存储格式。 | 支持的数据格式有ND、NZ。 | 是 |
| inputs /<br/>outputs | shape | list | 输入/输出的shape。 | 根据算子实际shape填写，例如[24, 20, 144, 8]。<br/>**说明：** 当输入为Scalar时，shape填null。 | 是 |
| inputs /<br/>outputs | ignore | bool | 是否忽略该输入/输出。 | - true：表示输入/输出可忽略，不分配Global Memory内存。<br/>- false：表示输入/输出不可忽略，默认false。 | 否 |
| inputs /<br/>outputs | param_type | string | 是否必选该输入/输出。 | - required：必选。<br/>- optional：可选。<br/>**说明：** 当param_type为optional时<br/>- ignore为false视为没有该输入。<br/>- ignore为true视为存在该输入。<br/>- 不支持中间有悬空输入。例如，存在3个输入x, y, z时，不允许ignore的情况为false, true, false。 | 是 |
| inputs /<br/>outputs | data_file | string | - inputs场景下，指输入数据bin文件。<br/>- outputs场景下，指标杆数据bin文件。 | 根据实际情况设置数据bin文件路径，必须为绝对路径，例如"/home/data.bin"。<br/>**说明：**<br/>- 当data_file设为空字符或null，表示不对运行输出作精度比对。<br/>- 当输入为Scalar时，data_file字段删除，只需配置data_value。 | 否 |
| inputs /<br/>outputs | data_value | 由dtype确定 | 输入的Scalar值。 | 根据实际情况填写。<br/>**说明：**<br/>- 只有input节点才可以配置该参数。<br/>- data_value与data_file互斥。若配置data_value则data_file必须删除，且shape必须为null，表示本节点是Scalar输入。 | 否 |
| inputs /<br/>outputs | value_depend | string | 输入是否为Const类型 | 该配置项可选，不配置该字段表示输入不为Const类型。<br/>- true：输入为Const类型。<br/>- false：输入不为Const类型。 |  |
| attrs | name | string | 算子的属性名，是区分每个算子的唯一标识，不可重复。 | 部分场景允许没有attrs，请根据实际情况填写。 | 是 |
| attrs | dtype | string | 数据类型。 | 部分场景允许没有attrs，请根据实际情况填写。 | 是 |
| attrs | value | 可变 | 属性值。 | 部分场景允许没有attrs，请根据实际情况填写。 | 是 |


#### 特殊格式输入

- **场景1：支持Scalar格式的输入。**
    当输入为Scalar格式，json中“inputs”配置项中删除data_file，shape配为null，data_value配为指定的标量值。

    ```json
    { 
        "op_type": "xxxx", 
        "data_script": "", 
        "gen_data": false, 
        "inputs": [{ 
            "name": "tileNumIn", 
            "dtype": "uint32", 
            "param_type": "required", 
                // ...
            "shape": null,
            "data_value": 8
            }] 
            // ... 
    }
    ```

- **场景2：支持TensorList格式的输入。**
    当输入为TensorList格式，该参数需要用[ ]表示，List中的每一项表示一个Tensor，示例如下。

    ```json
    { 
        "op_type": "xxxx", 
        "data_script": "", 
        "gen_data": false, 
        "inputs":   
            [{
                "name": "key0",
                "dtype": "float16",
                "format": "ND",
                "ignore": false,
                "shape": [1,5,8192,128],
                "param_type": "required",
                "data_file": "/home/data/input/k_0.bin"
            },{
                "name": "key1",
                "dtype": "float16",
                "format": "ND",
                "ignore": false,
                "shape": [1,5,8192,128],
                "param_type": "required",
                "data_file": "/home/data/input/k_1.bin"
       }]
   // ... 
    }
    ```
    
- **场景3：支持原地算子格式的输入。**
  > **说明：**
  >
  > 暂不支持该方式设置输入。
    
    当算子为[原地算子(in-place op)](#41算子调试概述)时，输入和输出地址一样，配置算子json时“outputs”和“inputs”除了“data_file”不同，其他配置项均保持一致。配置示例如下。
    
    - "inputs"中data_file：输入数据为x.bin。
    - "outputs"中data_file：输出数据为标杆数据golden_x.bin。

    ```json
    { 
        "op_type": "AddCustom", 
        "data_script": "./add_golden.py", 
        "gen_data": true, 
        "inputs": [ 
            { 
                "name": "x",
                "dtype": "int32",
                "format": "ND",
                "ignore": false,
                "shape": [32],
                "param_type": "required",
                "data_file": "x.bin"
            }, 
            { 
                "name": "y", 
                "dtype": "int32", 
                "format": "ND", 
                "ignore": false, 
                "shape": [32], 
                "param_type": "required", 
                "data_file": "y.bin" 
            } 
        ], 
        "outputs": [ 
            { 
                "name": "x", 
                "dtype": "int32", 
                "format": "ND", 
                "ignore": false, 
                "shape": [32],
                "param_type": "required",
                "data_file": "golden_x.bin"
            } 
        ], 
        "attrs": [ 
            { 
                "name": "mask", 
                "dtype": "list_int", 
                "value": [0,0] 
            }, 
            { 
                "name": "repeatTimes", 
                "dtype": "int", 
                "value": 1 
            }, 
            // ... 
        ] 
    }
    ```

### 4.2.4、params归一格式的算子json配置

#### json配置说明

为了支持输入/输出参数交叉配置的场景，params归一配置格式应运而生，所有输入/输出参数均放在“params”配置项中。该算子json配置文件中参数可以按**输入/输出规则排布**，也可以按**输入/输出交叉排布**，只要保证参数顺序与Kernel入口函数的参数顺序保持一致即可。

> **说明：**
>
> - **调试工具暂不支持该配置**。
>
> - **输入/输出规则排布**（所有输出参数排布在输入参数之后）：例如Kernel入口函数的参数排布为extern "C" __global__ __aicore__ void add_custom(GM_ADDR input1, GM_ADDR input2, GM_ADDR  input3,** GM_ADDR output**)。
>
> - **输入/输出交叉排布**（输入/输出参数排布顺序有交叉）：例如Kernel入口函数的参数排布为extern "C" __global__ __aicore__ void add_custom(GM_ADDR input1, GM_ADDR input2, **GM_ADDR output**, GM_ADDR input3)，output参数排在input3之前。
>
> - **核函数直调工程场景**的开发人员一般按此方式配置。

以AddCustom算子为例，对应的json配置示例如下，参数说明参见表1。

```json
{ 
    "op_type": "add_custom", 
    "data_script": "", 
    "gen_data": false, 
    "params": [{ 
                "name": "x", 
                "dtype": "float16", 
                "param_type": "input", 
                "shape": [1,16384], 
                "data_file": "x.bin" 
            }, 
            { 
                "name": "z", 
                "dtype": "float16", 
                "param_type": "output", 
                "shape": [1,16384], 
                "data_file": "golden_z.bin" 
            }, 
            { 
                "name": "y", 
                "dtype": "float16", 
                "param_type": "input", 
                "shape": [1,16384], 
                "data_file": "y.bin" 
            }, 
    ], 
    "kernel_info": { 
        "kernel_source": "add_custom.cpp", 
        "kernel_name": "add_custom", 
        "kernel_includes": [] 
    } 
}
```

**表1** params归一格式的算子json全量参数说明

| 参数名 | 数据类型 | 参数说明 | 取值说明 | 是否必选 |
| -------- | -------- | -------- | -------- | -------- |
| op_type | string | 算子名。 | 与待调测算子严格匹配。 | 是 |
| data_script | string | 数据生成脚本(python)，用于生成输入和标杆数据。 | 根据实际情况设置，如"/home/flash_attention_golden.py"。<br/>**说明：** 若无数据生成脚本，填写空字符或null。 | 否 |
| gen_data | bool | 是否根据data_script生成输入和标杆数据。 | - true：采用脚本生成数据。<br/>- false：不采用脚本生成数据，默认false。 | 是 |
| params | name | string | 核函数输入/输出的参数名。 | 根据实际情况设置。 | 是 |
| params | dtype | string | 输入/输出的数据类型。 | 目前支持bool、int、int8、int16、int32、int64、uint8、uint16、uint32、uint64、float16、float32、float64、bfloat16。<br/>**说明：** 算子json配置时dtype仅允许输入一种数据类型，不支持多种数据类型。 | 是 |
| params | param_type | string | 标识该节点是输入或输出。 | - input：表示为输入节点。<br/>- output：表示为输出节点。 | 是 |
| params | shape | list | 输入/输出的shape。 | 根据算子实际shape填写，例如[24, 20, 144, 8]。<br/>**说明：** 当输入为Scalar时，shape填null。 | 是 |
| params | data_file | string | - 当param_type为input：指定输入数据bin文件。<br/>- 当param_type为output：指定标杆数据bin文件。 | 根据实际情况设置数据bin文件路径，必须为绝对路径，例如"/home/data.bin"。<br/>**说明：**<br/>- 当data_file设为空字符或null，表示不对运行输出作精度比对。<br/>- 当输入为Scalar时，data_file字段删除，只需配置data_value。 | 否 |
| params | data_value | 由dtype确定 | 输入的Scalar值。 | 根据实际情况填写。<br/>**说明：**<br/>- 仅当param_type为input才可以配置该参数。<br/>- data_value与data_file互斥。若配置data_value则data_file必须删除，且shape必须为null，表示本节点是Scalar输入。 | 否 |
| kernel_info<br/>**说明：** 仅**核函数直调工程场景**需要设置kernel文件相关的配置。 | kernel_source | string | Kernel入口源文件绝对路径，注意格式是“绝对路径+文件名”。 | 根据开发的核函数信息填写。 | 是 |
| kernel_info | kernel_name | string | Kernel入口函数名。 | 根据开发的核函数信息填写。 | 是 |
| kernel_info | kernel_includes | list | Kernel文件依赖的头文件所在的路径。 | 可为[]，也可填入多个路径。 | 是 |


#### 特殊格式输入

- **场景1：支持Scalar格式的输入。**
    当输入为Scalar格式，json中“params”配置项中删除data_file，param_type配为“input”，shape配为null，data_value配为指定的标量值。

    ```json
    { 
        "op_type": "xxxx", 
        "data_script": "", 
        "gen_data": false, 
        "params": [{ 
                    "name": "input_1", 
                    "dtype": "float16", 
                    "param_type": "input", 
                    "shape": null, 
                    "data_value": 8 
                }, 
                // ... 
        ] 
    }
    ```

- **场景2：支持TensorList格式的输入。**
  当输入为TensorList格式，该参数需要用 **[ ]** 表示，List中的每一项表示一个Tensor，示例如下。

  ```json
    { 
        "op_type": "xxxx", 
        "data_script": "", 
        "gen_data": false, 
        "params": [ 
                [{
                    "name": "input_1",
                    "dtype": "float16",
                    "param_type": "input",
                    "shape": [1,16384],
                    "data_file": "input_1.bin"
                },  
                {
                    "name": "input_2",
                    "dtype": "float16",
                    "param_type": "input",
                    "shape": [1,16384],
                    "data_file": "input_2.bin"
                }],
                {  
                    "name": "output", 
                    "dtype": "float16", 
                    "param_type": "output", 
                    "shape": [1,16384], 
                    "data_file": "golden.bin" 
                }, 
        ], 
        "kernel_info": { 
            "kernel_source": "add_custom.cpp", 
            "kernel_name": "add_custom", 
            "kernel_includes": [] 
        } 
    }
  ```

- **场景3：支持原地算子格式的输入。**
  当算子为[原地算子(in-place op)](#41算子调试概述)时，输入和输出地址一样，配置算子json时除了“param_type”和“data_file”不同，其他配置项均保持一致。配置示例如下。

  - param_type：分别配为input、output。
  - data_file：输入数据为x.bin，输出数据为标杆数据golden_x.bin。

  ```json
    { 
        "op_type": "add_custom", 
        "data_script": "", 
        "gen_data": false, 
        "params": [{ 
                    "name": "x", 
                    "dtype": "float16", 
                    "param_type": "input", 
                    "shape": [1,16384], 
                    "data_file": "x.bin"       
                }, 
                { 
                    "name": "x", 
                    "dtype": "float16", 
                    "param_type": "output", 
                    "shape": [1,16384], 
                    "data_file": "golden_x.bin" 
                }, 
                { 
                    "name": "y", 
                    "dtype": "float16", 
                    "param_type": "input", 
                    "shape": [1,16384], 
                    "data_file": "y.bin" 
                }, 
        ], 
        "kernel_info": { 
            "kernel_source": "add_custom.cpp", 
            "kernel_name": "add_custom", 
            "kernel_includes": [] 
        } 
    }
    ```