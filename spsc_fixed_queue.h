#ifndef __LOCK_FREE_QUEUE_SPSC_H__
#define __LOCK_FREE_QUEUE_SPSC_H__

#include "common.h"

namespace LockFreeQueue
{

template <typename T>
class SPSCFixedQueue
{
public:
    SPSCFixedQueue() = default;
    ~SPSCFixedQueue()
    {
        if (m_pDatap != nullptr)
        {
            delete [] m_pDatap;
            m_pDatap = nullptr;
            m_pDatac = nullptr;
        }
    }

    int32_t Init(uint64_t uSize)
    {
        if (uSize == 0)
        {
            return ErrorCode::kQueueError;
        }

        auto uNewSize = PowerOf2Ceil(uSize);
        auto pData = new(std::nothrow) T[uNewSize];
        if (pData == nullptr)
        {
            return ErrorCode::kQueueError;
        }

        m_pDatap = pData;
        m_uSizep = uNewSize;
        m_uMaskp = uNewSize - 1;
        m_uTail = 0;
        m_uHeadRef = 0;
        m_statisp.Reset();

        m_pDatac = pData;
        m_uSizec = uNewSize;
        m_uMaskc = uNewSize - 1;
        m_uTail = 0;
        m_uHead = 0;
        m_uTailRef = 0;
        m_statisc.Reset();

        return 0;
    }

    T *NewEntry()
    {
        if (likely(m_uTail - m_uHeadRef < m_uSizep))
        {
            m_statisp.uNewEntryCount++;
            return &m_pDatap[m_uTail & m_uMaskp];
        }
        
        m_uHeadRef = m_uHead;
        if (likely(m_uTail - m_uHeadRef < m_uSizep))
        {
            m_statisp.uNewEntryCount++;
            return &m_pDatap[m_uTail & m_uMaskp];
        }
        
        m_statisp.uNewEntryFailCount++;
        return nullptr;
    }

    void PostEntry(T *pEntry)
    {
        std::atomic_thread_fence(std::memory_order_release);
        m_statisp.uPostEntryCount++;
        m_uTail++;
    }

    T *GetEntry()
    {
        if (likely(m_uHead < m_uTailRef))
        {
            m_statisc.uGetEntryCount++;
            return &m_pDatac[m_uHead & m_uMaskc];
        }

        m_uTailRef = m_uTail;
        if (likely(m_uHead < m_uTailRef))
        {
            m_statisc.uGetEntryCount++;
            return &m_pDatac[m_uHead & m_uMaskc];
        }
        
        m_statisc.uGetEntryFailCount++;
        return nullptr;
    }

    void FreeEntry(T *pEntry)
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        m_statisc.uFreeEntryCount++;
        m_uHead++;
    }

    int32_t Push(const T& t)
    {
        auto pEntry = NewEntry();
        if (likely(pEntry != nullptr))
        {
            *pEntry = t;
            PostEntry(pEntry);
            return 0;
        }

        return ErrorCode::kQueueFull;
    }

    int32_t Pop(T& t)
    {
        auto pEntry = GetEntry();
        if (likely(pEntry != nullptr))
        {
            t = *pEntry;
            FreeEntry(pEntry);
            return 0;
        }

        return ErrorCode::kQueueEmpty;
    }

    bool IsEmpty() const
    {
        return ACCESS_ONCE(m_uTail) - ACCESS_ONCE(m_uHead) == 0;
    }

    uint64_t GetSize() const
    {
        return ACCESS_ONCE(m_uTail) - ACCESS_ONCE(m_uHead);
    }

    void GetStatis(ProducerStatis &statisp, ConsumerStatis &statisc) const
    {
        statisp = m_statisp;
        statisc = m_statisc;
    }

    void ClearStatis()
    {
        m_statisp.Reset();
        m_statisc.Reset();
    }

private:
    ALIGN_AS_CACHELINE T *m_pDatap{nullptr};
    uint64_t m_uSizep{0};
    uint64_t m_uMaskp{0};
    uint64_t m_uTail{0};
    uint64_t m_uHeadRef{0};
    ProducerStatis m_statisp;

    ALIGN_AS_CACHELINE T *m_pDatac{nullptr};
    uint64_t m_uSizec{0};
    uint64_t m_uMaskc{0};
    uint64_t m_uHead{0};
    uint64_t m_uTailRef{0};
    ConsumerStatis m_statisc;
};

}

#endif // __LOCK_FREE_QUEUE_SPSC_H__
