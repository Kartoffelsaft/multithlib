# What is multithlib?

At it's most basic, multithlib is a library that allows one to tie an object to a thread.
This allows for all of the methods and state of an object to be handled asynchronously

## How do I use it?

Multithlib provides the `Actor<T>` type. 
If the constructor is passed the arguments of `T`'s constructor, it will construct `T` in place.
It can alternatively be passed `T&&` to use an existing instance of `T`.
The `Actor<T>` has a `call(...)` method that will allow dispatching method calls to the separate thread.

Example:

```c++
Actor<MyFunType> myActor{};

auto myFutureReturn = myActor.call(&MyFunType::myExpensiveMethod, myArg1, myArg2);

// do some other things

auto myPresentReturn = myFutureReturn.get();
```

## limitations and cautions

  * The `call` method requires that the type of actor be respecified. This may cause issues especially with compiler-generated types such as lambdas, and is clearly verbose as a human reader.
  * Safety of the user-provided type is not guaranteed, and as such it is highly recommended that you make sure that any pass-by-reference arguments are atomic or otherwise thread safe.
  * Returns of `call` go though an `ActorReturn<T>` struct, which is mostly just a wrapper around `std::future<std::any>`. This is unfortunate because the type of the return is quite obvious at compile time, but part of the internals demands that all return types be the same.
