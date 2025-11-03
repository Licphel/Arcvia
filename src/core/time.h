#pragma once

namespace arc {

struct watch {
    double interval;
    double timestamp_;
    double accp;

    watch(double interval);
    watch(double interval, double perc);
    operator bool();
};

struct clock {
    double partial;
    double delta;
    long ticks;
    long render_ticks;
    double seconds;

    static clock& now();
};

double lerp(double old, double now);

}  // namespace arc