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
"""Generate deterministic FP16 inputs and a CPU LogAddExp golden output."""

import argparse
import logging
from typing import Sequence, Tuple

import numpy as np


INPUT_A_SHAPE = (2, 1, 4)
INPUT_B_SHAPE = (1, 3, 1)
SEED = 2026


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate FP16 LogAddExp test data."
    )
    parser.add_argument(
        "--strict-special",
        type=int,
        choices=(0, 1),
        default=0,
        help=(
            "match LOGADDEXP_STRICT_SPECIAL: 0 generates finite inputs; "
            "1 generates Inf and NaN inputs"
        ),
    )
    return parser.parse_args()


def generate_finite_inputs() -> Tuple[np.ndarray, np.ndarray]:
    rng = np.random.default_rng(SEED)
    input_a = rng.uniform(-8.0, 8.0, size=INPUT_A_SHAPE).astype(np.float16)
    input_b = rng.uniform(-8.0, 8.0, size=INPUT_B_SHAPE).astype(np.float16)
    return input_a, input_b


def fp16_from_bits(values: Sequence[int], shape: Tuple[int, ...]) -> np.ndarray:
    return np.asarray(values, dtype=np.uint16).view(np.float16).reshape(shape)


def generate_special_inputs() -> Tuple[np.ndarray, np.ndarray]:
    # A covers +/-Inf, positive/negative qNaN, positive/negative sNaN and
    # finite values. B covers the special-value handling of the second input.
    input_a = fp16_from_bits(
        [
            0xFC00,  # -Inf
            0x7C00,  # +Inf
            0x7E00,  # positive qNaN
            0xFE00,  # negative qNaN
            0x7C01,  # positive sNaN
            0xFC01,  # negative sNaN
            0x0000,  # +0
            0xC200,  # -3
        ],
        INPUT_A_SHAPE,
    )
    input_b = fp16_from_bits(
        [
            0xFC00,  # -Inf
            0x7C00,  # +Inf
            0x7E00,  # positive qNaN
        ],
        INPUT_B_SHAPE,
    )
    return input_a, input_b


def main() -> None:
    args = parse_args()
    if args.strict_special == 1:
        mode = "strict-special"
        input_a, input_b = generate_special_inputs()
    else:
        mode = "finite"
        input_a, input_b = generate_finite_inputs()

    with np.errstate(invalid="ignore", over="ignore"):
        golden = np.logaddexp(input_a, input_b).astype(np.float16)

    input_a.tofile("input_a.bin")
    input_b.tofile("input_b.bin")
    golden.tofile("golden.bin")
    logging.info(
        "[gen_data] A%s + B%s -> C%s, dtype=float16, mode=%s",
        input_a.shape, input_b.shape, golden.shape, mode
    )


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    main()
