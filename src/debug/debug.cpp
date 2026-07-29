#include "debug.h"

void ThrowIfFailed(HRESULT result) {
    if (FAILED(result)) {
        throw std::runtime_error("HRESULT failed.");
    }
}
