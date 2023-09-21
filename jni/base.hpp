#pragma once
#include <string_view>
#include <functional>
#include <sys/types.h>
#include <dirent.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstdlib>

using namespace std;

void parse_prop_file(const char *file, const function<bool(string_view, string_view)> &fn);

#define ALLOW_MOVE_ONLY(clazz) \
clazz() = default;             \
clazz(const clazz&) = delete;  \
clazz(clazz &&o) { swap(o); }  \
clazz& operator=(clazz &&o) { swap(o); return *this; }

using sFILE = std::unique_ptr<FILE, decltype(&fclose)>;
sFILE make_file(FILE *fp);
using sDIR = std::unique_ptr<DIR, decltype(&closedir)>;
sDIR make_dir(DIR *dp);

static inline sDIR open_dir(const char *path) {
    return make_dir(opendir(path));
}

static inline sFILE open_file(const char *path, const char *mode) {
    return make_file(fopen(path, mode));
}

struct byte_view {
    byte_view() : _buf(nullptr), _sz(0) {}
    byte_view(const void *buf, size_t sz) : _buf((uint8_t *) buf), _sz(sz) {}

    // byte_view, or any of its subclass, can be copied as byte_view
    byte_view(const byte_view &o) : _buf(o._buf), _sz(o._sz) {}

    // String as bytes
    byte_view(const char *s, bool with_nul = true)
    : byte_view(std::string_view(s), with_nul, false) {}
    byte_view(const std::string &s, bool with_nul = true)
    : byte_view(std::string_view(s), with_nul, false) {}
    byte_view(std::string_view s, bool with_nul = true)
    : byte_view(s, with_nul, true /* string_view is not guaranteed to null terminate */ ) {}

    // Vector as bytes
    byte_view(const std::vector<uint8_t> &v) : byte_view(v.data(), v.size()) {}

    const uint8_t *buf() const { return _buf; }
    size_t sz() const { return _sz; }

protected:
    uint8_t *_buf;
    size_t _sz;

private:
    byte_view(std::string_view s, bool with_nul, bool check_nul)
    : byte_view(static_cast<const void *>(s.data()), s.length()) {
        if (with_nul) {
            if (check_nul && s[s.length()] != '\0')
                return;
            ++_sz;
        }
    }
};

// Interchangeable as `&mut [u8]` in Rust
struct byte_data : public byte_view {
    byte_data() = default;
    byte_data(void *buf, size_t sz) : byte_view(buf, sz) {}

    // byte_data, or any of its subclass, can be copied as byte_data
    byte_data(const byte_data &o) : byte_data(o._buf, o._sz) {}

    // Transparent conversion from common C++ types to mutable byte references
    byte_data(std::string &s, bool with_nul = true)
    : byte_data(s.data(), with_nul ? s.length() + 1 : s.length()) {}
    byte_data(std::vector<uint8_t> &v) : byte_data(v.data(), v.size()) {}

    void swap(byte_data &o) {
        std::swap(_buf, o._buf);
        std::swap(_sz, o._sz);
    }

    using byte_view::buf;
    uint8_t *buf() { return _buf; }
};

#define DISALLOW_COPY_AND_MOVE(clazz) \
clazz(const clazz &) = delete;        \
clazz(clazz &&) = delete;

template <class Func>
class run_finally {
    DISALLOW_COPY_AND_MOVE(run_finally)
public:
    explicit run_finally(Func &&fn) : fn(std::move(fn)) {}
    ~run_finally() { fn(); }
private:
    Func fn;
};

struct mmap_data : public byte_data {
    static_assert((sizeof(void *) == 8 && BLKGETSIZE64 == 0x80081272) ||
                  (sizeof(void *) == 4 && BLKGETSIZE64 == 0x80041272));
    ALLOW_MOVE_ONLY(mmap_data)

    explicit mmap_data(const char *name, bool rw = false);
    mmap_data(int fd, size_t sz, bool rw = false);
    ~mmap_data();
private:
    void init(int fd, size_t sz, bool rw);
};

int ssprintf(char *dest, size_t size, const char *fmt, ...);
size_t strscpy(char *dest, const char *src, size_t size);
int vssprintf(char *dest, size_t size, const char *fmt, va_list ap);

struct file_attr {
    struct stat st;
    char con[128];
};
void clone_attr(const char *src, const char *dest);