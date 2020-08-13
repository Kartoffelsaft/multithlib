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

TEST(ActorTest, MutableState)
{
    struct S {
        int x;
        S(): x{0} {}
        void addSome(int const some) {x += some;}
        int getX() const {return x;}
    };
    Actor<S> act{};

    auto a = act.call(&S::getX);
    act.call(&S::addSome, 3);
    auto b = act.call(&S::getX);
    act.call(&S::addSome, 4);
    auto c = act.call(&S::getX);

    ASSERT_EQ(a.get(), 0);
    ASSERT_EQ(b.get(), 3);
    ASSERT_EQ(c.get(), 7);
}

int main(int argc, char** argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
