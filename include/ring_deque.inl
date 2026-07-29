#include "ring_deque.hpp"

template <typename T, u8 SizeLog2>
bool TRingDeque<T, SizeLog2>::PushOwner(T* t)
{
    // load using memory_order_relaxed since only the owner writes to bottom
    u64 bVal = m_Bottom.load(std::memory_order_relaxed);

    // load using memory_order_acqure to ensure we see the latest updates
    // from thieves who may have incremented it
    u64 tVal = m_Top.load(std::memory_order_acquire);

    // deque is full
    if (bVal - tVal >= s_Size) return false;

    m_Buffer[bVal & s_IndexMask] = t;

    // memory_order_release ensures that the write to the buffer
    // happens before the update to the bottom is visble to others
    m_Bottom.store(bVal + 1, std::memory_order_release);
    return true;
}

template <typename T, u8 SizeLog2>
T* TRingDeque<T, SizeLog2>::PopOwner()
{
    u64 bVal = m_Bottom.load(std::memory_order_relaxed);

    // check if empty
    u64 tVal = m_Top.load(std::memory_order_acquire);
    if (tVal >= bVal) return nullptr;

    bVal--;
    m_Bottom.store(bVal, std::memory_order_relaxed);

    // seq_cst fence ensures that the store to m_Bottom is globally
    // visible before we load m_Top
    std::atomic_thread_fence(std::memory_order_seq_cst);

    // reload m_Top
    tVal = m_Top.load(std::memory_order_relaxed);

    if (tVal <= bVal)
    {
        T* t{ nullptr };
        // check if t is not the last item in the buffer
        if (tVal != bVal)
        {
            t = m_Buffer[bVal & s_IndexMask];
            m_Buffer[bVal & s_IndexMask] = nullptr;
            return t;
        }

        // check if m_Top was incremented by a thief concurrently
        u64 expected = tVal;
        bool bSuccess = m_Top.compare_exchange_strong(
            expected,
            tVal + 1,
            std::memory_order_seq_cst,
            std::memory_order_relaxed
        );

        if (bSuccess)
        {
            t = m_Buffer[tVal & s_IndexMask];
            m_Buffer[tVal & s_IndexMask] = nullptr;
        }

        // restore m_Bottom to match the value stored in m_Top
        m_Bottom.store(tVal + 1, std::memory_order_relaxed);
        return t;
    }
    else
    {
        // if this is reached then a thief stole the item after we decremented bottom.
        // the buffer is empty
        m_Bottom.store(tVal, std::memory_order_relaxed);
        return nullptr;
    }
}

template <typename T, u8 SizeLog2>
T* TRingDeque<T, SizeLog2>::Steal()
{
    u64 tVal = m_Top.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    u64 bVal = m_Bottom.load(std::memory_order_acquire);

    if (tVal < bVal)
    {
        u64 expected = tVal;
        bool bSuccess = m_Top.compare_exchange_strong(
            expected,
            tVal + 1,
            std::memory_order_seq_cst,
            std::memory_order_relaxed
        );

        if (bSuccess)
        {
            T* t = m_Buffer[tVal & s_IndexMask];
            m_Buffer[tVal & s_IndexMask] = nullptr;
            return t;
        }
    }
    return nullptr;
}