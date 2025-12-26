#!/usr/bin/env/ python
import os
import sys
import json
import numpy as np

def gen_data_sample():
    temp_json_path = sys.argv[1]
    output_path = sys.argv[2]
    with open(temp_json_path) as json_file:
        temp_json_data = json.load(json_file)
    inputx = temp_json_data["inputs"][0]
    inputx_data = np.random.uniform(-100, 100, inputx["shape"].astype(np.float16))
    inputx_data.tofile(os.path.join(output_path, inputx["data_file"]))

    inputy = temp_json_data["inputs"][1]
    inputy_data = np.random.uniform(-100, 100, inputx["shape"].astype(np.float16))
    inputy_data.tofile(os.path.join(output_path, inputy["data_file"]))

    golden = temp_json_data["outputs"][0]
    golden_data = (inputx_data + inputy_data).astype(np.float16)
    golden_data.tofile(os.path.join(output_path, golden["data_file"]))

if __name__ == "__main__":
    gen_data_sample()

