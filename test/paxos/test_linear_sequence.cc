#include <gtest/gtest.h>
#include <thread>

#include "paxos/linear_sequence.h"

using namespace ledgerd::paxos;

namespace paxos_test {

TEST(LinearSequence, LinearAdds) {
    LinearSequence<uint64_t> sequence(0);
    sequence.Add(1);
    sequence.Add(2);
    EXPECT_EQ(0, sequence.lower_bound());
    EXPECT_EQ(2, sequence.upper_bound());
    EXPECT_EQ(0, sequence.n_disjoint());
}

TEST(LinearSequence, DisjointAdds) {
    LinearSequence<uint64_t> sequence(0);
    sequence.Add(1);
    sequence.Add(2);
    sequence.Add(4);

    EXPECT_EQ(0, sequence.lower_bound());
    EXPECT_EQ(2, sequence.upper_bound());
    EXPECT_EQ(1, sequence.n_disjoint());

    sequence.Add(3);

    EXPECT_EQ(0, sequence.lower_bound());
    EXPECT_EQ(4, sequence.upper_bound());
    EXPECT_EQ(0, sequence.n_disjoint());
}

TEST(LinearSequence, DisjointOutOfOrderAdds) {
    LinearSequence<uint64_t> sequence(0);
    sequence.Add(6);
    sequence.Add(4);
    sequence.Add(3);
    sequence.Add(1);
    sequence.Add(2);
    
    EXPECT_EQ(0, sequence.lower_bound());
    EXPECT_EQ(4, sequence.upper_bound());
    EXPECT_EQ(1, sequence.n_disjoint());

    sequence.Add(5);
    EXPECT_EQ(0, sequence.lower_bound());
    EXPECT_EQ(6, sequence.upper_bound());
    EXPECT_EQ(0, sequence.n_disjoint());
}

TEST(LinearSequence, WaitForAdded) {
    LinearSequence<uint64_t> sequence(0);
    std::thread t1([&sequence] {
            sequence.Add(9);
        });
    sequence.WaitForAdded(9);
    EXPECT_EQ(0, sequence.upper_bound());
    t1.join();
}

TEST(LinearSequence, WaitForUpperBound) {
    LinearSequence<uint64_t> sequence(0);
    std::thread t1([&sequence] {
            for(int i = 1; i < 10; ++i) {
                sequence.Add(i);
            }
        });
    sequence.WaitForUpperBound(9);
    EXPECT_EQ(9, sequence.upper_bound());
    t1.join();
}

TEST(LinearSequence, WaitForUpperBoundAlreadyPassed) {
    LinearSequence<uint64_t> sequence(0);
    sequence.Add(1);
    sequence.Add(2);
    sequence.Add(3);
    sequence.WaitForUpperBound(2);
    EXPECT_EQ(3, sequence.upper_bound());
}

}
