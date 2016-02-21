#ifndef _SHAREABLE_UNIQUE_OWNERSHIP_IMPL_
#define _SHAREABLE_UNIQUE_OWNERSHIP_IMPL_

template <typename T>
OwningRef<T>::ResourceOwner::ResourceOwner() : markedForDestruction(false) {}

template <typename T>
OwningRef<T>::ResourceOwner::ResourceOwner(std::unique_ptr<T> resource)
    : markedForDestruction(false)
    , resource(std::move(resource)) {}

template <typename T>
T* OwningRef<T>::ResourceOwner::get() const {
    return resource.get();
}

template <typename T>
T* OwningRef<T>::ResourceOwner::operator->() const {
    return get();
}


template <typename T>
OwningRef<T>::OwningRef(T* resource)
    : resource(
        std::make_shared<ResourceOwner>(
            std::unique_ptr<T>(resource)
        )
    ) {}

template <typename T>
OwningRef<T>::OwningRef(std::unique_ptr<T> resource)
    : resource(
        std::make_shared<ResourceOwner>(
            std::move(resource)
        )
    ) {}

template <typename T>
OwningRef<T>::~OwningRef() {
    reset();
}

template <typename T>
T* OwningRef<T>::get() const {
    return resource->get();
}

template <typename T>
T* OwningRef<T>::operator->() const {
    return get();
}

template <typename T>
void OwningRef<T>::reset() {
    if (resource != nullptr) {
        resource->markedForDestruction.store(true);
        resource.reset();
    }
}

template<typename T, typename... CtorArgTypes>
OwningRef<T> makeOwning(CtorArgTypes&&... ctorArgs) {
    return OwningRef<T>(
        std::make_unique<T>(
            std::forward<CtorArgTypes>(ctorArgs)...
        )
    );
}


template <typename T>
WeakRef<T>::WeakRef(const OwningRef<T>& owner) : resource(owner.resource) {}

template <typename T>
ScopedRef<T> WeakRef<T>::lock() const {
    auto sharedPtr = resource.lock();
    return notMarkedForDestruction(sharedPtr.get())
        ? ScopedRef<T>(sharedPtr)
        : ScopedRef<T>();
}

template <typename T>
void WeakRef<T>::reset() {
    resource.reset();
}

template <typename T>
/*static*/ bool WeakRef<T>::notMarkedForDestruction(const Resource* resource) {
    return resource != nullptr && !resource->markedForDestruction.load();
}


template <typename T>
ScopedRef<T>::ScopedRef(const std::shared_ptr<Resource>& resource)
    : resource(resource) {}

template <typename T>
T* ScopedRef<T>::get() const {
    return resource->get();
}

template <typename T>
T* ScopedRef<T>::operator->() const {
    return get();
}

template <typename T>
bool ScopedRef<T>::empty() const {
    return resource == nullptr;
}

template <typename T>
void ScopedRef<T>::reset() {
    resource.reset();
}

#endif // _SHAREABLE_UNIQUE_OWNERSHIP_IMPL_
