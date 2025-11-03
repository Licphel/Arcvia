#pragma once
#include <functional>
#include <map>
#include <stack>

#include "core/loc.h"

// *** reg.h util ***
#define ARC_REGISTERABLE \
    location loc;        \
    uint32_t id;

namespace arc {

template <typename T>
struct registry {
    // keep the reference accessible. map won't move them.
    std::map<location, T> mapped;
    std::map<uint32_t, T> linear;
    std::stack<std::function<void()>> delayed_;
    uint32_t idx_next_ = 0;
    T* nonnull = nullptr;

    T* make(const location& loc, const T& obj) {
        uint32_t idx = idx_next_++;
        mapped[loc] = obj;
        linear[idx] = obj;
        // here, the template value should have members reg_index & reg_id.
        delayed_.push([this, idx, loc]() {
            linear[idx].id = idx;
            linear[idx].loc = loc;
        });
        return &linear[idx];
    }

    T* make_nonnull(const location& loc, const T& obj) {
        T* ptr = make(loc, obj);
        nonnull = ptr;
        return ptr;
    }

    void work() {
        while (!delayed_.empty()) {
            delayed_.top()();
            delayed_.pop();
        }
    }

    T* operator[](uint32_t idx) { 
        if(idx < 0 || idx >= linear.size())
            return nonnull;
        return &linear[idx]; 
    }

    T* operator[](const location& loc) { 
        if(!mapped.count(loc))
            return nonnull;
        return &mapped[loc]; 
    }
};

}  // namespace arc
