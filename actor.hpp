#pragma once

#include <cstdio>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <functional>
#include <utility>
#include <vector>
#include <future>
#include <tuple>
#include <memory>
#include <any>
#include <type_traits>
#include <condition_variable>

namespace multith {

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

    std::future<std::any> pushWorkUnblockable(std::packaged_task<std::any()>&& func)
    {
        auto ret = func.get_future();

        std::thread([func = std::move(func), this]{
            std::lock_guard<std::mutex> workQueueGuard{workQueueMutex};

            // for some forsaken reason, std::move() won't turn this into
            // a && type, and will only do so with this satanic chant
            workQueue.push((std::packaged_task<std::any()>&&)func);

            waiter.notify_one();
        }).detach();

        return ret;
    }

    std::future<std::any> pushWorkBlockable(std::packaged_task<std::any()>&& func)
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
    {
        if constexpr(!std::is_same<RetT, void>::value)
        {return std::any_cast<RetT>(ret.get());}
        else 
        {ret.get();}
    }
};

template<typename T>
class Actor
{
public:
    template<typename ... ConstructArgs>
    Actor<T>(ConstructArgs ... args): self{args...}, thr{WorkerThread{}} {}
    Actor<T>(T&& nself): self{std::move(nself)}, thr{WorkerThread{}} {}

    ~Actor<T>() = default;

    using type = T;

    // the standard forbids partial template specialization,
    // so blame them for the if constexpr
    template<typename RetT, typename ... ArgT>
    ActorReturn<RetT> callUnblockable(RetT (T::*mthd) (ArgT...), ArgT ... args)
    {
        if constexpr(!std::is_same<RetT, void>::value)
        {
            std::packaged_task<std::any()> mthdPacked{[=, this]() {
                return std::any((self.*mthd)(args...));
            }};
            return ActorReturn<RetT>{thr.pushWorkUnblockable(std::move(mthdPacked))};
        }
        else
        {
            std::packaged_task<std::any()> mthdPacked{[=, this]() {
                (self.*mthd)(args...);
                return std::any();
            }};
            return ActorReturn<RetT>{thr.pushWorkUnblockable(std::move(mthdPacked))};
        }
    }

    template<typename RetT, typename ... ArgT>
    ActorReturn<RetT> callUnblockable(RetT (T::*mthd) (ArgT...) const, ArgT ... args) const
    {
        if constexpr(!std::is_same<RetT, void>::value)
        {
            std::packaged_task<std::any()> mthdPacked{[=, this]() {
                return std::any((self.*mthd)(args...));
            }};
            return ActorReturn<RetT>{thr.pushWorkUnblockable(std::move(mthdPacked))};
        }
        else
        {
            // don't know what universe you're in to want to have a const void
            // function, but this is here if you really need it
            std::packaged_task<std::any()> mthdPacked{[=, this]() {
                (self.*mthd)(args...);
                return std::any();
            }};
            return ActorReturn<RetT>{thr.pushWorkUnblockable(std::move(mthdPacked))};
        }
    }

    template<typename RetT, typename ... ArgT>
    ActorReturn<RetT> callBlockable(RetT (T::*mthd) (ArgT...), ArgT ... args)
    {
        if constexpr(!std::is_same<RetT, void>::value)
        {
            std::packaged_task<std::any()> mthdPacked{[=, this]() {
                return std::any((self.*mthd)(args...));
            }};
            return ActorReturn<RetT>{thr.pushWorkBlockable(std::move(mthdPacked))};
        }
        else
        {
            std::packaged_task<std::any()> mthdPacked{[=, this]() {
                (self.*mthd)(args...);
                return std::any();
            }};
            return ActorReturn<RetT>{thr.pushWorkBlockable(std::move(mthdPacked))};
        }
    }

    template<typename RetT, typename ... ArgT>
    ActorReturn<RetT> callBlockable(RetT (T::*mthd) (ArgT...) const, ArgT ... args) const
    {
        if constexpr(!std::is_same<RetT, void>::value)
        {
            std::packaged_task<std::any()> mthdPacked{[=, this]() {
                return std::any((self.*mthd)(args...));
            }};
            return ActorReturn<RetT>{thr.pushWorkBlockable(std::move(mthdPacked))};
        }
        else
        {
            // don't know what universe you're in to want to have a const void
            // function, but this is here if you really need it
            std::packaged_task<std::any()> mthdPacked{[=, this]() {
                (self.*mthd)(args...);
                return std::any();
            }};
            return ActorReturn<RetT>{thr.pushWorkBlockable(std::move(mthdPacked))};
        }
    }

private:
    T self;
    mutable WorkerThread thr;
};

} // namespace multith

