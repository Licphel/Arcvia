#include <al/al.h>

#include "audio/device.h"
#include "core/io.h"
#include "core/log.h"

namespace arc {

std::shared_ptr<track> wav_load_(const path& path_) {
    auto file = io::read_bytes(path_);

    size_t index = 0;

    if (file.size() < 12) print_throw(log_level::fatal, "too small file: {}", path_.strp);

    if (file[index++] != 'R' || file[index++] != 'I' || file[index++] != 'F' || file[index++] != 'F')
        print_throw(log_level::fatal, "not a wave file: {}", path_.strp);

    index += 4;

    if (file[index++] != 'W' || file[index++] != 'A' || file[index++] != 'V' || file[index++] != 'E')
        print_throw(log_level::fatal, "not a wave file: {}", path_.strp);

    int samp_rate = 0;
    int16_t bps = 0;
    int16_t n_ch = 0;
    int byte_size = 0;
    ALenum format = 0;

    ALuint buffer;
    alGenBuffers(1, &buffer);

    while (index + 8 <= file.size()) {
        std::string identifier(4, '\0');
        identifier[0] = file[index++];
        identifier[1] = file[index++];
        identifier[2] = file[index++];
        identifier[3] = file[index++];

        uint32_t chunk_size = *reinterpret_cast<const uint32_t*>(&file[index]);
        index += 4;

        if (index + chunk_size > file.size()) print_throw(log_level::fatal, "invalid chunk size: {}", path_.strp);

        if (identifier == "fmt ") {
            if (chunk_size != 16) print_throw(log_level::fatal, "unknown format: {}", path_.strp);

            int16_t audio_format = *reinterpret_cast<const int16_t*>(&file[index]);
            index += 2;
            if (audio_format != 1) print_throw(log_level::fatal, "unknown format: {}", path_.strp);

            n_ch = *reinterpret_cast<const int16_t*>(&file[index]);
            index += 2;
            samp_rate = *reinterpret_cast<const int32_t*>(&file[index]);
            index += 4;
            index += 4;
            index += 2;
            bps = *reinterpret_cast<const int16_t*>(&file[index]);
            index += 2;

            if (n_ch == 1) {
                if (bps == 8)
                    format = AL_FORMAT_MONO8;
                else if (bps == 16)
                    format = AL_FORMAT_MONO16;
                else
                    print_throw(log_level::fatal, "can't play mono " + std::to_string(bps) + " sound.");
            } else if (n_ch == 2) {
                if (bps == 8)
                    format = AL_FORMAT_STEREO8;
                else if (bps == 16)
                    format = AL_FORMAT_STEREO16;
                else
                    print_throw(log_level::fatal, "can't play stereo " + std::to_string(bps) + " sound.");
            } else
                print_throw(log_level::fatal, "can't play audio with " + std::to_string(n_ch) + " channels");
        } else if (identifier == "data") {
            byte_size = chunk_size;
            const uint8_t* data = &file[index];
            alBufferData(buffer, format, data, chunk_size, samp_rate);
            index += chunk_size;
        } else if (identifier == "JUNK" || identifier == "iXML")
            index += chunk_size;
        else
            index += chunk_size;
    }

    std::shared_ptr<track> ptr = std::make_shared<track>();
    ptr->track_id_ = buffer;
    ptr->sec_len = static_cast<double>(byte_size) / (samp_rate * bps / 8.0) / n_ch;

    return ptr;
}

}  // namespace arc