#include "memory_pool.hpp"
#include <string.h>

namespace
{
    constexpr u64 CHUNK_HEADER_SIZE{ sizeof(u8*) };
}

bool MemoryPool::Init(u32 chunkSize, u32 numChunks)
{
    if(m_ppRawMemoryArray) Shutdown();

    m_ChunkSize = chunkSize;
    m_NumChunks = numChunks;

    if(Grow()) return true;
    return false;
}

void MemoryPool::Shutdown()
{
    if(m_ppRawMemoryArray)
    {
        for(u32 i = 0; i < m_MemArraySize; i++)
        {
            free(m_ppRawMemoryArray[i]);
        }
        free(m_ppRawMemoryArray);
    }

    Reset();
}

void* MemoryPool::Alloc()
{
    if(!m_pHead)
    {
        if(!m_CanResize) return nullptr;
        if(!Grow()) return nullptr;
    }

    u8* pRet = m_pHead;
    m_pHead = GetNext(m_pHead);
    return (pRet + CHUNK_HEADER_SIZE);
}

void MemoryPool::Free(void* pMem)
{
    if(pMem)
    {
        u8* pBlock = ((u8*)pMem) - CHUNK_HEADER_SIZE;

        SetNext(pBlock, m_pHead);
        m_pHead = pBlock;
    }
}

void MemoryPool::Reset()
{
    m_ppRawMemoryArray =  nullptr;
    m_pHead =  nullptr;
    m_ChunkSize =  0;
    m_NumChunks =  0;
    m_MemArraySize =  0;
    m_CanResize =  true;
}

bool MemoryPool::Grow()
{
    u64 allocSize = (u64)(sizeof(u8*) * (m_MemArraySize + 1));
    u8** ppNewMemArray = (u8**)malloc(allocSize);

    if(!ppNewMemArray) return false;

    for(u32 i = 0; i < m_MemArraySize; i++)
    {
        ppNewMemArray[i] = m_ppRawMemoryArray[i];
    }
    
    ppNewMemArray[m_MemArraySize] = AllocateNewBlock();

    if(m_pHead)
    {
        u8* pCurr = m_pHead;
        u8* pNext = GetNext(m_pHead);
        while(pNext)
        {
            pCurr = pNext;
            pNext = GetNext(pNext);
        }
        SetNext(pCurr, ppNewMemArray[m_MemArraySize]);
    }
    else
    {
        m_pHead = ppNewMemArray[m_MemArraySize];
    }

    if(m_ppRawMemoryArray) free(m_ppRawMemoryArray);

    m_ppRawMemoryArray = ppNewMemArray;
    m_MemArraySize++;

    return true;
}

u8* MemoryPool::AllocateNewBlock()
{
    u64 blockSize = m_ChunkSize + CHUNK_HEADER_SIZE;
    u64 trueSize = blockSize * m_NumChunks;

    u8* pNewMem = (u8*)malloc(trueSize);
    if(!pNewMem) return nullptr;

    u8* pEnd = pNewMem + trueSize;
    u8* pCurr = pNewMem;
    while(pCurr < pEnd)
    {
        u8* pNext = pCurr + blockSize;
        u8** ppChunkHeader = (u8**)pCurr;
        ppChunkHeader[0] = (pNext < pEnd ? pNext : nullptr);

        pCurr += blockSize;
    }

    return pNewMem;
}

u8* MemoryPool::GetNext(u8* pBlock)
{
    u8** ppChunkHeader = (u8**)pBlock;
    return ppChunkHeader[0];
}

void MemoryPool::SetNext(u8* pBlockToChange, u8* pNewNext)
{
    u8** ppChunkHeader = (u8**)pBlockToChange;
    ppChunkHeader[0] = pNewNext;
}