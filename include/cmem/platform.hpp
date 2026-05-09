#pragma once
#include <cstddef>

namespace cmem {

void* os_reserve(size_t size);
void  os_release(void* ptr, size_t size);
size_t os_page_size();

} // namespace cmem
