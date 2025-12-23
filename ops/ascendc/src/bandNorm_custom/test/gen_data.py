#!/usr/bin/env/ python

import numpy as np
import torch

def cal_band_power(real, image):
    eps = torch.finfo(torch.float32).eps
    real_square = np.square(real)
    image_square = np.square(image)
    power_1 = real_square + image_square
    power_2 = np.sum(power_1, axis=1)
    power = np.sqrt(power_2 + eps)
    return power

if __name__ == "__main__":

    input_shape = [2, 121, 47]
    real_data = np.random.randn(*input_shape).astype(np.float16)
    imag_data = np.random.randn(*input_shape).astype(np.float16)
    power = cal_band_power(real_data, imag_data)

    real_data.tofile("./real.bin")
    imag_data.tofile("./imag.bin")
    power.tofile("./target.bin")

