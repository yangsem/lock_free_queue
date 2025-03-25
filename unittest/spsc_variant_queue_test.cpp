#include "../spsc_variant_queue.h"
#include <gtest/gtest.h>
#include <time.h>

using namespace LockFreeQueue;

TEST(SPSPVariantQueue, TestInit)
{
    SPSCVariantQueue queue;
    ASSERT_EQ(queue.Init(0), ErrorCode::kQueueError);
    ASSERT_EQ(queue.Init(1), 0);
}

TEST(SPSPVariantQueue, TestNewGetEntry)
{
    SPSCVariantQueue queue;
    ASSERT_EQ(queue.Init(1), 0);
    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));

    char data[] = "abcdefg1234567890";

    auto pEntry = queue.NewEntry(sizeof(data));
    ASSERT_NE(pEntry, nullptr);
    memcpy(pEntry, data, sizeof(data));
    queue.PostEntry(pEntry);

    pEntry = queue.NewEntry(sizeof(data));
    ASSERT_NE(pEntry, nullptr);
    memcpy(pEntry, data, sizeof(data));
    queue.PostEntry(pEntry);

    ASSERT_FALSE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(2));

    uint32_t uLength = 0;
    pEntry = queue.GetEntry(uLength);
    ASSERT_NE(pEntry, nullptr);
    ASSERT_EQ(uLength, sizeof(data));
    ASSERT_EQ(memcmp(data, pEntry, sizeof(data)), 0);
    queue.FreeEntry(pEntry);

    pEntry = queue.GetEntry(uLength);
    ASSERT_NE(pEntry, nullptr);
    ASSERT_EQ(uLength, sizeof(data));
    ASSERT_EQ(memcmp(data, pEntry, sizeof(data)), 0);
    queue.FreeEntry(pEntry);

    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));
}

TEST(SPSPVariantQueue, TestPushPop)
{
    SPSCVariantQueue queue;
    ASSERT_EQ(queue.Init(1), 0);
    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));

    char data[] = "abcdefg1234567890";
    ASSERT_EQ(queue.Push(data, sizeof(data)), 0);
    ASSERT_EQ(queue.Push(data, sizeof(data)), 0);

    ASSERT_FALSE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(2));

    char buffer[32];
    auto pEntry = (void *)&buffer[0];

    uint32_t uLength = sizeof(buffer);
    ASSERT_EQ(queue.Pop(pEntry, uLength), 0);
    ASSERT_EQ(uLength, sizeof(data));
    ASSERT_EQ(memcmp(data, pEntry, sizeof(data)), 0);

    uLength = sizeof(buffer);
    ASSERT_EQ(queue.Pop(pEntry, uLength), 0);
    ASSERT_EQ(uLength, sizeof(data));
    ASSERT_EQ(memcmp(data, pEntry, sizeof(data)), 0);

    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));
}

TEST(SPSCFixedQueueTest, TestQueueFull)
{
    SPSCVariantQueue queue;
    ASSERT_EQ(queue.Init(1), 0);
    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));

    char data[246] = "abcdefg1234567890";
    ASSERT_EQ(queue.Push(data, sizeof(data)), 0);
    ASSERT_EQ(queue.Push(data, sizeof(data)), 0);
    ASSERT_EQ(queue.Push(data, sizeof(data)), 0);
    ASSERT_EQ(queue.Push(data, sizeof(data)), 0);

    ASSERT_EQ(queue.Push(data, sizeof(data)), ErrorCode::kQueueFull);

    ASSERT_EQ(queue.GetSize(), uint64_t(4));
}

TEST(SPSCFixedQueueTest, TestQueueEmpty)
{
    SPSCVariantQueue queue;
    ASSERT_EQ(queue.Init(1), 0);
    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));

    char buffer[32];
    uint32_t uLength = sizeof(buffer);
    auto pEntry = (void *)&buffer[0];
    ASSERT_EQ(queue.Pop(pEntry, uLength), ErrorCode::kQueueEmpty);
}

