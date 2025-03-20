#include "gtest/gtest.h"
#include "../spsc_fixed_unbonded_queue.h"

using namespace LockFreeQueue;

TEST(SPSCUnboundedQueueTest, PushPopTest)
{
    SPSCUnboundedQueue<int> queue;
    ASSERT_EQ(queue.Init(1024), 0);
    int value = 42;
    ASSERT_EQ(queue.Push(value), 0);

    int result;
    ASSERT_EQ(queue.Pop(result), 0);
    ASSERT_EQ(result, value);
}

TEST(SPSCUnboundedQueueTest, QueueEmptyTest)
{
    SPSCUnboundedQueue<int> queue;
    ASSERT_EQ(queue.Init(1024), 0);

    ASSERT_TRUE(queue.IsEmpty());

    int value = 42;
    queue.Push(value);
    ASSERT_FALSE(queue.IsEmpty());

    int result;
    queue.Pop(result);
    ASSERT_TRUE(queue.IsEmpty());

    ASSERT_EQ(queue.Pop(result), ErrorCode::kQueueEmpty);
}

TEST(SPSCUnboundedQueueTest, QueueSizeTest)
{
    SPSCUnboundedQueue<int> queue;
    ASSERT_EQ(queue.Init(1024), 0);

    ASSERT_EQ(queue.GetSize(), (uint64_t)0);

    int value = 42;
    queue.Push(value);
    ASSERT_EQ(queue.GetSize(), (uint64_t)1);

    int result;
    queue.Pop(result);
    ASSERT_EQ(queue.GetSize(), (uint64_t)0);
}

TEST(SPSCUnboundedQueueTest, MultiplePushPopTest)
{
    SPSCUnboundedQueue<int> queue;
    ASSERT_EQ(queue.Init(1024), 0);

    for (int i = 0; i < 2048; ++i)
    {
        ASSERT_EQ(queue.Push(i), 0);
    }

    for (int i = 0; i < 2048; ++i)
    {
        int result;
        ASSERT_EQ(queue.Pop(result), 0);
        ASSERT_EQ(result, i);
    }
}

TEST(SPSCUnboundedQueueTest, PushPopDifferentTypes)
{
    SPSCUnboundedQueue<int> queue;
    ASSERT_EQ(queue.Init(1024), 0);

    SPSCUnboundedQueue<std::string> stringQueue;
    stringQueue.Init(1024);

    std::string value = "test";
    ASSERT_EQ(stringQueue.Push(value), 0);

    std::string result;
    ASSERT_EQ(stringQueue.Pop(result), 0);
    ASSERT_EQ(result, value);
}

TEST(SPSCUnboundedQueueTest, GetStatisTest)
{
    SPSCUnboundedQueue<int> queue;
    ASSERT_EQ(queue.Init(1024), 0);

    ProducerStatis producerStatis;
    ConsumerStatis consumerStatis;
    queue.GetStatis(producerStatis, consumerStatis);

    ASSERT_EQ(producerStatis.uNewEntryCount, uint64_t(0));
    ASSERT_EQ(producerStatis.uNewEntryFailCount, uint64_t(0));
    ASSERT_EQ(producerStatis.uPostEntryCount, uint64_t(0));

    ASSERT_EQ(consumerStatis.uGetEntryCount, uint64_t(0));
    ASSERT_EQ(consumerStatis.uGetEntryFailCount, uint64_t(0));
    ASSERT_EQ(consumerStatis.uFreeEntryCount, uint64_t(0));

    int value = 42;
    queue.Push(value);
    queue.GetStatis(producerStatis, consumerStatis);

    ASSERT_EQ(producerStatis.uNewEntryCount, uint64_t(1));
    ASSERT_EQ(producerStatis.uNewEntryFailCount, uint64_t(0));
    ASSERT_EQ(producerStatis.uPostEntryCount, uint64_t(1));

    ASSERT_EQ(consumerStatis.uGetEntryCount, uint64_t(0));
    ASSERT_EQ(consumerStatis.uGetEntryFailCount, uint64_t(0));
    ASSERT_EQ(consumerStatis.uFreeEntryCount, uint64_t(0));

    int result;
    queue.Pop(result);
    queue.GetStatis(producerStatis, consumerStatis);

    ASSERT_EQ(producerStatis.uNewEntryCount, uint64_t(1));
    ASSERT_EQ(producerStatis.uNewEntryFailCount, uint64_t(0));
    ASSERT_EQ(producerStatis.uPostEntryCount, uint64_t(1));

    ASSERT_EQ(consumerStatis.uGetEntryCount, uint64_t(1));
    ASSERT_EQ(consumerStatis.uGetEntryFailCount, uint64_t(0));
    ASSERT_EQ(consumerStatis.uFreeEntryCount, uint64_t(1));
}

TEST(SPSCUnboundedQueueTest, PushPopWithThread)
{
    SPSCUnboundedQueue<int> queue;
    ASSERT_EQ(queue.Init(1024), 0);

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

    ProducerStatis producerStatis;
    ConsumerStatis consumerStatis;
    queue.GetStatis(producerStatis, consumerStatis);

    ASSERT_EQ(producerStatis.uNewEntryCount, uint64_t(1000000));
    ASSERT_EQ(consumerStatis.uGetEntryCount, uint64_t(1000000));
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}