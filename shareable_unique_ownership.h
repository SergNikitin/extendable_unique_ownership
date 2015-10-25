#ifndef _SHAREABLE_UNIQUE_OWNERSHIP_
#define _SHAREABLE_UNIQUE_OWNERSHIP_

#include <atomic>
#include <memory>

template <typename T> class WeakRef;
template <typename T> class ScopedRef;

template <typename T>
class OwningRef {
    friend class WeakRef<T>;
public:
    OwningRef() = default;
    OwningRef(std::unique_ptr<T>&&);
    OwningRef(T*);

    ~OwningRef();

    OwningRef(const OwningRef&) = delete;
    OwningRef& operator=(const OwningRef&) = delete;
    OwningRef(OwningRef&&) = default;
    OwningRef& operator=(OwningRef&&) = default;

    T* get() const;
    T* operator->() const;

    void reset();

private:
    struct ResourceOwner {
    public:
        ResourceOwner();
        explicit ResourceOwner(std::unique_ptr<T>&&);

        ResourceOwner(const ResourceOwner&) = delete;
        ResourceOwner& operator=(const ResourceOwner&) = delete;
        ResourceOwner(ResourceOwner&&) = delete;
        ResourceOwner& operator=(ResourceOwner&&) = delete;

        T* get() const;
        T* operator->() const;

        std::unique_ptr<T> resource;
        std::atomic_bool markedForDestruction;
    };

    std::shared_ptr<ResourceOwner> resource;
};

template <typename T, typename... CtorArgTypes>
OwningRef<T> makeOwning(CtorArgTypes&&... ctorArgs);

template <typename T>
class WeakRef
{
    friend class ScopedRef<T>;
public:
    WeakRef() = default;
    WeakRef(const OwningRef<T>&);

    WeakRef(const WeakRef&) = default;
    WeakRef& operator=(const WeakRef&) = default;
    WeakRef(WeakRef&&) = default;
    WeakRef& operator=(WeakRef&&) = default;

    ScopedRef<T> lock() const;
    void reset();

private:
    using Resource = typename OwningRef<T>::ResourceOwner;

    static bool notMarkedForDestruction(const std::shared_ptr<Resource>&);

    std::weak_ptr<Resource> resource;
};

template <typename T>
class ScopedRef {
    friend class WeakRef<T>;
public:
    ScopedRef(const ScopedRef&) = delete;
    ScopedRef& operator=(const ScopedRef&) = delete;
    ScopedRef& operator=(ScopedRef&&) = delete;

    T* operator->() const;

    bool empty() const;
    void reset();

private:
    using Resource = typename WeakRef<T>::Resource;

    ScopedRef() = default;
    ScopedRef(const std::shared_ptr<Resource>&);

    ScopedRef(ScopedRef&&) = default;

    std::shared_ptr<Resource> resource;
};

#include "shareable_unique_ownership_impl.h"

#endif // _SHAREABLE_UNIQUE_OWNERSHIP_
