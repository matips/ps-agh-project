#include "context.h"

#define FUSE_USE_VERSION 30
#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/param.h>
#include <limits.h>
#include <stdlib.h>

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

char root[PATH_MAX + 1];
FILE *log_f;

static int ps_getattr(const char *path, struct stat *stbuf) {
    path = transform_path(root, path);
    int res;
    res = lstat(path, stbuf);
    if (res == -1)
        return -errno;
    return 0;
}

static int ps_access(const char *path, int mask) {
    path = transform_path(root, path);
    int res;
    res = access(path, mask);
    if (res == -1)
        return -errno;
    return 0;
}

static int ps_readlink(const char *path, char *buf, size_t size) {
    path = transform_path(root, path);
    int res;
    res = readlink(path, buf, size - 1);
    if (res == -1)
        return -errno;
    buf[res] = '\0';
    return 0;
}

static int ps_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi) {

    path = transform_path(root, path);

    DIR *dp;
    struct dirent *de;
    (void) offset;
    (void) fi;
    dp = opendir(path);
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;

    }
    closedir(dp);
    return 0;
}

static int ps_mknod(const char *path, mode_t mode, dev_t rdev) {
    path = transform_path(root, path);
    int res;
    /* On Linux this could just be 'mknod(path, mode, rdev)' but this
       is more portable */
    if (S_ISREG(mode)) {
        res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (res >= 0)
            res = close(res);
    } else if (S_ISFIFO(mode))
        res = mkfifo(path, mode);
    else
        res = mknod(path, mode, rdev);
    if (res == -1)
        return -errno;
    return 0;
}

static int ps_mkdir(const char *path, mode_t mode) {
    path = transform_path(root, path);
    int res;
    res = mkdir(path, mode);
    if (res == -1)
        return -errno;
    return 0;
}

static int ps_unlink(const char *path) {
    path = transform_path(root, path);
    int res;
    res = unlink(path);
    if (res == -1)
        return -errno;
    return 0;
}

static int ps_rmdir(const char *path) {
    path = transform_path(root, path);
    int res;
    res = rmdir(path);
    if (res == -1)
        return -errno;
    return 0;
}

static int ps_symlink(const char *from, const char *to) {
    from = transform_path(root, from);
    to = transform_path(root, to);
    int res;
    res = symlink(from, to);
    if (res == -1)
        return -errno;
    return 0;
}

static int ps_rename(const char *from, const char *to) {
    from = transform_path(root, from);
    to = transform_path(root, to);
    int res;
    res = rename(from, to);
    if (res == -1)
        return -errno;
    return 0;
}

static int ps_link(const char *from, const char *to) {
    from = transform_path(root, from);
    to = transform_path(root, to);
    int res;
    res = link(from, to);
    if (res == -1)
        return -errno;
    return 0;
}

static int ps_chmod(const char *path, mode_t mode) {
    path = transform_path(root, path);
    int res;
    res = chmod(path, mode);
    if (res == -1)
        return -errno;
    return 0;
}

static int ps_chown(const char *path, uid_t uid, gid_t gid) {
    path = transform_path(root, path);
    int res;
    res = lchown(path, uid, gid);
    if (res == -1)
        return -errno;
    return 0;
}

static int ps_truncate(const char *path, off_t size) {
    path = transform_path(root, path);
    int res;
    res = truncate(path, size);
    if (res == -1)
        return -errno;
    return 0;
}

#ifdef HAVE_UTIMENSAT
static int ps_utimens(const char *path, const struct timespec ts[2])
{
    path = transform_path(root, path);
        int res;
        /* don't use utime/utimes since they follow symlinks */
        res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
        if (res == -1)
                return -errno;
        return 0;
}
#endif

static int ps_open(const char *path, struct fuse_file_info *fi) {
    path = transform_path(root, path);
    int res;
    res = open(path, fi->flags);
    if (res == -1)
        return -errno;
    close(res);
    return 0;
}

