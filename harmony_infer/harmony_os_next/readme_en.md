## HiAI Foundation -- Model Inference

## Introduction

This sample code demonstrates how to call CANN APIs to perform model inference.

This sample code demonstrates the capability of filtering and recognizing objects in images.

It depends on **libhiai_foundation.so**, a dynamic library of CANN, and **libneural_network_core.so**, a public dynamic library in the AI domain.

## Preview

|         **Home screen**          |            **result**            | **Next image**                    |   |
|:------------------------:|:--------------------------------:|------------------------------|---|
| ![](screenshots/sobel_pre.jpeg) | ![](screenshots/sobel_finished.jpeg) |![](screenshots/sobel_next.jpeg) |   |


Instructions:

1. On the main screen of the phone, click "NPU Inference" to launch the app. The app will automatically load the model, process the image, and display the processed image along with the model processing time.
2. Click "CPU Inference" to process the image using the CPU, and display the processed image along with the CPU processing time.
3. Click "Click for next image" to show the next image.
4. When exiting the app, the model will be automatically unloaded.


## Project Directory
```
└── entry/src/main                           // Code area
    ├── cpp
    │    ├── types/libentry
    │    │     └── Index.d.ts                // API registration file at the native layer
    │    ├── SobelCustom.cpp                 // Functions of APIs at the native layer
    │    ├── CMakeLists.txt                  // Compilation configurations at the native layer
    │    ├── HIAIModelManager.cpp            // Implementation of the model management class
    │    ├── HIAIModelManager.h              // Definition of the model management class
    ├── ets
    │    ├── entryability 
    │    │     └── EntryAbility.ets          //  Entry point class
    │    ├── entrybackupability 
    │    │     └── EntryBackupAblity.ets     //  backup file  
    │    ├── pages 
    │    │     └── Index.ets                 // Home screen display class
    └── resources
    │    ├── base/media                      // Image resources
    │    │   ├── cup.jpg
    │    │   └── guitar.jpg
    │    ├── rawfile 
    │    │    ├── hiai.om                    // Model file
 
```

## Implementation Details

The sample code uses the following APIs defined in HiAI Foundation and NNCore:

* OH_NN_ReturnCode HMS_HiAIOptions_SetBandMode(OH_NNCompilation* compilation, HiAI_BandMode bandMode)
* HiAI_BandMode HMS_HiAIOptions_GetBandMode(const OH_NNCompilation* compilation)
* OH_NN_ReturnCode HMS_HiAIOptions_SetModelDeviceOrder(OH_NNCompilation* compilation, HiAI_ExecuteDevice* executeDevices, size_t deviceCount);
* HiAI_Compatibility HMS_HiAICompatibility_CheckFromBuffer(const void* data, size_t size);
* OH_NN_ReturnCode OH_NNDevice_GetAllDevicesID(const size_t **allDevicesID, uint32_t *deviceCount);
* OH_NN_ReturnCode OH_NNDevice_GetName(size_t deviceID, const char **name);
* void *OH_NNTensor_GetDataBuffer(const NN_Tensor *tensor);
* OH_NN_ReturnCode OH_NNTensor_GetSize(const NN_Tensor *tensor, size_t *size);
* OH_NN_ReturnCode OH_NNTensor_Destroy(NN_Tensor **tensor);
* OH_NNCompilation *OH_NNCompilation_ConstructWithOfflineModelBuffer(const void *modelBuffer, size_t modelSize);
* void OH_NNCompilation_Destroy(OH_NNCompilation **compilation);
* OH_NN_ReturnCode OH_NNCompilation_SetDevice(OH_NNCompilation *compilation, size_t deviceID);
* void OH_NNCompilation_Destroy(OH_NNCompilation **compilation);
* OH_NN_ReturnCode OH_NNCompilation_Build(OH_NNCompilation *compilation);
* OH_NNExecutor *OH_NNExecutor_Construct(OH_NNCompilation *compilation);
* OH_NN_ReturnCode OH_NNExecutor_GetInputCount(const OH_NNExecutor *executor, size_t *inputCount);
* NN_TensorDesc *OH_NNExecutor_CreateInputTensorDesc(const OH_NNExecutor *executor, size_t index);
* NN_Tensor *OH_NNTensor_Create(size_t deviceID, NN_TensorDesc *tensorDesc);
* OH_NN_ReturnCode OH_NNTensorDesc_Destroy(NN_TensorDesc **tensorDesc);
* OH_NN_ReturnCode OH_NNExecutor_GetOutputCount(const OH_NNExecutor *executor, size_t *outputCount);
* NN_TensorDesc *OH_NNExecutor_CreateOutputTensorDesc(const OH_NNExecutor *executor, size_t index);
* OH_NN_ReturnCode OH_NNExecutor_RunSync(OH_NNExecutor *executor, NN_Tensor *inputTensor[], size_t inputCount, NN_Tensor *outputTensor[], size_t outputCount);
* void OH_NNExecutor_Destroy(OH_NNExecutor **executor);

Call APIs such as **OH_NNExecutor_RunSync**, pass the image to be classified, and wait for the classification result to display. Then check the logs. See **entry/src/main/ets/pages/Index.ets** for reference.

## Required Permissions

N/A

## Dependency

N/A

## Constraints

1. This example can run only on standard systems. Supported devices: Huawei phones, tablets, and 2in1.
2. HarmonyOS: HarmonyOS 5.0.1 Release or later
3. DevEco Studio: DevEco Studio 6.0.0 Release or later
4. HarmonyOS SDK: HarmonyOS 6.0.0 Release SDK or later
