#include "actor.hpp"
#include <gtest/gtest.h>

#include <chrono>
#include <mutex>
#include <ratio>
#include <thread>

using namespace multith;

TEST(ActorTest, TwoPlusTwo) 
{
    struct F 
    {int twoPlusTwo() {return 2 + 2;}};
    Actor<F> a{};

    ASSERT_EQ(a.callBlockable(&F::twoPlusTwo).get(), 4);
}

TEST(ActorTest, Lambda)
{
    auto l = Actor{[](){return 2 + 2;}};

    ASSERT_EQ(l.callBlockable(&decltype(l)::type::operator()).get(), 4);
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

    auto a = act.callBlockable(&S::getX);
    act.callBlockable(&S::addSome, 3);
    auto b = act.callBlockable(&S::getX);
    act.callBlockable(&S::addSome, 4);
    auto c = act.callBlockable(&S::getX);

    ASSERT_EQ(a.get(), 0);
    ASSERT_EQ(b.get(), 3);
    ASSERT_EQ(c.get(), 7);
}

TEST(ActorTest, InterActor)
{
    struct T {
        int x;
        T(): x{0} {}
        void addSome(int const some) {x += some;}
        int getX() {return x;}
    };
    struct S {
        Actor<T>* other;
        S(Actor<T>* nother): other{nother} {}
        void addSomeToOther(int const some) {other->callUnblockable(&T::addSome, some);}
    };
    
    Actor<T> t{};
    Actor<S> s{&t};

    s.callBlockable(&S::addSomeToOther, 3).get();
    
    ASSERT_EQ(t.callBlockable(&T::getX).get(), 3);
}

TEST(ActorTest, VoidDoesntBlock)
{
    auto const threshold = std::chrono::milliseconds(100);
    struct S {
        // it refuses to work without this line
        std::chrono::milliseconds const & threshold = threshold;
        void doSomeExpensiveStuff() {
            std::this_thread::sleep_for(threshold);
        }
    };

    Actor<S> offload;

    auto begin = std::chrono::system_clock::now();

    {
        offload.callUnblockable(&S::doSomeExpensiveStuff);
        offload.callBlockable(&S::doSomeExpensiveStuff);
    }

    auto end = std::chrono::system_clock::now();

    ASSERT_LT(end - begin, threshold);
}

// very similar to the last one, with a tiny change
TEST(ActorTest, UnusedDoesntBlock)
{
    auto const threshold = std::chrono::milliseconds(100);
    struct S {
        std::chrono::milliseconds const & threshold = threshold;
        int doSomeExpensiveStuff() {
            std::this_thread::sleep_for(threshold);

            return 3;
        }
    };

    Actor<S> offload;

    auto begin = std::chrono::system_clock::now();

    {
        offload.callUnblockable(&S::doSomeExpensiveStuff);
        offload.callBlockable(&S::doSomeExpensiveStuff);
    }

    auto end = std::chrono::system_clock::now();

    ASSERT_LT(end - begin, threshold);
}

int main(int argc, char** argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
