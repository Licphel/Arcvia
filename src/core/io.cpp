#include "core/io.h"

#include <fstream>

#include "core/def.h"
#include "core/log.h"

// clang-format off
#define BROTLI_IMPLEMENTATION
#include <brotli/decode.h>
#include <brotli/encode.h>
// clang-format on

namespace arc {

path::path() = default;
path::path(const std::string& name) : strp(name), npath_(fs::path(name)) { check_(); }

void path::check_() {
    for (size_t pos = 0; (pos = strp.find('\\', pos)) != std::string::npos;) strp.replace(pos, 1, "/");
}

std::string path::file_name() const { return npath_.filename().string(); }
std::string path::file_format() const { return npath_.extension().string(); }

path path::operator/(const std::string& name) const {
    bool append = strp[strp.length() - 1] != '/';
    return path(append ? strp + '/' + name : strp + name);
}

std::string path::operator-(const path& path_) const { return fs::relative(npath_, path_.npath_).generic_string(); }

path path::open(const std::string& name) { return path(name); }
path path::open_local(const std::string& name) { return execution_path() / name; }
path path::execution_path() { return path(fs::current_path().string()) / ARC_LIB_NAME; }

path path::parent() const { return {npath_.parent_path().string()}; }
void path::del() const { fs::remove(npath_); }
void path::rename(const std::string& name) const { fs::rename(npath_, name); }
bool path::exists() const { return fs::exists(npath_); }

void path::mkdirs() const {
    fs::create_directories(judge() == path_type::dir ? npath_ : parent().npath_);
    if (judge() == path_type::file) {
        std::ofstream stream(npath_);
        stream.close();
    }
}

path_type path::judge() const {
    if (fs::is_directory(npath_)) return path_type::dir;
    if (fs::is_regular_file(npath_)) return path_type::file;
    return path_type::unk;
}

std::vector<path> path::sub_dirs() const {
    std::vector<path> paths;
    for (auto& k : fs::directory_iterator(npath_)) {
        if (k.is_directory()) paths.push_back(path(k.path().string()));
    }
    return paths;
}

std::vector<path> path::sub_files() const {
    std::vector<path> paths;
    for (auto& k : fs::directory_iterator(npath_)) {
        if (k.is_regular_file()) paths.push_back(path(k.path().string()));
    }
    return paths;
}

std::vector<path> path::recurse_files() const {
    std::vector<path> paths;
    for (auto& k : fs::recursive_directory_iterator(npath_)) {
        if (k.is_regular_file()) paths.push_back(path(k.path().string()));
    }
    return paths;
}

static std::vector<uint8_t> brotli_compress(const std::vector<uint8_t>& src, int quality) {
    if (src.empty()) return {};
    size_t max_sz = BrotliEncoderMaxCompressedSize(src.size());
    std::vector<uint8_t> out(max_sz);
    size_t encoded_sz = max_sz;
    if (BROTLI_TRUE != BrotliEncoderCompress(quality, BROTLI_DEFAULT_WINDOW, BROTLI_DEFAULT_MODE, src.size(),
                                             src.data(), &encoded_sz, out.data()))
        print_throw(log_level::fatal, "Brotli encoder failed");
    out.resize(encoded_sz);
    return out;
}

static std::vector<uint8_t> brotli_decompress(const std::vector<uint8_t>& src) {
    if (src.empty()) return {};
    std::vector<uint8_t> dst;
    size_t avail_in = src.size();
    const uint8_t* nxt_in = src.data();
    BrotliDecoderState* st = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
    if (!st) print_throw(log_level::fatal, "brotli decoder create failed");

    for (;;) {
        uint8_t buf[64 * 1024];
        size_t avail_out = sizeof(buf);
        uint8_t* nxt_out = buf;
        auto rc = BrotliDecoderDecompressStream(st, &avail_in, &nxt_in, &avail_out, &nxt_out, nullptr);
        size_t produced = sizeof(buf) - avail_out;
        if (produced) dst.insert(dst.end(), buf, buf + produced);
        if (rc == BROTLI_DECODER_RESULT_SUCCESS) break;
        if (rc == BROTLI_DECODER_RESULT_ERROR) print_throw(log_level::fatal, "brotli decoder error");
    }
    BrotliDecoderDestroyInstance(st);
    return dst;
}

std::vector<uint8_t> io::read_bytes(const path& path_, compression_level clvl) {
    std::ifstream file(path_.npath_, std::ios::binary | std::ios::ate);
    if (!file) print_throw(log_level::fatal, "cannot find {}", path_.strp);
    size_t len = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> raw(len);
    file.read(reinterpret_cast<char*>(raw.data()), len);
    if (!file) print_throw(log_level::fatal, "short read in {}", path_.strp);

    if (clvl == compression_level::raw_read) return raw;

    return io::decompress(raw);
}

void io::write_bytes(const path& path_, const std::vector<uint8_t>& data, compression_level clvl) {
    if (!path_.exists()) path_.mkdirs();
    auto out = io::compress(data, clvl);
    std::ofstream file(path_.npath_, std::ios::binary);
    if (!file) print_throw(log_level::fatal, "cannot open {} for write", path_.strp);
    file.write(reinterpret_cast<const char*>(out.data()), out.size());
}

std::string io::read_str(const path& path_) {
    auto raw = io::read_bytes(path_);
    return std::string(reinterpret_cast<const char*>(raw.data()), raw.size());
}

void io::write_str(const path& path_, const std::string& text) {
    io::write_bytes(path_, std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(text.data()),
                                                reinterpret_cast<const uint8_t*>(text.data() + text.size())));
}

std::vector<uint8_t> io::compress(std::vector<uint8_t> buf, compression_level clvl) {
    std::vector<uint8_t> out;
    switch (clvl) {
        case compression_level::no:
            out = buf;
            break;
        case compression_level::fastest:
            out = brotli_compress(buf, 1);
            break;
        case compression_level::optimal:
            out = brotli_compress(buf, 6);
            break;
        case compression_level::smallest:
            out = brotli_compress(buf, 11);
            break;
        default:
            print_throw(log_level::fatal, "unsupported compression level {}", static_cast<int>(clvl));
    }
    return out;
}

std::vector<uint8_t> io::decompress(std::vector<uint8_t> buf) { return brotli_decompress(buf); }

}  // namespace arc