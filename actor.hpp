#pragma once

#include <cstdio>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <functional>
#include <vector>
#include <future>
#include <tuple>
#include <memory>
#include <any>
#include <type_traits>
#include <condition_variable>

class WorkerThread
{
public:
    
    WorkerThread(): 
        workQueue{},
        workQueueMutex{},
        waiter{},
        threadLooping{new std::atomic_bool(true)}
    {
        thr = std::thread(&WorkerThread::workerThreadLoop, this);
    }

    ~WorkerThread()
    {
        threadLooping.clear();
        waiter.notify_one();
        if(thr.joinable()) thr.join();
    }

    std::future<std::any> pushWork(std::packaged_task<std::any()>&& func)
    {
        auto ret = func.get_future();
        {
            std::lock_guard<std::mutex> workQueueGuard{workQueueMutex};

            workQueue.push(std::move(func));
        }
        waiter.notify_one();

        return ret;
    }

private:
    
    std::queue<std::packaged_task<std::any()>> workQueue;
    std::mutex workQueueMutex;      // atomizes workQueue
    std::condition_variable waiter; // waits for incoming function calls (reduces CPU usage)

    std::thread thr;
    std::atomic_flag threadLooping{true};

    void workerThreadLoop()
    {
        while(threadLooping.test_and_set())
        {
            std::unique_lock<std::mutex> workQueueGuard(workQueueMutex);

            for(
                auto work = &workQueue.front(); 
                !workQueue.empty(); 
                workQueue.pop(), 
                work = &workQueue.front()
            ){
                (*work)();
            }
            waiter.wait(workQueueGuard);
        }
    }
};

template<typename RetT>
struct ActorReturn
{
    ActorReturn<RetT>(std::future<std::any>&& nRet): ret{std::move(nRet)} {};
    ActorReturn<RetT>() {};

    std::future<std::any> ret;

    RetT get()
    {return std::any_cast<RetT>(ret.get());}
};

template<typename T>
class Actor
{
public:
    Actor<T>(): self{T{}}, thr{WorkerThread{}} {} 
    Actor<T>(T&& nself): self{std::move(nself)}, thr{WorkerThread{}} {}

    ~Actor<T>() = default;

    using type = T;

    // the standard forbids partial template specialization,
    // so blame them for the if constexpr
    template<typename RetT, typename ... ArgT>
    ActorReturn<RetT> call(RetT (T::*mthd) (ArgT...), ArgT ... args)
    {
        if constexpr(!std::is_same<RetT, void>::value)
        {
            std::packaged_task<std::any()> mthdPacked{[=]() {
                return std::any((self.*mthd)(args...));
            }};
            return ActorReturn<RetT>{thr.pushWork(std::move(mthdPacked))};
        }
        else
        {
            std::packaged_task<std::any()> mthdPacked{[=]() {
                (self.*mthd)(args...);
                return std::any();
            }};
            return ActorReturn<RetT>{thr.pushWork(std::move(mthdPacked))};
        }
    }

    template<typename RetT, typename ... ArgT>
    ActorReturn<RetT> call(RetT (T::*mthd) (ArgT...) const, ArgT ... args) const
    {
        if constexpr(!std::is_same<RetT, void>::value)
        {
            std::packaged_task<std::any()> mthdPacked{[=]() {
                return std::any((self.*mthd)(args...));
            }};
            return ActorReturn<RetT>{thr.pushWork(std::move(mthdPacked))};
        }
        else
        {
            std::packaged_task<std::any()> mthdPacked{[=]() {
                (self.*mthd)(args...);
                return std::any();
            }};
            return ActorReturn<RetT>{thr.pushWork(std::move(mthdPacked))};
        }
    }

private:
    T self;
    mutable WorkerThread thr;
};

