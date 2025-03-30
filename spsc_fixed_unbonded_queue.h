#ifndef __LOCK_FREE_QUEUE_SPSC_UNBOUNDED_H__
#define __LOCK_FREE_QUEUE_SPSC_UNBOUNDED_H__

#include <type_traits>
#include "queue_common.h"

namespace LockFreeQueue
{

template <typename T>
class SPSCUnboundedQueue
{
public:
    SPSCUnboundedQueue() = default;
    
    SPSCUnboundedQueue(const SPSCUnboundedQueue&) = delete;
    SPSCUnboundedQueue& operator=(const SPSCUnboundedQueue&) = delete;
    SPSCUnboundedQueue(SPSCUnboundedQueue&&) = delete;
    SPSCUnboundedQueue& operator=(SPSCUnboundedQueue&&) = delete;

    ~SPSCUnboundedQueue()
    {
        auto pNode = m_pHead;
        while (pNode != nullptr)
        {
            Node* pNext = pNode->pNext;
            FreeNode(pNode);
            pNode = pNext;
        }

        m_pHead = nullptr;
        m_pTail = nullptr;
    }

    int32_t Init(uint32_t uNodeSize = 1024)
    {
        if (uNodeSize == 0)
        {
            return ErrorCode::kQueueError;
        }

        m_uNodeSize = uNodeSize;
        auto pNode = NewNode();
        if (pNode == nullptr)
        {
            return ErrorCode::kQueueError;
        }
        
        m_pHead = pNode;
        m_pTail = pNode;

        return 0;
    }

    T *NewEntry()
    {
        if (likely(m_pTail->header.uTail < m_pTail->header.uSizep))
        {
            m_statisp.uNewEntryCount++;
            return &m_pTail->pData[m_pTail->header.uTail];
        }

        Node* pNode = NewNode();
        if (pNode != nullptr)
        {
            m_pTail->pNext = pNode;
            m_pTail = pNode;
            m_statisp.uNewEntryCount++;
            return &m_pTail->pData[0];
        }

        m_statisp.uNewEntryFailCount++;
        return nullptr;
    }

    void PostEntry(T* pEntry)
    {
        UNSED(pEntry);
        assert(pEntry == &m_pTail->pData[m_pTail->header.uTail]);
        std::atomic_thread_fence(std::memory_order_release);
        m_statisp.uPostEntryCount++;
        m_pTail->header.uTail++;
    }

    T *GetEntry()
    {
        if (likely(m_pHead->header.uHead < m_pHead->header.uTailRef))
        {
            m_statisc.uGetEntryCount++;
            return &m_pHead->pData[m_pHead->header.uHead];
        }

        m_pHead->header.uTailRef = ACCESS_ONCE(m_pHead->header.uTail);
        if (likely(m_pHead->header.uHead < m_pHead->header.uTailRef))
        {
            m_statisc.uGetEntryCount++;
            return &m_pHead->pData[m_pHead->header.uHead];
        }

        if (likely(m_pHead->header.uHead == m_pHead->header.uSizec
                    && m_pHead->pNext != nullptr))
        {
            auto pNext = m_pHead->pNext;
            FreeNode(m_pHead);
            m_pHead = pNext;
            m_statisc.uGetEntryCount++;
            return &m_pHead->pData[0];
        }

        m_statisc.uGetEntryFailCount++;
        return nullptr;
    }

    void FreeEntry(T* pEntry)
    {
        UNSED(pEntry);
        assert(pEntry == &m_pHead->pData[m_pHead->header.uHead]);
        std::atomic_thread_fence(std::memory_order_acquire);
        m_statisc.uFreeEntryCount++;
        m_pHead->header.uHead++;
    }

    template <typename V>
    int32_t Push(V&& t)
    {
        auto pEntry = NewEntry();
        if (likely(pEntry != nullptr))
        {
            try
            {
                *pEntry = std::forward<V>(t);
            }
            catch (...)
            {
                m_statisp.uNewEntryFailCount++;
                return ErrorCode::kQueueError;
            }

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
            try
            {
                t = std::move(*pEntry);
            }
            catch (...)
            {
                m_statisc.uGetEntryFailCount++;
                return ErrorCode::kQueueError;
            }

            FreeEntry(pEntry);
            return 0;
        }

        return ErrorCode::kQueueEmpty;
    }
    
    uint64_t GetSize() const
    {
        return ACCESS_ONCE(m_statisp.uPostEntryCount) 
                - ACCESS_ONCE(m_statisc.uGetEntryCount);
    }

    bool IsEmpty() const
    {
        return GetSize() == 0;
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
    Node *NewNode()
    {
        auto uNodeSize = Node::CalNodeSize(m_uNodeSize);
        auto ptr = malloc(sizeof(Node) + sizeof(T) * (m_uNodeSize - 1));
        if (ptr == nullptr)
        {
            m_uNewBlockFailedCount++;
            return nullptr;
        }

        auto pNode = (Node*)ptr;
        pNode->header.uSizep = m_uNodeSize;
        pNode->header.uTail = 0;
        pNode->header.uSizec = m_uNodeSize;
        pNode->header.uHead = 0;
        pNode->header.uTailRef = 0;
        
        pNode->pNext = nullptr;

        if (!std::is_trivial<T>::value)
        {
            try
            {
                new(pNode->pData) T[m_uNodeSize];
            }
            catch (...)
            {
                m_uNewBlockFailedCount++;
                free(ptr);
                return nullptr;
            }
        }

        m_uNewBlockCount++;
        return pNode;
    }

    void FreeNode(Node* pNode)
    {
        if (!std::is_trivial<T>::value)
        {
            auto pData = reinterpret_cast<T *>(pNode->GetData());
            for (uint32_t i = 0; i < pNode->uSizep; i++)
            {
                pData[i].~T();
            }
        }

        free(pNode);
        m_uFreeBlockCount--;
    }

private:
    uint32_t m_uNodeSize{0};
    uint64_t m_uNewBlockCount{0};
    uint64_t m_uFreeBlockCount{0};
    uint64_t m_uNewBlockFailedCount{0};

    ALIGN_AS_CACHELINE Node* m_pTail{nullptr};
    ProducerStatis m_statisp;

    ALIGN_AS_CACHELINE Node* m_pHead{nullptr};
    ConsumerStatis m_statisc;
};

}

#endif // __LOCK_FREE_QUEUE_SPSC_UNBOUNDED_H__