#ifndef __SPSC_VARIANT_QUEUE_H__
#define __SPSC_VARIANT_QUEUE_H__

#include "common.h"

namespace LockFreeQueue
{

class SPSCVariantQueue
{
    struct Entry
    {
        uint32_t uMagic;
        uint32_t uLength;
        uint8_t pData[1];

        inline uint32_t GetEntrySize() const
        {
            return ALIGN8(uLength + offsetof(Entry, pData));
        }

        inline static uint32_t CalEntrySize(uint32_t uSize)
        {
            return ALIGN8(uSize + offsetof(Entry, pData));
        }

        inline static uint32_t GetMinSize()
        {
            return CalEntrySize(0);
        }

        static Entry *ToEntry(void *pData)
        {
            return reinterpret_cast<Entry *>(
                    reinterpret_cast<uint8_t *>(pData) - GetMinSize());
        }
    };

public:
    SPSCVariantQueue() = default;

    SPSCVariantQueue(const SPSCVariantQueue&) = delete;
    SPSCVariantQueue& operator=(const SPSCVariantQueue&) = delete;
    SPSCVariantQueue(SPSCVariantQueue&&) = delete;
    SPSCVariantQueue& operator=(SPSCVariantQueue&&) = delete;
    
    ~SPSCVariantQueue()
    {
    }

    int32_t Init(uint64_t uMemSizeKb)
    {
        if (uMemSizeKb == 0)
        {
            return ErrorCode::kQueueError;
        }

        auto uNewSizeBytes = PowerOf2Ceil(uMemSizeKb * 1024);
        auto ptr = malloc(uNewSizeBytes);
        if (ptr == nullptr)
        {
            return ErrorCode::kQueueError;
        }

        m_pDatap = reinterpret_cast<uint8_t *>(ptr);
        m_uSizep =uNewSizeBytes;
        m_uTail = 0;
        m_uHeadRef = 0;
        m_statisp.Reset();

        m_pDatac = reinterpret_cast<uint8_t *>(ptr);
        m_uSizec = uNewSizeBytes;
        m_uHead = 0;
        m_uTailRef = 0;
        m_statisc.Reset();

        return 0;
    }

    uint8_t *NewEntry(uint32_t uSize)
    {
        assert (uSize < m_uSizep);
        auto uEntrySize = Entry::CalEntrySize(uSize);
        auto uNewSize = ALIGN8(uEntrySize);

        auto pEntry = NewEntryRef(uNewSize);
        if (likely(pEntry != nullptr))
        {
            m_statisp.uNewEntryCount++;
            pEntry->uLength = uSize;
            return pEntry->pData;
        }

        m_uHeadRef = ACCESS_ONCE(m_uHead);
        pEntry = NewEntryRef(uNewSize);
        if (likely(pEntry != nullptr))
        {
            m_statisp.uNewEntryCount++;
            pEntry->uLength = uSize;
            return pEntry->pData;
        }

        m_statisp.uNewEntryFailCount++;
        return nullptr;
    }

    void PostEntry(uint8_t *pData)
    {
        auto pEntry = Entry::ToEntry(pData);
        assert(pEntry->uMagic == kAvailableMagic);
        std::atomic_thread_fence(std::memory_order_release);
        m_statisp.uPostEntryCount++;
        m_uTail += pEntry->GetEntrySize();
    }

    uint8_t *GetEntry(uint32_t &uSize)
    {
        auto pEntry = GetEntryRef();
        if (likely(pEntry != nullptr))
        {
            uSize = pEntry->uLength;
            m_statisc.uGetEntryCount++;
            return pEntry->pData;
        }

        m_uTailRef = ACCESS_ONCE(m_uTail);
        pEntry = GetEntryRef();
        if (likely(pEntry != nullptr))
        {
            uSize = pEntry->uLength;
            m_statisc.uGetEntryCount++;
            return pEntry->pData;
        }

        m_statisc.uGetEntryFailCount++;
        return nullptr;
    }

    void FreeEntry(uint8_t *pData)
    {
        auto pEntry = Entry::ToEntry(pData);
        assert(pEntry->uMagic == kAvailableMagic);
        std::atomic_thread_fence(std::memory_order_acquire);
        m_statisc.uFreeEntryCount++;
        m_uHead += pEntry->GetEntrySize();
    }

    int32_t Push(const void *pData, uint32_t uLength)
    {
        auto pEntry = NewEntry(uLength);
        if (likely(pEntry != nullptr))
        {
            memcpy(pEntry, pData, uLength);
            PostEntry(pEntry);
            return 0;
        }

        return ErrorCode::kQueueFull;
    }

