#include "../spsc_fixed_queue.h"
#include <gtest/gtest.h>

using namespace LockFreeQueue;


TEST(SPSCFixedQueueTest, TestInit)
{
    SPSCFixedQueue<int> queue;
    EXPECT_EQ(queue.Init(0), ErrorCode::kQueueError);
    EXPECT_EQ(queue.Init(8), 0);
    EXPECT_TRUE(queue.IsEmpty());
}

TEST(SPSCFixedQueueTest, TestNewGetEntry)
{
    SPSCFixedQueue<int> queue;
    queue.Init(4);

    EXPECT_TRUE(queue.IsEmpty());

    int* entry = queue.NewEntry();
    EXPECT_NE(entry, static_cast<int*>(nullptr));
    *entry = 1;
    queue.PostEntry(entry);

    entry = queue.NewEntry();
    EXPECT_NE(entry, static_cast<int*>(nullptr));
    *entry = 2;
    queue.PostEntry(entry);

    EXPECT_FALSE(queue.IsEmpty());

    entry = queue.GetEntry();
    EXPECT_NE(entry, static_cast<int*>(nullptr));
    EXPECT_EQ(*entry, 1);
    queue.FreeEntry(entry);

    entry = queue.GetEntry();
    EXPECT_NE(entry, static_cast<int*>(nullptr));
    EXPECT_EQ(*entry, 2);
    queue.FreeEntry(entry);

    EXPECT_TRUE(queue.IsEmpty());
}

TEST(SPSCFixedQueueTest, TestPushPop)
{
    SPSCFixedQueue<int> queue;
    queue.Init(8);

    EXPECT_TRUE(queue.IsEmpty());

    EXPECT_EQ(queue.Push(1), 0);
    EXPECT_EQ(queue.Push(2), 0);
    EXPECT_EQ(queue.Push(3), 0);

    EXPECT_FALSE(queue.IsEmpty());

    int value;
    EXPECT_EQ(queue.Pop(value), 0);
    EXPECT_EQ(value, 1);

    EXPECT_EQ(queue.Pop(value), 0);
    EXPECT_EQ(value, 2);

    EXPECT_EQ(queue.Pop(value), 0);
    EXPECT_EQ(value, 3);

    EXPECT_TRUE(queue.IsEmpty());
}

TEST(SPSCFixedQueueTest, TestQueueFull)
{
    SPSCFixedQueue<int> queue;
    queue.Init(4);

    EXPECT_EQ(queue.Push(1), 0);
    EXPECT_EQ(queue.Push(2), 0);
    EXPECT_EQ(queue.Push(3), 0);
    EXPECT_EQ(queue.Push(4), 0);

    EXPECT_EQ(queue.Push(5), ErrorCode::kQueueFull);
}

TEST(SPSCFixedQueueTest, TestQueueEmpty)
{
    SPSCFixedQueue<int> queue;
    queue.Init(4);

    int value;
    EXPECT_EQ(queue.Pop(value), ErrorCode::kQueueEmpty);
}

TEST(SPSCFixedQueueTest, TestStatistics)
{
    SPSCFixedQueue<int> queue;
    queue.Init(4);

    ProducerStatis prodStatis;
    ConsumerStatis consStatis;

    queue.GetStatis(prodStatis, consStatis);
    EXPECT_EQ(prodStatis.uNewEntryCount, uint64_t(0));
    EXPECT_EQ(consStatis.uGetEntryCount, uint64_t(0));

    queue.Push(1);
    queue.Push(2);

    queue.GetStatis(prodStatis, consStatis);
    EXPECT_EQ(prodStatis.uNewEntryCount, uint64_t(2));
    EXPECT_EQ(consStatis.uGetEntryCount, uint64_t(0));

    int value;
    queue.Pop(value);
    queue.Pop(value);

    queue.GetStatis(prodStatis, consStatis);
    EXPECT_EQ(prodStatis.uNewEntryCount, uint64_t(2));
    EXPECT_EQ(consStatis.uGetEntryCount, uint64_t(2));
}

TEST(SPSCFixedQueueTest, TestWrapAround)
{
    SPSCFixedQueue<int> queue;
    queue.Init(4);

    EXPECT_TRUE(queue.IsEmpty());

    for (int i = 0; i < 4; ++i)
    {
        EXPECT_EQ(queue.Push(i), 0);
    }

    EXPECT_EQ(queue.Push(4), ErrorCode::kQueueFull);

    int value;
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_EQ(queue.Pop(value), 0);
        EXPECT_EQ(value, i);
    }

    EXPECT_EQ(queue.Pop(value), ErrorCode::kQueueEmpty);
    EXPECT_TRUE(queue.IsEmpty());

    for (int i = 4; i < 8; ++i)
    {
        EXPECT_EQ(queue.Push(i), 0);
    }

    EXPECT_EQ(queue.Push(8), ErrorCode::kQueueFull);

    for (int i = 4; i < 8; ++i)
    {
        EXPECT_EQ(queue.Pop(value), 0);
        EXPECT_EQ(value, i);
    }

    EXPECT_EQ(queue.Pop(value), ErrorCode::kQueueEmpty);
    EXPECT_TRUE(queue.IsEmpty());

    ProducerStatis prodStatis;
    ConsumerStatis consStatis;

    queue.GetStatis(prodStatis, consStatis);
    EXPECT_EQ(prodStatis.uPostEntryCount, uint64_t(8));
    EXPECT_EQ(consStatis.uFreeEntryCount, uint64_t(8));
    EXPECT_EQ(prodStatis.uNewEntryFailCount, uint64_t(2));
    EXPECT_EQ(prodStatis.uNewEntryCount, uint64_t(8));
    EXPECT_EQ(consStatis.uGetEntryCount, uint64_t(8));
    EXPECT_EQ(consStatis.uGetEntryFailCount, uint64_t(2));
    
    queue.ClearStatis();
    queue.GetStatis(prodStatis, consStatis);
    EXPECT_EQ(prodStatis.uPostEntryCount, uint64_t(0));
    EXPECT_EQ(consStatis.uFreeEntryCount, uint64_t(0));
    EXPECT_EQ(prodStatis.uNewEntryFailCount, uint64_t(0));
    EXPECT_EQ(prodStatis.uNewEntryCount, uint64_t(0));
    EXPECT_EQ(consStatis.uGetEntryCount, uint64_t(0));
    EXPECT_EQ(consStatis.uGetEntryFailCount, uint64_t(0));
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
                EXPECT_EQ(value, i);
            }
        }
    });

    is_running = true;
    producer.join();
    consumer.join();

    EXPECT_TRUE(queue.IsEmpty());

    ProducerStatis prodStatis;
    ConsumerStatis consStatis;

    queue.GetStatis(prodStatis, consStatis);
    EXPECT_EQ(prodStatis.uNewEntryCount, uint64_t(1000000));
    EXPECT_EQ(prodStatis.uPostEntryCount, uint64_t(1000000));
    // EXPECT_EQ(prodStatis.uNewEntryFailCount, 0);
    EXPECT_EQ(consStatis.uGetEntryCount, uint64_t(1000000));
    EXPECT_EQ(consStatis.uFreeEntryCount, uint64_t(1000000));
    // EXPECT_EQ(consStatis.uGetEntryFailCount, 0);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}