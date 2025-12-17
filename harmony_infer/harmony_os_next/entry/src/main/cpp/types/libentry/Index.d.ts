import { resourceManager } from "@kit.LocalizationKit";

export const LoadModel : (resMgr : resourceManager.ResourceManager) => number
export const processImageWithSobel: (buffer: Uint8Array, width: number, height: number) => Uint8Array;
export const InitIOTensors : (input : Uint8Array) => number
export const GetSobelResult : () => Uint8Array
export const RunModel : () => number
export const GetResult : () => string[]
export const UnloadModel : () => number