static int ps_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {
    path = transform_path(root, path);
    int fd;
    int res;
    (void) fi;
    fd = open(path, O_RDONLY);
    if (fd == -1)
        return -errno;
    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;
    close(fd);
    return res;
}

static int ps_write(const char *path, const char *buf, size_t size,
        off_t offset, struct fuse_file_info *fi) {
    path = transform_path(root, path);
    int fd;
    int res;
    (void) fi;
    fd = open(path, O_WRONLY);
    if (fd == -1)
        return -errno;
    res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;
    close(fd);
    return res;
}

static int ps_statfs(const char *path, struct statvfs *stbuf) {
    path = transform_path(root, path);
    int res;
    res = statvfs(path, stbuf);
    if (res == -1)
        return -errno;
    return 0;
}

static int ps_release(const char *path, struct fuse_file_info *fi) {
    /* Just a stub.  This method is optional and can safely be left
       unimplemented */
    (void) path;
    (void) fi;
    return 0;
}

static int ps_fsync(const char *path, int isdatasync,
        struct fuse_file_info *fi) {
    /* Just a stub.  This method is optional and can safely be left
       unimplemented */
    (void) path;
    (void) isdatasync;
    (void) fi;
    return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int ps_fallocate(const char *path, int mode,
                        off_t offset, off_t length, struct fuse_file_info *fi)
{
    path = transform_path(root, path);
        int fd;
        int res;
        (void) fi;
        if (mode)
                return -EOPNOTSUPP;
        fd = open(path, O_WRONLY);
        if (fd == -1)
                return -errno;
        res = -posix_fallocate(fd, offset, length);
        close(fd);
        return res;
}
#endif
#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int ps_setxattr(const char *path, const char *name, const char *value,
                        size_t size, int flags)
{    path = transform_path(root, path);

        int res = lsetxattr(path, name, value, size, flags);
        if (res == -1)
                return -errno;
        return 0;
}
static int ps_getxattr(const char *path, const char *name, char *value,
                        size_t size)
{    path = transform_path(root, path);

        int res = lgetxattr(path, name, value, size);
        if (res == -1)
                return -errno;
        return res;
}
static int ps_listxattr(const char *path, char *list, size_t size)
{    path = transform_path(root, path);

        int res = llistxattr(path, list, size);
        if (res == -1)
                return -errno;
        return res;
}
static int ps_removexattr(const char *path, const char *name)
{    path = transform_path(root, path);

        int res = lremovexattr(path, name);
        if (res == -1)
                return -errno;
        return 0;
}
#endif /* HAVE_SETXATTR */
static struct fuse_operations ps_oper = {
        .getattr        = ps_getattr,
        .access         = ps_access,
        .readlink       = ps_readlink,
        .readdir        = ps_readdir,
        .mknod          = ps_mknod,
        .mkdir          = ps_mkdir,
        .symlink        = ps_symlink,
        .unlink         = ps_unlink,
        .rmdir          = ps_rmdir,
        .rename         = ps_rename,
        .link           = ps_link,
        .chmod          = ps_chmod,
        .chown          = ps_chown,
        .truncate       = ps_truncate,
#ifdef HAVE_UTIMENSAT
        .utimens        = ps_utimens,
#endif
        .open           = ps_open,
        .read           = ps_read,
        .write          = ps_write,
        .statfs         = ps_statfs,
        .release        = ps_release,
        .fsync          = ps_fsync,
#ifdef HAVE_POSIX_FALLOCATE
        .fallocate      = ps_fallocate,
#endif
#ifdef HAVE_SETXATTR
        .setxattr       = ps_setxattr,
        .getxattr       = ps_getxattr,
        .listxattr      = ps_listxattr,
        .removexattr    = ps_removexattr,
#endif
};

int main(int argc, char *argv[]) {
    umask(0);
    //first paramter is for me, others for fuse
    if (argc < 3) {
        printf("usage: %s <storage> <mouned_dir_and_fuse_options>", argv[0]);
        return 1;
    }
    realpath(argv[1], root);

    argv[1] = argv[0];
    return fuse_main(argc - 1, argv + 1, &ps_oper, NULL);
}