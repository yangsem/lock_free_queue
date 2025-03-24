#include "../spsc_fixed_queue.h"
#include <gtest/gtest.h>

using namespace LockFreeQueue;


TEST(SPSCFixedQueueTest, TestInit)
{
    SPSCFixedQueue<int> queue;
    ASSERT_EQ(queue.Init(0), ErrorCode::kQueueError);
    ASSERT_EQ(queue.Init(8), 0);
    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));
}

TEST(SPSCFixedQueueTest, TestNewGetEntry)
{
    SPSCFixedQueue<int> queue;
    queue.Init(4);

    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));

    int* entry = queue.NewEntry();
    ASSERT_NE(entry, static_cast<int*>(nullptr));
    *entry = 1;
    queue.PostEntry(entry);

    entry = queue.NewEntry();
    ASSERT_NE(entry, static_cast<int*>(nullptr));
    *entry = 2;
    queue.PostEntry(entry);

    ASSERT_FALSE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(2));

    entry = queue.GetEntry();
    ASSERT_NE(entry, static_cast<int*>(nullptr));
    ASSERT_EQ(*entry, 1);
    queue.FreeEntry(entry);

    entry = queue.GetEntry();
    ASSERT_NE(entry, static_cast<int*>(nullptr));
    ASSERT_EQ(*entry, 2);
    queue.FreeEntry(entry);

    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));
}

TEST(SPSCFixedQueueTest, TestPushPop)
{
    SPSCFixedQueue<int> queue;
    queue.Init(8);

    ASSERT_TRUE(queue.IsEmpty());

    ASSERT_EQ(queue.Push(1), 0);
    ASSERT_EQ(queue.Push(2), 0);
    ASSERT_EQ(queue.Push(3), 0);

    ASSERT_FALSE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(3));

    int value;
    ASSERT_EQ(queue.Pop(value), 0);
    ASSERT_EQ(value, 1);

    ASSERT_EQ(queue.Pop(value), 0);
    ASSERT_EQ(value, 2);

    ASSERT_EQ(queue.Pop(value), 0);
    ASSERT_EQ(value, 3);

    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));
}

TEST(SPSCFixedQueueTest, TestQueueFull)
{
    SPSCFixedQueue<int> queue;
    queue.Init(4);

    ASSERT_EQ(queue.Push(1), 0);
    ASSERT_EQ(queue.Push(2), 0);
    ASSERT_EQ(queue.Push(3), 0);
    ASSERT_EQ(queue.Push(4), 0);

    ASSERT_EQ(queue.Push(5), ErrorCode::kQueueFull);
    ASSERT_EQ(queue.GetSize(), uint64_t(4));
}

TEST(SPSCFixedQueueTest, TestQueueEmpty)
{
    SPSCFixedQueue<int> queue;
    queue.Init(4);

    int value;
    ASSERT_EQ(queue.Pop(value), ErrorCode::kQueueEmpty);
}

TEST(SPSCFixedQueueTest, TestStatistics)
{
    SPSCFixedQueue<int> queue;
    queue.Init(4);

    ProducerStatis prodStatis;
    ConsumerStatis consStatis;

    queue.GetStatis(prodStatis, consStatis);
    ASSERT_EQ(prodStatis.uNewEntryCount, uint64_t(0));
    ASSERT_EQ(consStatis.uGetEntryCount, uint64_t(0));

    queue.Push(1);
    queue.Push(2);

    queue.GetStatis(prodStatis, consStatis);
    ASSERT_EQ(prodStatis.uNewEntryCount, uint64_t(2));
    ASSERT_EQ(consStatis.uGetEntryCount, uint64_t(0));

    int value;
    queue.Pop(value);
    queue.Pop(value);

    queue.GetStatis(prodStatis, consStatis);
    ASSERT_EQ(prodStatis.uNewEntryCount, uint64_t(2));
    ASSERT_EQ(consStatis.uGetEntryCount, uint64_t(2));
}

TEST(SPSCFixedQueueTest, TestWrapAround)
{
    SPSCFixedQueue<int> queue;
    queue.Init(4);

    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));

    for (int i = 0; i < 4; ++i)
    {
        ASSERT_EQ(queue.Push(i), 0);
    }

    ASSERT_EQ(queue.Push(4), ErrorCode::kQueueFull);
    ASSERT_EQ(queue.GetSize(), uint64_t(4));

    int value;
    for (int i = 0; i < 4; ++i)
    {
        ASSERT_EQ(queue.Pop(value), 0);
        ASSERT_EQ(value, i);
    }

    ASSERT_EQ(queue.Pop(value), ErrorCode::kQueueEmpty);
    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));

    for (int i = 4; i < 8; ++i)
    {
        ASSERT_EQ(queue.Push(i), 0);
    }

    ASSERT_EQ(queue.Push(8), ErrorCode::kQueueFull);
    ASSERT_EQ(queue.GetSize(), uint64_t(4));

    for (int i = 4; i < 8; ++i)
    {
        ASSERT_EQ(queue.Pop(value), 0);
        ASSERT_EQ(value, i);
    }

    ASSERT_EQ(queue.Pop(value), ErrorCode::kQueueEmpty);
    ASSERT_TRUE(queue.IsEmpty());
    ASSERT_EQ(queue.GetSize(), uint64_t(0));

    ProducerStatis prodStatis;
    ConsumerStatis consStatis;

    queue.GetStatis(prodStatis, consStatis);
    ASSERT_EQ(prodStatis.uPostEntryCount, uint64_t(8));
    ASSERT_EQ(consStatis.uFreeEntryCount, uint64_t(8));
    ASSERT_EQ(prodStatis.uNewEntryFailCount, uint64_t(2));
    ASSERT_EQ(prodStatis.uNewEntryCount, uint64_t(8));
    ASSERT_EQ(consStatis.uGetEntryCount, uint64_t(8));
    ASSERT_EQ(consStatis.uGetEntryFailCount, uint64_t(2));
    
    queue.ClearStatis();
    queue.GetStatis(prodStatis, consStatis);
    ASSERT_EQ(prodStatis.uPostEntryCount, uint64_t(0));
    ASSERT_EQ(consStatis.uFreeEntryCount, uint64_t(0));
    ASSERT_EQ(prodStatis.uNewEntryFailCount, uint64_t(0));
    ASSERT_EQ(prodStatis.uNewEntryCount, uint64_t(0));
    ASSERT_EQ(consStatis.uGetEntryCount, uint64_t(0));
    ASSERT_EQ(consStatis.uGetEntryFailCount, uint64_t(0));
}

TEST(SPSCFixedQueueTest, TestPushPopWithThread)
{
    SPSCFixedQueue<int> queue;
    queue.Init(8192);

    volatile bool is_running = false;
    std::thread producer([&]() {
        while (!is_running)
        {
            usleep(0);
        }

        for (int i = 0; i < 1000000; ++i)
        {
            while (queue.Push(i) != 0)
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

        int value;
        for (int i = 0; i < 1000000; ++i)
        {
            while (queue.Pop(value) != 0)
            {
                usleep(0);
            }
            
            if (value != i)
            {
                ASSERT_EQ(value, i);
            }
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
    // ASSERT_EQ(prodStatis.uNewEntryFailCount, 0);
    ASSERT_EQ(consStatis.uGetEntryCount, uint64_t(1000000));
    ASSERT_EQ(consStatis.uFreeEntryCount, uint64_t(1000000));
    // ASSERT_EQ(consStatis.uGetEntryFailCount, 0);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}