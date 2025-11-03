#include "gfx/device.h"

#include <al/al.h>
#include <al/alc.h>

#include "audio/device.h"
#include "core/math.h"

namespace arc {

static ALCdevice* al_dev;
static ALCcontext* al_ctx;
static std::vector<std::shared_ptr<clip>> clip_r;
static double rolloff = 1.0;
static double max_dist = 1.0;
static double ref_dist = 1.0;

track::~track() { alDeleteBuffers(1, &track_id_); }

extern std::shared_ptr<track> wav_load_(const path& path_);

std::shared_ptr<track> track::load(const path& path_) { return wav_load_(path_); }

clip::~clip() { alDeleteSources(1, &clip_id_); }

clip_status clip::status() {
    int sts;
    alGetSourcei(clip_id_, AL_SOURCE_STATE, &sts);
    if (sts == AL_STOPPED) return clip_status::end;
    if (sts == AL_PLAYING) return clip_status::playing;
    if (sts == AL_INITIAL) return clip_status::idle;
    return clip_status::paused;
}

void clip::set(clip_op param, double v) {
    if (param == clip_op::gain)
        alSourcef(clip_id_, AL_GAIN, v);
    else if (param == clip_op::pitch)
        alSourcef(clip_id_, AL_PITCH, v);
}

void clip::set(clip_op param, const vec2& v) {
    if (param == clip_op::location) alSource3f(clip_id_, AL_POSITION, v.x, v.y, 0);
}

void clip::set(clip_op param, const vec3& v) {
    if (param == clip_op::location) alSource3f(clip_id_, AL_POSITION, v.x, v.y, 0);
}

void clip::operate(clip_op param) {
    if (param == clip_op::play)
        alSourcePlay(clip_id_);
    else if (param == clip_op::loop) {
        alSourcei(clip_id_, AL_LOOPING, AL_TRUE);
        alSourcePlay(clip_id_);
    } else if (param == clip_op::pause)
        alSourcePause(clip_id_);
    else if (param == clip_op::stop)
        alSourceStop(clip_id_);
}

std::shared_ptr<clip> clip::make(std::shared_ptr<track> track) {
    unsigned int id;
    alGenSources(1, &id);
    alSourcei(id, AL_BUFFER, track->track_id_);
    alSourcef(id, AL_ROLLOFF_FACTOR, rolloff);
    alSourcef(id, AL_REFERENCE_DISTANCE, ref_dist);
    alSourcef(id, AL_MAX_DISTANCE, max_dist);

    std::shared_ptr<clip> ptr = std::make_shared<clip>();
    ptr->relying_track = track;
    ptr->clip_id_ = id;
    clip_r.push_back(ptr);
    return ptr;
}

void process_tracks_() {
    auto it = clip_r.begin();
    while (it != clip_r.end()) {
        auto& cl = *it;
        if (cl->status() == clip_status::end)
            it = clip_r.erase(it);
        else
            ++it;
    }
}

void tk_make_device() {
    // init openal
    al_dev = alcOpenDevice(nullptr);
    al_ctx = alcCreateContext(al_dev, nullptr);
    alcMakeContextCurrent(al_ctx);
    event_tick += process_tracks_;
    event_dispose += []() {
        alcDestroyContext(al_ctx);
        alcCloseDevice(al_dev);
    };
}

void tk_end_make_device() {
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    // nothing
}

void tk_set_device_option(device_option opt, double v) {
    if (opt == device_option::roll_off)
        rolloff = v;
    else if (opt == device_option::ref_dist)
        ref_dist = v;
    else if (opt == device_option::max_dist)
        max_dist = v;
}

void tk_set_device_option(device_option opt, const vec2& v) {
    if (opt == device_option::listener) alListener3f(AL_POSITION, v.x, v.y, 0);
}

void tk_set_device_option(device_option opt, const vec3& v) {
    if (opt == device_option::listener) alListener3f(AL_POSITION, v.x, v.y, v.z);
}

}  // namespace arc
