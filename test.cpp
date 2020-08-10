#include "actor.hpp"
#include <gtest/gtest.h>

TEST(ActorTest, TwoPlusTwo) 
{
    struct F 
    {int twoPlusTwo() {return 2 + 2;}};
    Actor<F> a{};

    ASSERT_EQ(a.call(&F::twoPlusTwo).get(), 4);
}

TEST(ActorTest, Lambda)
{
    auto l = Actor{[](){return 2 + 2;}};

    ASSERT_EQ(l.call(&decltype(l)::type::operator()).get(), 4);
}

int main(int argc, char** argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
