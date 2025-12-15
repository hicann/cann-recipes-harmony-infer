
#include "register/tilingdata_base.h"

namespace optiling {
BEGIN_TILING_DATA_DEF(SobelCustomTilingData)
  TILING_DATA_FIELD_DEF(uint32_t, size);
  TILING_DATA_FIELD_DEF(uint32_t, totalLength);
  TILING_DATA_FIELD_DEF(uint32_t, blockLength);
  TILING_DATA_FIELD_DEF(uint32_t, tileLength);
  TILING_DATA_FIELD_DEF(uint32_t, grayLength);
  TILING_DATA_FIELD_DEF(uint32_t, tileNum);
  TILING_DATA_FIELD_DEF(uint32_t, H);
  TILING_DATA_FIELD_DEF(uint32_t, W);
END_TILING_DATA_DEF;

REGISTER_TILING_DATA_CLASS(SobelCustom, SobelCustomTilingData)
}
