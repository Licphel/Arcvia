#pragma once
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace arc {

enum class path_type { dir, file, unk };
enum class compression_level { no, fastest, optimal, smallest, raw_read, dcmp_read };

struct path {
    std::string strp;
    /* unstable */ fs::path npath_;

    path();
    path(const std::string& name);

    // regulate the path form.
    void check_();
    // returns the file name with extension.
    std::string file_name() const;
    // returns the file extension with the dot, like ".txt".
    std::string file_format() const;
    // get the subdir
    path operator/(const std::string& name) const;
    // get the relative path_, say 'minus' the #path from #this path_.
    // for example, "C:/dir/file.txt" - "C:/dir" = "file.txt".
    std::string operator-(const path& path_) const;

    // open a absolute path_.
    static path open(const std::string& name);
    // open a path relative to the execution path of the executable file.
    static path open_local(const std::string& name);
    static path execution_path();

    path parent() const;
    void del() const;
    void rename(const std::string& name) const;
    bool exists() const;
    // if the path is a file, create its parent directories and an empty file.
    // if the path is a directory, create it and its parent directories.
    void mkdirs() const;
    // get the type of the path_.
    path_type judge() const;
    std::vector<path> sub_dirs() const;
    // get all files in the directory, but not in its sub-directories.
    std::vector<path> sub_files() const;
    // get all files in the directory and its sub-directories, and, so on,
    // recursively.
    std::vector<path> recurse_files() const;
};

namespace io {

// note: if you want to read a compressed file, use compression_level::DCMP_READ
// instead of compression_level::RAW_READ.
std::vector<uint8_t> read_bytes(const path& path_, compression_level clvl = compression_level::raw_read);
void write_bytes(const path& path_, const std::vector<uint8_t>& data, compression_level clvl = compression_level::no);
std::string read_str(const path& path_);
void write_str(const path& path_, const std::string& text);
std::vector<uint8_t> compress(std::vector<uint8_t> buf, compression_level clvl = compression_level::optimal);
std::vector<uint8_t> decompress(std::vector<uint8_t> buf);

}  // namespace io

}  // namespace arc