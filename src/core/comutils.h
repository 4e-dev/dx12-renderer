/*
Name: comutils.h
Author: Bao Bui
Description: Utility functions for anything related to ComPtrs.
*/

#pragma once
#include <wrl.h>

using namespace Microsoft::WRL;

/*
Description: Manually releases ComPtr types.
In: The address to a ComPtr.
Out: Void.
*/
template<typename T>
void ManuallyReleaseCom(T*& ComPtr) {
    if (ComPtr) {
        ComPtr->Release();
        ComPtr = nullptr;
    }
}
