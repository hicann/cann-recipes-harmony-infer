# 应用场景说明
图像CV前处理计算灵活多变, 常规NN模型较难处理, 难以在NPU上使用常规NN算子计算。为在NPU上解决CV前处理灵活计算的问题, CANN与某伙伴的合作中，使用Ascend C实现了CV领域的sobel计算，有效将前处理搬移到NPU上计算。此方案在某客户的边缘检测业务中得到验证和应用。

# SobelCustom自定义算子样例说明
本样例通过Ascend C编程语言，实现Sobel算子。
按照工程创建->算子实现->编译部署>算子调用的流程完成算子开发。整个过程都依赖于算子工程：基于工程代码框架完成算子核函数的开发和Tiling实现，通过工程编译脚本完成算子的编译部署，继而实现单算子调用或第三方框架中的算子调用。

# 算子描述
FasterGelu算子对应的数学表达式为：
$FasterGelu(xi) = xi/(1+exp(-1.702*|xi|))*exp(0.851*(xi-|xi|))$
其中$xi$是输入的元素

# 算子规格描述
支持 1* 576 * 10240和1 * 2304 *5120两个规格

# 支持的产品型号
本样例支持如下产品型号：

HUAWEI MateBook Pro

HUAWEI MateBook Fold非凡大师

HUAWEI MatePad Edge

# 编译安装执行
参考[ascendc算子工程编译部署](../src/README.md)
