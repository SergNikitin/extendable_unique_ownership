#ifndef _SHAREABLE_UNIQUE_OWNERSHIP_IMPL_
#define _SHAREABLE_UNIQUE_OWNERSHIP_IMPL_

template <typename T>
unique_extendable_ptr<T>::resource_owner::resource_owner()
    : marked_for_destruction(false) {}

template <typename T>
unique_extendable_ptr<T>::resource_owner::resource_owner(
    std::unique_ptr<T> resource)
    : marked_for_destruction(false)
    , resource(std::move(resource)) {}

template <typename T>
T* unique_extendable_ptr<T>::resource_owner::get() const {
    return resource.get();
}

template <typename T>
T* unique_extendable_ptr<T>::resource_owner::operator->() const {
    return get();
}


template <typename T>
unique_extendable_ptr<T>::unique_extendable_ptr(std::unique_ptr<T> resource)
    : resource(
        std::make_shared<resource_owner>(
            std::move(resource)
        )
    ) {}

template <typename T>
unique_extendable_ptr<T>::~unique_extendable_ptr() {
    reset();
}

template <typename T>
T* unique_extendable_ptr<T>::get() const {
    return resource->get();
}

template <typename T>
T* unique_extendable_ptr<T>::operator->() const {
    return get();
}

template <typename T>
void unique_extendable_ptr<T>::reset() {
    if (resource != nullptr) {
        resource->marked_for_destruction.store(true);
        resource.reset();
    }
}

template<typename T, typename... CtorArgTypes>
unique_extendable_ptr<T> make_unique_extendable(CtorArgTypes&&... ctorArgs) {
    auto unique = std::make_unique<T>(std::forward<CtorArgTypes>(ctorArgs)...);
    return unique_extendable_ptr<T>(std::move(unique));
}


template <typename T>
weak_extender<T>::weak_extender(const unique_extendable_ptr<T>& owner)
    : link(owner.resource) {}

template <typename T>
scoped_extender<T> weak_extender<T>::lock() const {
    auto strong_link = link.lock();
    return not_marked_for_destruction(strong_link.get())
        ? scoped_extender<T>(strong_link)
        : scoped_extender<T>();
}

template <typename T>
void weak_extender<T>::reset() {
    link.reset();
}

template <typename T>
/*static*/ bool weak_extender<T>::not_marked_for_destruction(const resource* resource) {
    return resource != nullptr && !resource->marked_for_destruction.load();
}


template <typename T>
scoped_extender<T>::scoped_extender(strong_lifetime_link link)
    : link(std::move(link)) {}

template <typename T>
T* scoped_extender<T>::get() const {
    return !empty() ? link->get() : nullptr;
}

template <typename T>
T* scoped_extender<T>::operator->() const {
    return get();
}

template <typename T>
bool scoped_extender<T>::empty() const {
    return link == nullptr;
}

template <typename T>
void scoped_extender<T>::reset() {
    link.reset();
}

#endif // _SHAREABLE_UNIQUE_OWNERSHIP_IMPL_
