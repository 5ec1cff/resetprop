#include <sys/sysmacros.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <string>

#include "base.hpp"

using namespace std;

sDIR make_dir(DIR *dp) {
    return sDIR(dp, [](DIR *dp){ return dp ? closedir(dp) : 1; });
}

sFILE make_file(FILE *fp) {
    return sFILE(fp, [](FILE *fp){ return fp ? fclose(fp) : 1; });
}

void file_readline(bool trim, FILE *fp, const function<bool(string_view)> &fn) {
    size_t len = 1024;
    char *buf = (char *) malloc(len);
    char *start;
    ssize_t read;
    while ((read = getline(&buf, &len, fp)) >= 0) {
        start = buf;
        if (trim) {
            while (read && "\n\r "sv.find(buf[read - 1]) != string_view::npos)
                --read;
            buf[read] = '\0';
            while (*start == ' ')
                ++start;
        }
        if (!fn(start))
            break;
    }
    free(buf);
}

void file_readline(bool trim, const char *file, const function<bool(string_view)> &fn) {
    if (auto fp = open_file(file, "re"))
        file_readline(trim, fp.get(), fn);
}
void file_readline(const char *file, const function<bool(string_view)> &fn) {
    file_readline(false, file, fn);
}

void parse_prop_file(FILE *fp, const function<bool(string_view, string_view)> &fn) {
    file_readline(true, fp, [&](string_view line_view) -> bool {
        char *line = (char *) line_view.data();
        if (line[0] == '#')
            return true;
        char *eql = strchr(line, '=');
        if (eql == nullptr || eql == line)
            return true;
        *eql = '\0';
        return fn(line, eql + 1);
    });
}

void parse_prop_file(const char *file, const function<bool(string_view, string_view)> &fn) {
    if (auto fp = open_file(file, "re"))
        parse_prop_file(fp.get(), fn);
}

// https://github.com/topjohnwu/Magisk/commit/23c1f0111bb23d56d63fda0ca57e18c15e6b7811#diff-01079c251823f38d2a9fd0e9c999e4c959cc05b642f7364e60ae1cfc2707103bL3
void mmap_data::init(int fd, size_t sz, bool rw) {
    _sz = sz;
    void *b = sz > 0
            ? mmap(nullptr, sz, PROT_READ | PROT_WRITE, rw ? MAP_SHARED : MAP_PRIVATE, fd, 0)
            : nullptr;
    _buf = static_cast<uint8_t *>(b);
}

mmap_data::mmap_data(const char *name, bool rw) {
    int fd = open(name, (rw ? O_RDWR : O_RDONLY) | O_CLOEXEC);
    if (fd < 0)
        return;

    run_finally g([=] { close(fd); });
    struct stat st{};
    if (fstat(fd, &st))
        return;
    if (S_ISBLK(st.st_mode)) {
        uint64_t size;
        ioctl(fd, BLKGETSIZE64, &size);
        init(fd, size, rw);
    } else {
        init(fd, st.st_size, rw);
    }
}


mmap_data::~mmap_data() {
    if (_buf)
        munmap(_buf, _sz);
}

// https://github.com/topjohnwu/Magisk/blob/15e13a8d8bb61ed896df94881d63903cbfcc516b/native/src/base/misc.cpp#L273
int vssprintf(char *dest, size_t size, const char *fmt, va_list ap) {
    if (size > 0) {
        *dest = 0;
        return std::min(vsnprintf(dest, size, fmt, ap), (int) size - 1);
    }
    return -1;
}

int ssprintf(char *dest, size_t size, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    int r = vssprintf(dest, size, fmt, va);
    va_end(va);
    return r;
}

#include <sys/syscall.h>
#include <sys/xattr.h>

// https://github.com/topjohnwu/Magisk/blob/15e13a8d8bb61ed896df94881d63903cbfcc516b/native/src/base/selinux.cpp#L73
static int lsetfilecon(const char *path, const char *ctx) {
    return syscall(__NR_lsetxattr, path, XATTR_NAME_SELINUX, ctx, strlen(ctx) + 1, 0);
}

static void freecon(char *s) {
    free(s);
}

static int lgetfilecon(const char *path, char **ctx) {
    char buf[1024];
    int rc = syscall(__NR_lgetxattr, path, XATTR_NAME_SELINUX, buf, sizeof(buf) - 1);
    if (rc >= 0)
        *ctx = strdup(buf);
    return rc;
}

#undef strlcpy
size_t strscpy(char *dest, const char *src, size_t size) {
    return std::min(strlcpy(dest, src, size), size - 1);
}

int getattr(const char *path, file_attr *a) {
    if (lstat(path, &a->st) == -1)
        return -1;
    char *con;
    if (lgetfilecon(path, &con) == -1)
        return -1;
    strcpy(a->con, con);
    freecon(con);
    return 0;
}

int setattr(const char *path, file_attr *a) {
    if (chmod(path, a->st.st_mode & 0777) < 0)
        return -1;
    if (chown(path, a->st.st_uid, a->st.st_gid) < 0)
        return -1;
    if (a->con[0] && lsetfilecon(path, a->con) < 0)
        return -1;
    return 0;
}

void clone_attr(const char *src, const char *dest) {
    file_attr a;
    getattr(src, &a);
    setattr(dest, &a);
}