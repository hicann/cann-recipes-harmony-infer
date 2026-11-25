# Copyright (c) 2026 Tang Risheng
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Generate a single-node FP16 LogAddExp ONNX model with broadcasting."""

import logging
from pathlib import Path

import onnx
from onnx import TensorProto, helper


INPUT_A_SHAPE = [2, 1, 4]
INPUT_B_SHAPE = [1, 3, 1]
OUTPUT_SHAPE = [2, 3, 4]
OUTPUT_PATH = Path("LogAddExp.onnx")


def main() -> None:
    input_a = helper.make_tensor_value_info(
        "A", TensorProto.FLOAT16, INPUT_A_SHAPE
    )
    input_b = helper.make_tensor_value_info(
        "B", TensorProto.FLOAT16, INPUT_B_SHAPE
    )
    output = helper.make_tensor_value_info(
        "C", TensorProto.FLOAT16, OUTPUT_SHAPE
    )
    node = helper.make_node(
        "LogAddExp",
        inputs=["A", "B"],
        outputs=["C"],
        name="LogAddExp",
        domain="custom",
    )
    graph = helper.make_graph(
        [node], "log_add_exp_test", [input_a, input_b], [output]
    )
    model = helper.make_model(
        graph,
        producer_name="log-add-exp-sample",
        opset_imports=[helper.make_opsetid("", 11), helper.make_opsetid("custom", 1)],
    )
    model.ir_version = 6
    onnx.checker.check_model(model)
    onnx.save(model, OUTPUT_PATH)
    logging.info("[create_onnx] generated %s", OUTPUT_PATH)


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    main()
