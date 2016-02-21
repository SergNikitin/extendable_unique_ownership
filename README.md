# Extendable unique resource ownership

## Introduction

This is a tiny module that provides 3 smart-pointers that are indended to be
used together:

* `unique_extendable_ptr`
* `weak_extender`
* `scoped_extender`

It is the result of thinking about lifetime management in video games
where it is often desired to remove a particular game object from a game scene
as quickly as possible. Removal from the scene during the object's destruction
seems a better solution than functions like removeFromScene() which leave the object
alive since it does not produce any new object states (alive and on scene, alive
but not on scene) which can complicate the rest of the logic. No logic branches exist
if the object is destroyed. Depending on the ways of achieving this way of object
removal, several problems may arise in a multi-threaded environment like unsafe
usage of resources between multiple threads or accidental undesired lifetime
extension of a resource.

## Goals

The main goal of the module is to give an option to explicitly make the ownership
of a resource unique (like std::unique_ptr) while providing `std::shared_ptr`-like
thread-safety guarantees.

The second goal is to make an incorrect usage difficult or impossible by design
(like std::unique_ptr and `std::shared_ptr` in the areas of their expertise). I am
a big fun of concepts which let you achieve this since it allows you to think more
about things that matter (like architecture and performance) and think less about
common implementation mistakes you can make (like forgetting to lock a mutex).

The same safety and performance that this module aims to provide may be easily
achieved just by using `std::shared_ptr`/`std::weak_ptr` but there will be no
logical clarity of a unique resource ownership which I value very much (of course,
unique ownership is not always the case - but I would like to explicitely state so
when on a logical, conceptual level it is, even if it can not be achived through
classical means due to implementation troubles like thread-safety).

## Explanation

Consider the following use-case: there is a storage of some kind
which stores resources as std::unique_ptr-s. Let's say there are outside
users that can request a raw pointer to a resource from the storage to work
with it and the storage may destroy the resource upon request from the
outside.
In a multi-threaded environment this may lead to situations when thread A
recieves a raw pointer to the resource and thread B destroys the
std::unique_ptr along with the resource that is being used by thread A,
which will lead to a crash.

One of the obvious solutions to this is to store resources in `std::shared_ptr`-s
and to provide the outside users with `std::shared_ptr`-s/`std::weak_ptr`-s.
This provides a thread-safe way of knowing whether the resource is alive or
not. However in this scenario the storage is no longer the unique owner of
the resources and an outside user can easily extend the lifetime of the
resource for an indefinite period of time just by storing a `std::shared_ptr`
somewhere (or, if a `std::weak_ptr` is provided by storage, by locking it and
storing the result) in the system. Depending on the situation this may or may not
be desired.

An example of a system in which this would be undesired is a scene of a game
in which a resource is an object and the destruction of a resource results
in object disappearing from the view of the player. In this situation it is
desired to remove the object from scene as quickly as possible when the game
logic says so. By using `std::shared_ptr`/`std::weak_ptr`, a careless module may
prevent this from happening.
`unique_extendable_ptr` provides means to avoid these problems, albeit in a
hacky way. It is essentially a wrapper around `std::shared_ptr`. A storage may
give an outside user an option to get a `weak_extender` that is a wrapper
around `std::weak_ptr`, which can be copied and stored without any impact on
the resource lifetime. When the time comes to perform an actual work with
the resource, an outside user can lock `weak_extender` which will produce
a `scoped_extender`. `scoped_extender` is similar to `std::shared_ptr`, but it
is unmovable and uncopiable - similar to `boost::scoped_ptr`, but not creatable from
outside of the `weak_extender`. Since it is uncopiable and unmovable, it can not be
stored by value (neither in the scope of a function nor inside a different object).
The only way to access it is to catch the value returned by `weak_extender::lock()`
by const reference (the 'const' is important, since the C++ standard states that
the lifetime of a temporary object returned from a function may be extended for the
current scope only if it was caught by const reference). Storing this
reference in a different object, however, does not extend the lifetime of
a temporary. This means that `scoped_extender` will be alive only until
the function that called `weak_extender::lock()` finishes.

All of this means that in most cases the lifetime of a resource may be
extended beyond the lifetime of the `unique_extendable_ptr` only for a very
short period of time. An infinite loop will break this guarantee, but in
general an infinite loop is hard to miss, relatively easy to diagnose and
almost never desired.

## Problems
### Holy Jesus that's a hack
In theory this module should allow to write clearer and simpler code by fully
taking care of a problem it aims to tackle, but it relies heavily on a very
unconvential thing - cathing a temporary by a const reference. Although the
behaviour menthioned above is fully C++ standard-complient, it is one of those dark
corners of C++ that is not well-known and which we generally avoid. Due to this
fact I would expect most people to cater to a familiar
`std::shared_ptr`/`std::weak_ptr` solution.

### Return Value Optimization
RVO is present in all major C++ compilers though in my expirience it
may be used by one compiler and not used by another while compiling the same code
or the compilers may treat the result differently.
If a compiler decides to use RVO it may compile a call like

```cpp
auto unique = make_unique_extendable<int>(0);
auto weak = weak_extender(unique);
auto scoped = weak.lock(); // notice there is no const auto& on the left
```

As far as my understanding of compilers goes, the compiler may have checked that
`scoper_extender` creation is legal inside `weak_extender::lock()`, allowed it and did not generate any constructor code outside of it (which would be illegal) by just optimizing temporaries away.

That would be against the standard and will not be portable, but it may compile.
