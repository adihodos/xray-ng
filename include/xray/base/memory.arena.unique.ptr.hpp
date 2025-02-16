#pragma once

#include "xray/base/unique_pointer.hpp"
#include "xray/base/memory.arena.hpp"

namespace xray::base {

template<typename T>
struct MemoryArenaDeleter
{
    MemoryArena* arena;

    explicit MemoryArenaDeleter(MemoryArena* a) noexcept
        : arena{ a }
    {
    }

    void operator()(T* ptr) const noexcept
    {
        if (ptr) {
            ptr->~T();
            arena->free(ptr, sizeof(T));
        }
    }
};

template<typename T>
using unique_arena_ptr = unique_pointer<T, MemoryArenaDeleter<T>>;

template<typename T, typename... Targs>
unique_pointer<T, MemoryArenaDeleter<T>>
make_unique(MemoryArena& arena, Targs&&... args)
{
    void* ptr = arena.alloc_align(sizeof(T), alignof(T));
    return unique_pointer<T, MemoryArenaDeleter<T>>{
        ptr ? new (ptr) T{ std::forward<Targs>(args)... } : nullptr,
        MemoryArenaDeleter<T>{ &arena },
    };
}

}
