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

constexpr uint64_t kCacheLineSize = 64;

enum ErrorCode
{
    kQueueFull = 1,
    kQueueEmpty = 2,
    kQueueError = 3,
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
