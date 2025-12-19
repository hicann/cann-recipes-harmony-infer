from onnx import helper
from onnx import AttributeProto, TensorProto, GraphProto
import onnx
import os
import numpy as np

input = helper.make_tensor_value_info("input", TensorProto.FLOAT16, [64, 2048])

input1 = helper.make_tensor_value_info("input1", TensorProto.FLOAT16, [2048])

output1 = helper.make_tensor_value_info("output1", TensorProto.FLOAT16, [64, 2048])

custom_op = onnx.helper.make_node(
    "RmsNormCustom",
    name = "RmsNormCustom",
    inputs = ['input', 'input1'],
    outputs = ['output1'],
)

graph_def = helper.make_graph(
    [custom_op],
    "test-model",
    [input, input1],
    [output1],
    initializer = [],
)

model_def = helper.make_model(graph_def, producer_name='onnx-example')

model_def.opset_import[0].version = 11

model_def.ir_version = 6

new_model_path = os.path.join("./", 'RmsNormCustom.onnx')

onnx.save(model_def, new_model_path)