TEST(SPSCFixedQueueTest, TestStatistics)
{
    SPSCVariantQueue queue;
    ASSERT_EQ(queue.Init(1), 0);

    ProducerStatis prodStatis;
    ConsumerStatis consStatis;

    queue.GetStatis(prodStatis, consStatis);
    ASSERT_EQ(prodStatis.uNewEntryCount, uint64_t(0));
    ASSERT_EQ(consStatis.uGetEntryCount, uint64_t(0));

    char data[246] = "abcdefg1234567890";
    ASSERT_EQ(queue.Push(data, sizeof(data)), 0);
    ASSERT_EQ(queue.Push(data, sizeof(data)), 0);
    ASSERT_EQ(queue.Push(data, sizeof(data)), 0);
    ASSERT_EQ(queue.Push(data, sizeof(data)), 0);
    ASSERT_EQ(queue.Push(data, sizeof(data)), ErrorCode::kQueueFull);

    queue.GetStatis(prodStatis, consStatis);
    ASSERT_EQ(prodStatis.uNewEntryCount, uint64_t(4));
    ASSERT_EQ(prodStatis.uNewEntryFailCount, uint64_t(1));
    ASSERT_EQ(consStatis.uGetEntryCount, uint64_t(0));

    char buffer[256];
    auto pEntry = (void *)&buffer[0];

    uint32_t uLength = sizeof(buffer);
    ASSERT_EQ(queue.Pop(pEntry, uLength), 0);
    ASSERT_EQ(uLength, sizeof(data));
    ASSERT_EQ(memcmp(data, pEntry, sizeof(data)), 0);

    uLength = sizeof(buffer);
    ASSERT_EQ(queue.Pop(pEntry, uLength), 0);
    ASSERT_EQ(uLength, sizeof(data));
    ASSERT_EQ(memcmp(data, pEntry, sizeof(data)), 0);

    uLength = sizeof(buffer);
    ASSERT_EQ(queue.Pop(pEntry, uLength), 0);
    ASSERT_EQ(uLength, sizeof(data));
    ASSERT_EQ(memcmp(data, pEntry, sizeof(data)), 0);

    uLength = sizeof(buffer);
    ASSERT_EQ(queue.Pop(pEntry, uLength), 0);
    ASSERT_EQ(uLength, sizeof(data));
    ASSERT_EQ(memcmp(data, pEntry, sizeof(data)), 0);

    uLength = sizeof(buffer);
    ASSERT_EQ(queue.Pop(pEntry, uLength), ErrorCode::kQueueEmpty);

    queue.GetStatis(prodStatis, consStatis);
    ASSERT_EQ(prodStatis.uNewEntryCount, uint64_t(4));
    ASSERT_EQ(prodStatis.uNewEntryFailCount, uint64_t(1));
    ASSERT_EQ(consStatis.uGetEntryCount, uint64_t(4));
    ASSERT_EQ(consStatis.uFreeEntryCount, uint64_t(4));
    ASSERT_EQ(consStatis.uGetEntryFailCount, uint64_t(1));
}

TEST(SPSCFixedQueueTest, TestQueueRing)
{
    SPSCVariantQueue queue;
    ASSERT_EQ(queue.Init(1), 0);

    char data[118] = "abcdefg1234567890";
    char buffer[128];
    uint32_t uLength = sizeof(buffer);

    for (uint32_t i = 0; i < 7; i++)
    {
        ASSERT_EQ(queue.Push(data, sizeof(data)), 0);
        ASSERT_EQ(queue.Push(data, sizeof(data)), 0);
        uLength = sizeof(buffer);
        auto pEntry = (void *)&buffer[0];
        ASSERT_EQ(queue.Pop(pEntry, uLength), 0);
        ASSERT_EQ(uLength, sizeof(data));
        ASSERT_EQ(memcmp(data, pEntry, sizeof(data)), 0);
    }

    std::string data2(246, 'x');
    ASSERT_EQ(queue.Push(data2.c_str(), data2.length()), ErrorCode::kQueueFull);

    for (uint32_t i = 0; i < 7; i++)
    {
        uLength = sizeof(buffer);
        auto pEntry = (void *)&buffer[0];
        ASSERT_EQ(queue.Pop(pEntry, uLength), 0);
        ASSERT_EQ(uLength, sizeof(data));
        ASSERT_EQ(memcmp(data, pEntry, sizeof(data)), 0);
    }

    char buffer2[128];
    uLength = sizeof(buffer2);
    auto pEntry = (void *)&buffer2[0];
    ASSERT_EQ(queue.Pop(pEntry, uLength), ErrorCode::kQueueEmpty);
}

TEST(SPSCFixedQueueTest, TestPushPopWithThread)
{
    SPSCVariantQueue queue;
    ASSERT_EQ(queue.Init(8192), 0);

    char data[256];
    for (uint32_t i = 0; i < sizeof(data); i++)
    {
        data[i] = 'A' + (i % 26);
    }

    volatile bool is_running = false;
    std::thread producer([&]() {
        while (!is_running)
        {
            usleep(0);
        }

        for (int i = 0; i < 1000000; ++i)
        {
            timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            uint32_t uLength = (ts.tv_sec * 17 + ts.tv_nsec * 17) % 128;
            while (queue.Push(data, uLength) != 0)
            {
                usleep(0);
            }
        }
    });

    std::thread consumer([&]() {
        while (!is_running)
        {
            usleep(0);
        }

        char output[256];
        auto pEntry = (void *)&output[0];
        for (int i = 0; i < 1000000; ++i)
        {
            uint32_t uLength = sizeof(output);
            while (queue.Pop(pEntry, uLength) != 0)
            {
                usleep(0);
            }

            ASSERT_EQ(memcmp(data, pEntry, uLength), 0);
        }
    });

    is_running = true;
    producer.join();
    consumer.join();

    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));

    ProducerStatis prodStatis;
    ConsumerStatis consStatis;

    queue.GetStatis(prodStatis, consStatis);
    ASSERT_EQ(prodStatis.uNewEntryCount, uint64_t(1000000));
    ASSERT_EQ(prodStatis.uPostEntryCount, uint64_t(1000000));
    ASSERT_EQ(consStatis.uGetEntryCount, uint64_t(1000000));
    ASSERT_EQ(consStatis.uFreeEntryCount, uint64_t(1000000));
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}