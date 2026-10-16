#include <cstdlib>
#include <new>
#include <malloc.h>

// Test-executable-only interception of C++ allocation, including aligned allocation.
namespace { thread_local bool armed = false; thread_local size_t allocations = 0; }
void beginAllocationCheck() { allocations = 0; armed = true; }
size_t endAllocationCheck() { armed = false; return allocations; }
void* operator new(size_t size)
{
    if (armed) ++allocations;
    if (void* p = std::malloc(size == 0 ? 1 : size)) return p;
    throw std::bad_alloc();
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, size_t) noexcept { std::free(p); }
void operator delete[](void* p, size_t) noexcept { std::free(p); }
void* operator new(size_t size, std::align_val_t alignment)
{
    if (armed) ++allocations;
    if (void* p = _aligned_malloc(size == 0 ? 1 : size, static_cast<size_t>(alignment))) return p;
    throw std::bad_alloc();
}
void* operator new[](size_t size, std::align_val_t alignment) { return ::operator new(size, alignment); }
void operator delete(void* p, std::align_val_t) noexcept { _aligned_free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { _aligned_free(p); }
void operator delete(void* p, size_t, std::align_val_t) noexcept { _aligned_free(p); }
void operator delete[](void* p, size_t, std::align_val_t) noexcept { _aligned_free(p); }
