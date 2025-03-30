#ifndef __OS_COMMON_H__
#define __OS_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>

#include <new>
#include <atomic>
#include <thread>

#if defined(_WIN32) || defined(_WIN64)
#define OS_WINDOWS
#elif defined(__APPLE__)
#define OS_MAC
#else
#define OS_LINUX
#endif

#ifndef OS_WINDOWS
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#define UNSED(x) ((void)(x))

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN8(x) ALIGN(x, 8)
#define ALIGN16(x) ALIGN(x, 16)
#define ALIGN32(x) ALIGN(x, 32)
#define ALIGN64(x) ALIGN(x, 64)

#define ALIGN_AS_CACHELINE __attribute__((aligned(LockFreeQueue::kCacheLineSize)))

#define ACCESS_ONCE(x) (*(volatile decltype(x) *)&(x))

namespace LockFreeQueue
{

enum ErrorCode
{
    kQueueFull = 1,
    kQueueEmpty = 2,
    kQueueError = 3,
};

constexpr uint32_t kAvailableMagic = 0x7F7F7F7F;
constexpr uint32_t kPlaceholdMagic = 0xF7F7F7F7;

constexpr uint64_t kCacheLineSize = 64;

struct Entry
{
    uint32_t uMagic;
    uint32_t uLength;
    uint8_t pData[1];

    inline uint32_t GetEntrySize() const
    {
        return CalEntrySize(uLength);
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

template <typename T>
struct Node
{
    ALIGN_AS_CACHELINE uint32_t uSizep{0};
    uint32_t uTail{0};

    ALIGN_AS_CACHELINE uint32_t uSizec{0};
    uint32_t uHead{0};
    uint32_t uTailRef{0};

    Node* pNext{nullptr};
    T *pData[1];

    static inline uint32_t GetNodeSize()
    {
        return CalNodeSize(uSizep);
    }

    static uint32_t CalNodeSize(uint32_t uSize)
    {
        return offsetof(Node, pData) + sizeof(T) * (uSize);
    }

    static inline uint32_t GetMinNodeSize()
    {
        return CalNodeSize(0);
    }
};

struct ProducerStatis
{
    uint64_t uNewEntryCount{0};
    uint64_t uPostEntryCount{0};
    uint64_t uNewEntryFailCount{0};

    void Reset()
    {
        uNewEntryCount = 0;
        uPostEntryCount = 0;
        uNewEntryFailCount = 0;
    }
};

struct ConsumerStatis
{
    uint64_t uGetEntryCount{0};
    uint64_t uFreeEntryCount{0};
    uint64_t uGetEntryFailCount{0};

    void Reset()
    {
        uGetEntryCount = 0;
        uFreeEntryCount = 0;
        uGetEntryFailCount = 0;
    }
};

inline uint64_t PowerOf2Ceil(uint64_t x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    x++;
    return x;
}

}

#endif // __OS_COMMON_H__
