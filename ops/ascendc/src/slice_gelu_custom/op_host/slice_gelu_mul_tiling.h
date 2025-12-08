
#ifndef OP_HOST_SLICEGELUMUL_TILING_H
#define OP_HOST_SLICEGELUMUL_TILING_H
#include "register/tilingdata_base.h"

namespace optiling {
BEGIN_TILING_DATA_DEF(SliceGeluMulTilingData)
  TILING_DATA_FIELD_DEF(uint32_t, totalLength);
  TILING_DATA_FIELD_DEF(uint32_t, tileNum);
  TILING_DATA_FIELD_DEF(uint32_t, ALIGN_NUM);
  TILING_DATA_FIELD_DEF(uint32_t, tiling_size);
  TILING_DATA_FIELD_DEF(uint32_t, block_size);
  TILING_DATA_FIELD_DEF(uint32_t, aivNum);
  TILING_DATA_FIELD_DEF(uint32_t, core_size);
  TILING_DATA_FIELD_DEF(uint32_t, core_remain);
  TILING_DATA_FIELD_DEF(uint32_t, outputDim2);
  TILING_DATA_FIELD_DEF(uint32_t, outputDim1);
  TILING_DATA_FIELD_DEF(uint32_t, blockFactor);
END_TILING_DATA_DEF;

REGISTER_TILING_DATA_CLASS(SliceGeluMul, SliceGeluMulTilingData)
}

#endif // OP_HOST_SLICEGELUMUL_TILING_H
