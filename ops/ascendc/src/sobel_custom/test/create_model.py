from onnx import helper
from onnx import AttributeProto, TensorProto, GraphProto
import onnx
import os
import numpy as np

input1_input = helper.maker_tensor_value_info("input1", TensorProto.UINT8, [1, 763, 1024, 3])

output = helper.maker_tensor_value_info("output1", TensorProto.UINT8, [1, 1, 761, 1023])

custom_op = onnx.helper.make_node(
    "SobelCustom",
    name = "SobelCustom",
    inputs = ['input1'],
    outputs = ['output1'],
)

graph_def = helper.make_graph(
    [custom_op],
    "test_model",
    [input1_input],
    [output],
    initializer = [],
)

model_def = helper.make_model(graph_def, producer_name='onnx_example')

model_def.opset_import[0].version = 11

model_def.ir_version = 6

new_model_path = os.path.join("./", 'SobelCustom.onnx')

onnx.save(model_def, new_model_path)