#pragma once

#include <cstddef>
#include <memory>

namespace arc {

// obs means observer pointer, a raw-pointer-like weak pointer.
template <typename T>
struct obs {
   private:
    T* ptr_ = nullptr;
    std::weak_ptr<T> weak_;
    bool unsafe = false;

    obs(T* ptr) : ptr_(ptr), unsafe(true) {}

   public:
    obs() = default;
    obs(nullptr_t) {}
    obs(const std::shared_ptr<T>& sp) : ptr_(sp.get()), weak_(sp) {}
    obs(const obs&) = default;
    obs& operator=(const obs&) = default;

    operator T*() const { return expired() ? nullptr : ptr_; }
    T* get() const { return expired() ? nullptr : ptr_; }
    T& operator*() const { return *get(); }
    T* operator->() const { return get(); }

    inline bool expired() const {
        if (unsafe) return false;
        return weak_.expired();
    }
    inline std::shared_ptr<T> lock() const { return weak_.lock(); }
    explicit operator bool() const { return get() != nullptr; }

    // dev should ensure that during the usage of the returned obsptr, the object won't be deleted.
    template <typename R>
    static obs<R> unsafe_make(R* rptr) {
        return obs<R>(rptr);
    }
};

}  // namespace arc