    int32_t Pop(void *pData, uint32_t &uLength)
    {
        uint32_t uGetLen = 0;
        auto pEntry = GetEntry(uGetLen);
        if (likely(pEntry != nullptr))
        {
            if (likely(uLength >= uGetLen))
            {
                memcpy(pData, pEntry, uGetLen);
                uLength = uGetLen;
                FreeEntry(pEntry);
                return 0;
            }
            
            return ErrorCode::kQueueError;
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
        // FIXME: empty maybe not equal!
        return m_uHead == m_uTail;
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
    Entry *NewEntryRef(uint32_t uNewSize)
    {
        if (unlikely(m_uTail - m_uHeadRef >= m_uSizep))
        {
            return nullptr;
        }

        auto uTail = m_uTail & (m_uSizep - 1);
        auto uHead = m_uHeadRef & (m_uSizep - 1);

        /**
         *             head              tail
         * _____________|_________________|_________
        */
       if (uTail >= uHead)
       {
           /**
            *               head              tail
            * _______________|_________________|_________
            *                                   ______
           */
            if (likely(uTail + uNewSize <= m_uSizep))
            {
                auto pEntry = reinterpret_cast<Entry *>(&m_pDatap[uTail]);
                pEntry->uMagic = kAvailableMagic;
                return pEntry;
            }
            
            /** change tail
             * tail             head
             * |_______________|______________________
            */
            if (m_uSizep - uTail >= Entry::GetMinSize())
            {
                auto pEntry = reinterpret_cast<Entry *>(&m_pDatap[uTail]);
                pEntry->uMagic = kPlaceholdMagic;
                pEntry->uLength = m_uSizep - uTail;
            }
            std::atomic_thread_fence(std::memory_order_release);
            m_uTail |= (m_uSizep - 1);
            uTail = 0;

            /** 
             * tail             head
             * |_______________|______________________
             *  _______
            */
            if (uTail + uNewSize <= uHead)
            {
                auto pEntry = reinterpret_cast<Entry *>(&m_pDatap[uTail]);
                pEntry->uMagic = kAvailableMagic;
                return pEntry;
            }
       }
       /**
        *             tail             head
        * _____________|________________|_________
       */
       else
       {
           /**
            *              tail             head
            *  _____________|_______________|______________________
                             _______
           */
           if (likely(uTail + uNewSize <= uHead))
           {
               auto pEntry = reinterpret_cast<Entry *>(&m_pDatap[uTail]);
               pEntry->uMagic = kAvailableMagic;
               return pEntry;
           }
       }

       return nullptr;
    }

    Entry *GetEntryRef()
    {
        if (unlikely(m_uHead >= m_uTailRef))
        {
            return nullptr;
        }

        auto uHead = m_uHead & (m_uSizec - 1);
        auto uTail = m_uTailRef & (m_uSizec - 1);

        /**
         *             head              tail
         * _____________|_________________|_________
        */
        if (uHead < uTail)
        {
            return reinterpret_cast<Entry *>(&m_pDatac[uHead]);
        }
        /**
        *             tail             head
        * _____________|________________|_________
       */
        else if (uHead >= uTail)
        {
            /**
             *             tail             head
             * _____________|________________|__________
             *                                ____
            */
            auto pEntry = reinterpret_cast<Entry *>(&m_pDatac[uHead]);
            if (unlikely(m_uSizec - uHead > Entry::GetMinSize() 
                        && pEntry->uMagic != kPlaceholdMagic))
            {
                return pEntry;
            }
            
            /** change tail
             * head            tail
             *  |_______________|______________________
             *   _____
            */
            std::atomic_thread_fence(std::memory_order_acquire);
            m_uHead |= (m_uSizec - 1);
            uHead = 0;
            if (uHead < uTail)
            {
                return reinterpret_cast<Entry *>(&m_pDatac[0]);
            }
        }

        return nullptr;
    }

private:
    ALIGN_AS_CACHELINE uint8_t *m_pDatap{nullptr};
    uint64_t m_uSizep{0};
    uint64_t m_uTail{0};
    uint64_t m_uHeadRef{0};
    ProducerStatis m_statisp;

    ALIGN_AS_CACHELINE uint8_t *m_pDatac{nullptr};
    uint64_t m_uSizec{0};
    uint64_t m_uHead{0};
    uint64_t m_uTailRef{0};
    ConsumerStatis m_statisc;
};

}

#endif // __SPSC_VARIANT_QUEUE_H__
