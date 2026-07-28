#include "ring_deque.hpp"

template <typename T, u8 SizeLog2>
bool TRingDeque<T, SizeLog2>::PushOwner(T* t)
{
    i64 bVal = m_Bottom.load(std::memory_order_relaxed);
    i64 tVal = m_Top.load(std::memory_order_acquire);

    if (bVal - tVal >= s_Size) return false;

    m_Buffer[bVal & s_IndexMask] = t;

    m_Bottom.store(bVal + 1, std::memory_order_release);
    return true;
}

template <typename T, u8 SizeLog2>
T* TRingDeque<T, SizeLog2>::PopOwner()
{
    i64 bVal = m_Bottom.load(std::memory_order_relaxed) - 1;
    m_Bottom.store(bVal, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    i64 tVal = m_Top.load(std::memory_order_relaxed);

    if (tVal <= bVal)
    {
        T* t = m_Buffer[bVal & s_IndexMask];
        if (tVal != bVal) return t;

        i64 expected = tVal;
        bool bSuccess = m_Top.compare_exchange_strong(
            expected,
            tVal + 1,
            std::memory_order_seq_cst,
            std::memory_order_relaxed
        );

        m_Bottom.store(bVal + 1, std::memory_order_relaxed);
        return bSuccess ? t : nullptr;
    }

    m_Bottom.store(bVal + 1, std::memory_order_relaxed);
    return nullptr;
}

template <typename T, u8 SizeLog2>
T* TRingDeque<T, SizeLog2>::Steal()
{
    i64 tVal = m_Top.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    i64 bVal = m_Bottom.load(std::memory_order_acquire);

    if (tVal < bVal)
    {
        T* t = m_Buffer[tVal & s_IndexMask];
        i64 expected = tVal;
        bool bSuccess = m_Top.compare_exchange_strong(
            expected,
            tVal + 1,
            std::memory_order_seq_cst,
            std::memory_order_relaxed
        );

        return bSuccess ? t : nullptr;
    }
    return nullptr;
}