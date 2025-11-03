#pragma once
#include <memory>

#include "core/io.h"
#include "core/math.h"

namespace arc {

struct track {
    /* unstable */ unsigned int track_id_;
    double sec_len;

    ~track();

    static std::shared_ptr<track> load(const path& path_);
};

enum class device_option { listener, roll_off, ref_dist, max_dist };

enum class clip_op {
    // parameters
    location,
    gain,
    pitch,
    // operations
    loop,
    play,
    pause,
    stop
};

enum class clip_status { playing, idle, end, paused };

struct clip {
    std::shared_ptr<track> relying_track;
    /* unstable */ unsigned int clip_id_;

    ~clip();
    clip_status status();
    void set(clip_op param, double v);
    void set(clip_op param, const vec2& v);
    void set(clip_op param, const vec3& v);
    // play, loop, pause or stop the clip.
    // note: once the clip is stopped, it cannot be played again.
    void operate(clip_op param);

    static std::shared_ptr<clip> make(std::shared_ptr<track> track);
};

void tk_make_device();
void tk_end_make_device();

// these options should be set between #tk_make_device and #tk_end_make_device.
void tk_set_device_option(device_option opt, double v);
void tk_set_device_option(device_option opt, const vec2& v);
void tk_set_device_option(device_option opt, const vec3& v);

}  // namespace arc