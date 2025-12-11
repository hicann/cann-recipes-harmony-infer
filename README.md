# cann-recipes-harmony-infer

## 🔥Latest News
- [2025/12] Ascend C自定义算子在支付宝端侧大模型、QQ音乐声伴分离、智谱GLM-Edge-1.5b-Chat、悟空图像本地闪绘等项目中的实践。

## 🚀概述
cann-recipes-harmony-infer仓库旨在为鸿蒙开发者提供一些基于Ascend C高效实现的自定义创新算法，支持在鸿蒙设备和昇腾设备运行。同时提供基于CANN平台的优化样例，方便开发者快速实现端云算子、模型迁移和部署。

## ✨解决方案列表
|实践|解决方案简介|
|-----|-----|
|[QuantMatmul](ops/ascendc/docs/custom-npu_quant_matmul.md)在支付宝端侧大模型实践|基于Ascend C实现QuantMatmul自定义量化算子Kirin9030设备上高性能部署。
|[BandNorm](ops/ascendc/docs/custom-npu_bandnorm.md)在QQ音乐声伴分离实践|基于Ascend C实现BandNorm自定义数据类型计算，助力QQ音乐在鸿蒙Kirin9020设备上实现人声和伴奏分离。|
|[RmsNorm](ops/ascendc/docs/custom-npu_rmsnorm.md)在智谱GLM-Edge-1.5b-Chat大模型实践|基于Ascend C实现BandNorm算子KirinX90设备上高性能部署。|
|[SliceGelu](ops/ascendc/docs/custom-npu_slice_gelu.md)在悟空图像SDXL大模型实践|基于Ascend C实现SliceGelu融合算子KirinX90设备上高性能部署。|

## 📖目录结构说明
```
├── docs                                        # 文档目录
| ├──common                                     # 公共文档目录
├── ops                                         # 算子目录
|  ├── ascendc                                  # ascendc算子
|  |  ├── KernelInvocationSample                # 算子Kernel执行样例
|  |  ├── docs                                  # 算子文档目录
|  |  |   ├── custom-xxx.md                     # xxxx算子文档
|  |  |   ├── ...
|  |  ├── src                                   # 算子实现目录
|  |  |   ├── xxx_custom                        # xxx算子实现
|  |  |   ├── ...
└── CONTRIBUTION.md                             # 本仓贡献指南
└── README.md                                   # 本公共README
└── ...
```

## 📝相关信息

- [贡献指南](./CONTRIBUTION.md)
- [许可证](./LICENSE)

    cann-recipes-harmony-infer仓涉及的模型，如模型目录下存在License的以该License为准。如模型目录下不存在License的，遵循Apache 2.0许可证，对应协议文本可查阅[LICENSE](./LICENSE)
- [免责声明](./DISCLAIMER.md)
