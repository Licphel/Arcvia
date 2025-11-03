#include "core/time.h"

namespace arc {

watch::watch(double interval) : interval(interval), accp(0.0) {}

watch::watch(double interval, double perc) : interval(interval), accp(perc) {}

watch::operator bool() {
    double cs = clock::now().seconds;
    if (cs >= timestamp_ + interval) {
        timestamp_ = cs;
        return true;
    }
    return cs - timestamp_ >= (1.0 - accp) * interval;
}

static clock clock_;

clock& clock::now() { return clock_; }

double lerp(double old, double now) { return old + clock_.partial * (now - old); }

}  // namespace arc
