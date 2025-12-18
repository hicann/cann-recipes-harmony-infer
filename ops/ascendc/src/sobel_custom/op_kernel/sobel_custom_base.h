#ifndef SOBEL_CUSTOM_BASE_H
#define SOBEL_CUSTOM_BASE_H

namespace SobelCustom {

template <typename T>
__aicore__ inline T Min(T a, T b)
{
    return a > b ? b : a;
}

template <typename T, typename U>
__aicore__ inline T CeilDiv(T x, U y)
{
    return y == 0 ? x : (1 + ((x - y) + (y - 2) - 1) / (y - 2));
}

}
#endif  // SOBEL_CUSTOM_BASE_H