#pragma once

#include "defines.hpp"
#include <atomic>
#include <new>

constexpr u64 CACHE_LINE_SIZE{ std::hardware_destructive_interference_size };

// Ring-buffer deque, owned by one worker thread, owner pushes/pops at bottom
// thieves CAS-steal at top
template <typename T, u8 SizeLog2 = 9>
class TRingDeque
{
    static_assert(SizeLog2 <= 16);
public:
    bool Push(T* t);
    T* PopOwner();
    T* Steal();

private:
    static constexpr u32 s_Size{ 1 << SizeLog2 };
    static constexpr u32 s_IndexMask{ s_Size - 1 };
    
    alignas(CACHE_LINE_SIZE) T* m_Buffer[s_Size]{};
    alignas(CACHE_LINE_SIZE) std::atomic<u64> m_Top{ 0 };
    alignas(CACHE_LINE_SIZE) std::atomic<u64> m_Bottom{ 0 };
};

#include "ring_deque.inl"