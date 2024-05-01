#include "unimemory.h"
#include <iostream>
#include <cstdlib>

namespace UniMemory {
void AbortIfOutOfMemory(void *ptr) {
    if (ptr == nullptr) {
        std::cerr << "Out of memory" << std::endl;
        std::abort();
    }
}

void ExitIfOutOfMemory(void *ptr) {
    if (ptr == nullptr) {
        std::cerr << "Out of memory" << std::endl;
        std::_Exit(-1);
    }
}
}