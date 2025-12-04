# cann-recipes-harmony-infer

## 🔥Latest News
- [2025/12] GLM-Edge-1.5B-Chat模型在KirinX90上支持推理部署。

## 🚀概述
cann-recipes-harmony-infer仓库旨在为鸿蒙开发者提供一些基于Ascend C高效实现的自定义创新算法，支持在鸿蒙设备和昇腾设备运行。同时提供基于CANN平台的优化样例，方便开发者快速实现端云模型迁移和部署。

## ✨解决方案列表
|实践|解决方案简介|
|-----|-----|
|[QuantMatmul](ops/ascendc/docs/custom-npu_quant_matmul.md)在支付宝端侧大模型实践|基于Ascend C实现QuantMatmul自定义量化算子Kirin9030设备上高性能部署。
|[BandNorm](ops/ascendc/docs/custom-npu_bandnorm.md)在QQ音乐声伴分离实践|基于Ascend C实现BandNorm自定义数据类型计算，助力QQ音乐在鸿蒙Kirin9020设备上实现人声和伴奏分离。|
|[RmsNorm](ops/ascendc/docs/custom-npu_rmsnorm.md)在智谱GLM-Edge-1.5b-Chat大模型实践|基于Ascend C实现BandNorm算子KirinX90设备上高性能部署。|
|[FasterGelu](ops/ascendc/docs/custom-npu_fastergelu.md)在悟空图像SDXL大模型实践|基于Ascend C实现BandNorm算子KirinX90设备上高性能部署。|

## 📖目录结构说明
```
├── docs                                        # 文档目录
|  ├── models                                   # 模型文档目录
|  |  ├── chat-glm                     # 智谱chat-glm-1.5B相关文档
|  |  ├── qwen3                             # Qwen3-Dense-7B相关文档
|  |  └── ...
|  └── common                                   # 公共文档目录
├── harmony_infer                                    # 鸿蒙设备模型执行相关
|  ├── harmony_os                         # 鸿蒙双框架模型执行
|  ├── harmony_os_next                       # 鸿蒙单框架框架模型执行
│  └── ...
├── models                                      # 模型脚本目录
|  ├── chat-glm                        # chat-glm的模型脚本亲和性适配及导出脚本
|  ├── qwen3                               # Qwen3-Dense-7B的模型脚本亲和性适配及导出脚本
│  └── ...
├── ops                                         # 算子目录
|  ├── ascendc                                  # ascendc算子
|  |  ├── docs                   # 算子文档目录
|  |  |   ├── custom-npu_quant_matmul.md                   # 支付宝自定义量化算子文档
|  |  |   ├── custom-npu_bandnorm.md                   # QQ音乐bandnorm算子文档
|  |  ├── src                     # 算子实现目录
|  |  |   ├── bandnorm_custom                     # QQ音乐STFT算子实现
|  |  |   ├── quant_matmul_custom                     # 支付宝自定义量化算子实现
└── CONTRIBUTION.md
└── README.md
└── ...
```

## 📝相关信息

- [贡献指南](./CONTRIBUTION.md)
- [许可证](./LICENSE)

    cann-recipes-harmony-infer仓涉及的模型，如模型目录下存在License的以该License为准。如模型目录下不存在License的，遵循Apache 2.0许可证，对应协议文本可查阅[LICENSE](./LICENSE)
- [免责声明](./DISCLAIMER.md)
