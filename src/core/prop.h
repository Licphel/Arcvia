#pragma once

#include <functional>
#include <utility>

namespace arc {

template <typename Sig>
struct property;

template <typename Ret, typename... Args>
struct property<Ret(Args...)> {
    std::function<Ret(Args...)> f_;
    Ret v_;
    bool init;

    property() : v_{}, init(false) {}
    property(const property<Ret(Args...)>& p) = default;
    template <typename F, std::enable_if_t<std::is_invocable_v<F, Args...>, int> = 0>
    property(F&& f) : f_(std::forward<F>(f)), init(true) {}
    template <typename R>
    property(const R& r) : v_(r), init(true) {}

    Ret operator()(Args... args) const {
        if (f_) return f_(std::forward<Args>(args)...);
        return v_;
    }

    operator bool() const {
        return init;
    }
};

template <typename... Args>
struct property<void(Args...)> {
    std::function<void(Args...)> f_;
    bool init = false;

    property() : init(false) {}
    property(const property<void(Args...)>& p) = default;
    template <typename F, std::enable_if_t<std::is_invocable_v<F, Args...>, int> = 0>
    property(F&& f) : f_(std::forward<F>(f)), init(true) {}

    void operator()(Args... args) const {
        if (f_) f_(std::forward<Args>(args)...);
    }

    operator bool() const {
        return init;
    }
};

}  // namespace arc