#include "kernel_operator.h"
#include "adv_api/activation/gelu.h"

using namespace AscendC;
constexpr int32_t BUFFER_NUM = 2;
#ifndef CEIL
#define CEIL(N,n) (((N)+(n)-1)/(n))
#endif 

template <typename T>
class KernelSliceGeluMul
{
public:
    __aicore__ inline KernelSliceGeluMul() {}
    __aicore__ inline void Init(
        GM_ADDR x,
        GM_ADDR y,
        const SliceGeluMulTilingData* tilingData
        )
    {
        this->blockFactor=tilingData->blockFactor;
        this->outputDim1=tilingData->outputDim1;
        this->outputDim2=tilingData->outputDim2;
        auto ublength=this->outputDim1*this->outputDim2*2;
        
        this->src_global.SetGlobalBuffer((__gm__ T *)(x), ublength);
        this->dst_global.SetGlobalBuffer((__gm__ T *)(y), ublength/2);

        pipe.InitBuffer(inQueueXL, BUFFER_NUM, this->blockFactor);
        pipe.InitBuffer(inQueueXR, BUFFER_NUM, this->blockFactor);
        pipe.InitBuffer(inQueueTmp, BUFFER_NUM, this->blockFactor);
        pipe.InitBuffer(outQueue, BUFFER_NUM, this->blockFactor);
    }

    __aicore__ inline void Process()
    {
        uint64_t batch=this->outputDim1;
        uint64_t dim=this->outputDim2;

        this->onceDimSize=dim*sizeof(T);
        auto dimfrac=this->blockFactor/onceDimSize;

        //auto fracDimnum = dimfrac==0?batch:(batch+dimfrac-1)/dimfrac;
        auto fracDimnum=CEIL(batch,dimfrac);
        uint64_t onceBatch = fracDimnum==1?batch:dimfrac;
        uint64_t onceBatchL = batch-(fracDimnum-1)*dimfrac;

        AscendC::printf("\n onceDimSize=%d dimfrac=%d fracDimnum=%d onceBatch=%d onceBatchL=%d",onceDimSize,dimfrac,fracDimnum,onceBatch,onceBatchL);
        for(int32_t i=0;i<fracDimnum;i++)
        {
            this->realBatch=(i==fracDimnum-1)?onceBatchL:onceBatch;
            this->outputSize=realBatch*onceDimSize;
            AscendC::printf("\n index=%d",i);
            AscendC::printf("\n realBatch=%d outputSize=%d fracDimnum=%d onceBatch=%d onceBatchL=%d",this->realBatch,this->outputSize,fracDimnum,onceBatch,onceBatchL);
            CopyIn(i,this->blockFactor);
            Compute(i,this->blockFactor);
            CopyOut(i,this->blockFactor);
            this->inputStart+=outputSize;
            this->outputStart+=outputSize/2;
        }
    }

private:
    __aicore__ inline void CopyIn(uint32_t process, uint32_t length)
    {
        LocalTensor<T> xlLocal=inQueueXL.AllocTensor<T>();
        LocalTensor<T> xrLocal=inQueueXR.AllocTensor<T>();
        DataCopyExtParams dataCopyParamsL(this->realBatch,this->onceDimSize,onceDimSize,0,0);
        DataCopyExtParams dataCopyParamsR(this->realBatch,onceDimSize,onceDimSize,0,0);
        DataCopyPadExtParams<T> padParams(false,0,0,0);
        AscendC::printf("\n inputStart = %d",this->inputStart);
        
        DataCopyPad(xlLocal,src_global[this->inputStart],dataCopyParamsL,padParams);
        DataCopyPad(xrLocal,src_global[this->inputStart+this->outputDim2],dataCopyParamsR,padParams);
    
        inQueueXL.EnQue(xlLocal);
        inQueueXR.EnQue(xrLocal);
    }
    __aicore__ inline void Compute(uint32_t process, uint32_t length)
    {
        LocalTensor<T> xlLocal=inQueueXL.DeQue<T>();
        LocalTensor<T> xrLocal=inQueueXR.DeQue<T>();
        LocalTensor<T> tmplocal=inQueueTmp.AllocTensor<T>();
        LocalTensor<T> yLocal=outQueue.AllocTensor<T>();

        AscendC::FasterGelu<T,false,false>(tmplocal,xrLocal,this->realBatch*this->outputDim2);
        AscendC::Mul<T>(yLocal,xlLocal,tmplocal,this->realBatch*this->outputDim2);

        outQueue.EnQue<T>(yLocal);
        inQueueXL.FreeTensor(xlLocal);
        inQueueXR.FreeTensor(xrLocal);
        inQueueTmp.FreeTensor(tmplocal); 
    }

    __aicore__ inline void CopyOut(uint32_t process, uint32_t length)
    {
        LocalTensor<T> yLocal = outQueue.DeQue<T>();
        DataCopyExtParams dataCopyParams(1,this->outputSize,0,0,0);
        AscendC::printf("\n outputStart = %d outputSize = %d",this->outputStart,this->outputSize);
        //PipeBarrier<PIPE_ALL>();
        DataCopyPad(dst_global[this->outputStart],yLocal,dataCopyParams);
        outQueue.FreeTensor(yLocal);
    }

private:

    TPipe pipe;
    GlobalTensor<T> src_global;
    GlobalTensor<T> dst_global;
    TQue<QuePosition::VECIN, 1> inQueueXL;
    TQue<QuePosition::VECIN, 1> inQueueXR;
    TQue<QuePosition::VECIN, 1> inQueueTmp;
    TQue<QuePosition::VECOUT, 1> outQueue;

    uint64_t bloackDim=0;
    uint64_t blockFactor=0;
    uint64_t tailBlockNum=0;
    uint64_t blockTailFactor=0;
    uint32_t loopCount=0;
    uint32_t ubSize=0;
    uint32_t outputSize=0;
    uint32_t realBatch=0;
    uint64_t onceDimSize=0;
    uint64_t inputStart=0;
    uint64_t outputStart=0;

    uint32_t outputDim1=0;
    uint32_t outputDim2=0;
  
};

extern "C" __global__ __aicore__ void slice_gelu_mul(GM_ADDR a, GM_ADDR b, GM_ADDR workspace, GM_ADDR tiling) {
    GET_TILING_DATA(tiling_data, tiling);
    // TODO: user kernel impl
    KernelSliceGeluMul<half> op;
    op.Init(a, b,&tiling_data);
    op.Process();
}
