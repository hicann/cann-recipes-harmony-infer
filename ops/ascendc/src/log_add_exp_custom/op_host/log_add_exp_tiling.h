
/*
 * Copyright (c) 2026 Tang Risheng
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OP_HOST_LOG_ADD_EXP_TILING_H
#define OP_HOST_LOG_ADD_EXP_TILING_H
#include "register/tilingdata_base.h"

namespace optiling {
BEGIN_TILING_DATA_DEF(LogAddExpTilingData)
  TILING_DATA_FIELD_DEF(uint32_t, tile_length);
  TILING_DATA_FIELD_DEF(uint32_t, total_length);
  TILING_DATA_FIELD_DEF(uint32_t, input_a_length);
  TILING_DATA_FIELD_DEF(uint32_t, input_b_length);
  TILING_DATA_FIELD_DEF(uint32_t, rank);
  TILING_DATA_FIELD_DEF(uint32_t, row_length);
  TILING_DATA_FIELD_DEF(uint32_t, tiles_per_row);
  TILING_DATA_FIELD_DEF(uint32_t, task_count);
  TILING_DATA_FIELD_DEF(uint32_t, tasks_per_core);
  TILING_DATA_FIELD_DEF(uint32_t, task_remain);
  TILING_DATA_FIELD_DEF(uint32_t, needs_broadcast);
  TILING_DATA_FIELD_DEF_ARR(uint32_t, 4, output_dims);
  TILING_DATA_FIELD_DEF_ARR(uint32_t, 4, input_a_strides);
  TILING_DATA_FIELD_DEF_ARR(uint32_t, 4, input_b_strides);
  // Keep the packed replay TilingData size divisible by 8 bytes.
  TILING_DATA_FIELD_DEF(uint32_t, reserved);
END_TILING_DATA_DEF;

REGISTER_TILING_DATA_CLASS(LogAddExp, LogAddExpTilingData)
}  // namespace optiling

#endif  // OP_HOST_LOG_ADD_EXP_TILING_H
