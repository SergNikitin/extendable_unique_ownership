#ifndef _EXTENDABLE_UNIQUE_OWNERSHIP_
#define _EXTENDABLE_UNIQUE_OWNERSHIP_

#include <atomic>
#include <memory>

template <typename T> class weak_extender;
template <typename T> class scoped_extender;

template <typename T>
class unique_extendable_ptr {
public:
    unique_extendable_ptr() = default;
    explicit unique_extendable_ptr(std::unique_ptr<T>);
    explicit unique_extendable_ptr(T*);

    ~unique_extendable_ptr();

    unique_extendable_ptr(const unique_extendable_ptr&) = delete;
    unique_extendable_ptr& operator=(const unique_extendable_ptr&) = delete;
    unique_extendable_ptr(unique_extendable_ptr&&) = default;
    unique_extendable_ptr& operator=(unique_extendable_ptr&&) = default;

    T* get() const;
    T* operator->() const;

    void reset();

private:
	friend class weak_extender<T>;
	friend class scoped_extender<T>;

    struct resource_owner;

    using strong_lifetime_link = std::shared_ptr<resource_owner>;
    using weak_lifetime_link = std::weak_ptr<resource_owner>;

    strong_lifetime_link resource;
};

template <typename T>
struct unique_extendable_ptr<T>::resource_owner {
public:
    resource_owner();
    explicit resource_owner(std::unique_ptr<T>);

    resource_owner(const resource_owner&) = delete;
    resource_owner& operator=(const resource_owner&) = delete;
    resource_owner(resource_owner&&) = delete;
    resource_owner& operator=(resource_owner&&) = delete;

    T* get() const;
    T* operator->() const;

    std::unique_ptr<T> resource;
    std::atomic_bool marked_for_destruction;
};

template <typename T, typename... CtorArgTypes>
unique_extendable_ptr<T> make_unique_extendable(CtorArgTypes&&... ctorArgs);

template <typename T>
class weak_extender {
public:
    weak_extender() = default;
    explicit weak_extender(const unique_extendable_ptr<T>&);

    weak_extender(const weak_extender&) = default;
    weak_extender& operator=(const weak_extender&) = default;
    weak_extender(weak_extender&&) = default;
    weak_extender& operator=(weak_extender&&) = default;

    scoped_extender<T> lock() const;
    void reset();

private:
    friend class scoped_extender<T>;

    using resource = typename unique_extendable_ptr<T>::resource_owner;
    using weak_lifetime_link = typename unique_extendable_ptr<T>::weak_lifetime_link;

    static bool not_marked_for_destruction(const resource*);

    weak_lifetime_link link;
};

template <typename T>
class scoped_extender {
public:
    scoped_extender(const scoped_extender&) = delete;
    scoped_extender& operator=(const scoped_extender&) = delete;
    scoped_extender& operator=(scoped_extender&&) = delete;

    T* get() const;
    T* operator->() const;

    bool empty() const;
    void reset();

private:
    friend class weak_extender<T>;

    using strong_lifetime_link = typename unique_extendable_ptr<T>::strong_lifetime_link;

    scoped_extender() = default;
    explicit scoped_extender(strong_lifetime_link);

    scoped_extender(scoped_extender&&) = default;

    strong_lifetime_link link;
};

#include "shareable_unique_ownership_impl.h"

#endif // _EXTENDABLE_UNIQUE_OWNERSHIP_
