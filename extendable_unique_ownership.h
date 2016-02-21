#ifndef _EXTENDABLE_UNIQUE_OWNERSHIP_
#define _EXTENDABLE_UNIQUE_OWNERSHIP_

#include <atomic>
#include <memory>

template <typename T> class weak_extender;
template <typename T> class scoped_extender;

/**
 * @brief Smart pointer which in terms of lifetime management concepts is
 * uniquely responsible for a lifetime of a certain resource (meaning, when this
 * object is destroyed, the resource should be considered destroyed as well) but
 * provides means to extend the lifetime of that resource for a short period of
 * time
 * @details Short lifetime extension is useful to make the resource existence
 * thread-safe (in case the outside user started to work with the resource in a
 * different thread before the destruction of this object took place) while
 * retaining the logical concept of unique ownership.
 *
 * The only ways to access the underlying resource is either by direct access
 * to the unique_extendable_ptr or by @see weak_extender + @see scoped_extender
 * usage.
 *
 * In most cases the lifetime of a resource may be extended by @see scoped_extender
 * beyond the lifetime of the unique_extendable_ptr only for a very short period
 * of time. An infinite loop may break this guarantee, but in general an infinite
 * loop is hard to miss, relatively easy to diagnose and almost never desired.
 *
 * @tparam T Type of the owned resource
 */
template <typename T>
class unique_extendable_ptr {
public:
    /**
     * @brief Counstructs and empty smart pointer that does not own anything
     */
    unique_extendable_ptr() = default;
    /**
     * @brief Constructor that consumes ownership of a heap-allocated resource
     * @details Is intentionnaly not marked "explicit" to work with raw owning
     * pointers as well as std::unique_ptr
     */
    unique_extendable_ptr(std::unique_ptr<T>);

    /**
     * @brief @see reset()
     */
    ~unique_extendable_ptr();

    unique_extendable_ptr(const unique_extendable_ptr&) = delete;
    unique_extendable_ptr& operator=(const unique_extendable_ptr&) = delete;
    unique_extendable_ptr(unique_extendable_ptr&&) = default;
    unique_extendable_ptr& operator=(unique_extendable_ptr&&) = default;

    T* get() const;
    T* operator->() const;

    /**
     * @brief Stops owning the resource and destroys it if its lifetime was not
     * temporarily extended by @see scoped_extender
     * @details If the lifetime of a resource was temporarily extended by
     * @see scoped_extender (i.e. in another thread) then it will be destroyed
     * when the last scoped_extender leaves the scope
     */
    void reset();

private:
    friend class weak_extender<T>;
    friend class scoped_extender<T>;

    struct resource_owner;

    using strong_lifetime_link = std::shared_ptr<resource_owner>;
    using weak_lifetime_link = std::weak_ptr<resource_owner>;

    strong_lifetime_link resource;
};

/**
 * @brief unique_extendable_ptr internal struct
 */
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
    /**
     * @brief Used to stop providing access to the resource immidiately
     * after the unique_extendable_ptr was destroyed (in case the access is
     * requested through a @see weak_extender that is still alive)
     */
    std::atomic_bool marked_for_destruction;
};

/**
 * @brief A convinience function with the same goals and behaviour as
 * std::make_unique<>()
 */
template <typename T, typename... CtorArgTypes>
unique_extendable_ptr<T> make_unique_extendable(CtorArgTypes&&... ctorArgs);

/**
 * @brief An object that does not extend the lifetime of a resource owned by
 * the corresponding @see unique_extendable_ptr but provides the means to
 * access it in a thread-safe way
 * @details Can be copied and moved without any impact on the lifetime of
 * the resource owned by @see unique_extendable_ptr
 */
template <typename T>
class weak_extender {
public:
    weak_extender() = default;
    explicit weak_extender(const unique_extendable_ptr<T>&);

    weak_extender(const weak_extender&) = default;
    weak_extender& operator=(const weak_extender&) = default;
    weak_extender(weak_extender&&) = default;
    weak_extender& operator=(weak_extender&&) = default;

    /**
     * @brief Returns an object that provides access to the resource
     * owned by @see unique_extendable_ptr
     */
    scoped_extender<T> lock() const;
    /**
     * @brief Stops assotiating itself with the corresponding @see unique_extendable_ptr
     */
    void reset();

private:
    friend class scoped_extender<T>;

    using resource = typename unique_extendable_ptr<T>::resource_owner;
    using weak_lifetime_link = typename unique_extendable_ptr<T>::weak_lifetime_link;

    static bool not_marked_for_destruction(const resource*);

    weak_lifetime_link link;
};

/**
 * @brief Provides thread-safe access to the resource owned by a corresponding
 * @see unique_extendable_ptr. An uncopiable and unmovable object that may be
 * retrieved only by weak_extender::lock() and may be caught only by const
 * reference. Can not be stored in a container or another object which makes it
 * in most cases to extend the lifetime of the resource only for a very short
 * time.
 */
template <typename T>
class scoped_extender {
public:
    scoped_extender(const scoped_extender&) = delete;
    scoped_extender& operator=(const scoped_extender&) = delete;
    scoped_extender& operator=(scoped_extender&&) = delete;

    T* get() const;
    T* operator->() const;

    bool empty() const;
    /**
     * @brief Stops assotiating itself with the corresponding @see unique_extendable_ptr
     */
    void reset();

private:
    friend class weak_extender<T>;

    using strong_lifetime_link = typename unique_extendable_ptr<T>::strong_lifetime_link;

    scoped_extender() = default;
    explicit scoped_extender(strong_lifetime_link);

    scoped_extender(scoped_extender&&) = default;

    strong_lifetime_link link;
};

#include "extendable_unique_ownership_impl.h"

#endif // _EXTENDABLE_UNIQUE_OWNERSHIP_
