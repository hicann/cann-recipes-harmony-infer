import os
import sys
import torch
import torch.nn as nn
import numpy as np

#1.构建模型
d0: int = 1
d1: int = 5
d2: int = 5120

def my_gelu(x):
    y = x / (1 + np.exp(-1.702 * x))
    return y
class SlicedGELUModel(nn.Module):
    def __init__(self):
        super().__init__()
        self.gelu = my_gelu

        #切片参数配置（可修改区域）
        self.slice_dim = 2      #切片维度（示例中切第2维，即5120维度）
        self.slice_start = 0    #起始索引
        self.slice_end = int(d2 / 2)   #结束索引（不包括该位置）
        self.slice_end_1 = d2

    def forward(self, x):
        #执行切片操作 [1， 1024， 5120] =》 [1， 1024， 2560]
        y = x[:, :, self.slice_end:self.slice_end_1]
        x = x[:, :, self.slice_start:self.slice_end]
        w = x * self.gelu(y)
        return w

model = SlicedGELUModel()
model.eval()

dummy_input = torch.randn(d0, d1, d2, dtype=torch.float16) #原始输入形状
output = model(dummy_input)
output_shape = model(dummy_input).shape
print(f"\n切片后输出形状：{output_shape}") #应显示 torch.Size（[1， 1024， 2560]）

dummy_input.numpy().tofile(f"./a.bin")
output.numpy().tofile(f"./b.bin")
