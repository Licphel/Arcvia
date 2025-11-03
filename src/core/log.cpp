#include "core/log.h"

#include <fstream>

#include "core/io.h"

namespace arc {

static std::ofstream logstream_ = std::ofstream(path::open_local("latest.log").npath_);

void log_redirect(const std::string& logv) {
    logstream_ << logv << std::endl;
    logstream_ << std::flush;
}

std::string get_header_(log_level type) {
    std::string header;
    switch (type) {
        case log_level::info:
            header = "[INFO] ";
            break;
        case log_level::warn:
            header = "[WARN] ";
            break;
        case log_level::fatal:
            header = "[fatal] ";
            break;
    };
    return header;
}

}  // namespace arc