#include "register/tilingdata_base.h"
#include "tiling/rmsnorm/rmsnorm_tiling.h"
#include "tiling/platform/platform_ascendc.h"
#include "register/op_def_registry.h"

namespace optiling {
BEGIN_TILING_DATA_DEF(TilingRmsNormGm2Ub)
    TILING_DATA_FIELD_DEF(uint64_t, blockM);
    TILING_DATA_FIELD_DEF(uint32_t, splitM);
    TILING_DATA_FIELD_DEF(uint32_t, splitK);
    TILING_DATA_FIELD_DEF(uint32_t, loopK);
    TILING_DATA_FIELD_DEF(uint32_t, tailK);
END_TILING_DATA_DEF;
REGISTER_TILING_DATA_CLASS(TilingRmsNormGm2UbOp, TilingRmsNormGm2Ub)

BEGIN_TILING_DATA_DEF(TilingRmsNormLargeReduce)
    TILING_DATA_FIELD_DEF(uint32_t, reduceSplitK);
    TILING_DATA_FIELD_DEF(uint32_t, reduceLoopK);
    TILING_DATA_FIELD_DEF(uint32_t, reduceTailK);
END_TILING_DATA_DEF;
REGISTER_TILING_DATA_CLASS(TilingRmsNormLargeReduceOp, TilingRmsNormLargeReduce)

BEGIN_TILING_DATA_DEF(RMSNormTilingData)
    TILING_DATA_FIELD_DEF(uint64_t, originM);
    TILING_DATA_FIELD_DEF(uint64_t, originK);
    TILING_DATA_FIELD_DEF(uint32_t, hasGamma);
    TILING_DATA_FIELD_DEF(float, epsilon);
    TILING_DATA_FIELD_DEF_STRUCT(RmsNormTiling, rmsNormTiling);
    TILING_DATA_FIELD_DEF_STRUCT(TilingRmsNormGm2Ub, tilingDataGm2Ub);
    TILING_DATA_FIELD_DEF_STRUCT(TilingRmsNormLargeReduce, tilingDataLargeReduce);
END_TILING_DATA_DEF;

REGISTER_TILING_DATA_CLASS(RmsNormCustom, RMSNormTilingData)
}