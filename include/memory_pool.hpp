#pragma once

#include "defines.hpp"

// game coding complete fourth edition

class MemoryPool
{
public:
    DISABLE_COPY(MemoryPool);

    MemoryPool() { Reset(); }
    ~MemoryPool() { Shutdown(); }

    bool Init(u32 chunkSize, u32 numChunks);
    void Shutdown();

    void* Alloc();
    void Free(void* pMem);
    
    inline u32 ChunkSize() const { return m_ChunkSize; }
    inline bool CanResize() const { return m_CanResize; }
    inline void SetCanResize(bool flag) { m_CanResize = flag; }

private:
    void Reset();
    bool Grow();
    u8* AllocateNewBlock();

    u8* GetNext(u8* pBlock);
    void SetNext(u8* pBlockToChange, u8* pNewNext);

private:
    u8** m_ppRawMemoryArray{ nullptr };
    u8* m_pHead{ nullptr };
    u32 m_ChunkSize{ 0 };
    u32 m_NumChunks{ 0 };
    u32 m_MemArraySize{ 0 };
    bool m_CanResize{ true };
};