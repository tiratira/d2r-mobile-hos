#include "gfx/nvn/nvn_dirty.h"
/*
 * hle_fs.c — nn::fs 服务层 HLE(上游 dm/host/hle_fs.c 的 HOS 最小集)。
 *
 * Horizon 的文件服务(fsp-srv)在宿主里不存在,SDK 里的 fs 客户端若真去 IPC
 * 会挂死。这里强制接管 24 个文件接口,把客体路径落到应用沙箱:
 *   rom  → <filesDir>/romfs(只读;使用者用 tools/extract_romfs_hos.py 提取)
 *   save → <filesDir>/save (可写;存档)
 *   temp → <filesDir>/temp (可写;临时)
 * 也接受 DM_ROMFS / DM_SAVE / DM_TEMP 环境变量覆盖(开发机自检用)。
 *
 * 路径规则(照上游):以第一个 ':' 切挂载名(区分大小写、必须与挂载表逐字节
 * 相等);挂载内逐段解析,空段与 "." 忽略,".." 立即失败(防逃逸);只有
 * Create/Rename 的目标允许最后一段不存在。
 *
 * 句柄:HOS 用「表下标 + 1」而不是宿主 fd(Horizon 的 FileHandle 0 恒无效,
 * 且 fd 可能撞 0)。目录句柄为堆对象(DIR* + 打开方式)。
 *
 * 存档事务(上游 clonefile + renamex_np RENAME_SWAP)在 HOS P0 版不做:
 * Commit = fsync + RES_OK。写失败仍返回 RES_FAIL,由上层自行处理。
 */
#include "host/dm.h"
#include "hal/dm_platform.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define FS_PATH_MAX 1024
#define FS_MNT_MAX 8
#define FS_FH_MAX 1024
#define FS_DH_MAX 256

#define RES_PATH_NOT_FOUND 0x202u
#define RES_PATH_EXISTS 0x402u

typedef struct {
    char name[16];
    char host[512];
    int writable;
} fs_mnt;

typedef struct {
    int fd;
    int mode;   /* 1 读 / 2 写 / 4 追加 */
    int used;
} fs_fh;

typedef struct {
    DIR *dir;
    int mode;   /* 1 只列目录 / 2 只列文件 / 0 全列 */
    int used;
} fs_dh;

typedef struct {
    char name[0x301];
    uint8_t pad0[3];
    int8_t type;      /* 0 目录 / 1 文件 */
    uint8_t pad1[3];
    int64_t size;
} DirectoryEntry;

static fs_mnt g_mnt[FS_MNT_MAX];
static int g_nmnt;
static fs_fh g_fh[FS_FH_MAX];
static fs_dh g_dh[FS_DH_MAX];
static pthread_mutex_t g_fs_mu = PTHREAD_MUTEX_INITIALIZER;
static char g_romfs[512], g_save[512], g_temp[512];
static int g_inited;

void dmFsSetRoots(const char *romfs, const char *save)
{
    pthread_mutex_lock(&g_fs_mu);
    if (romfs != NULL) {
        snprintf(g_romfs, sizeof(g_romfs), "%s", romfs);
    }
    if (save != NULL) {
        snprintf(g_save, sizeof(g_save), "%s", save);
    }
    g_inited = 0;
    pthread_mutex_unlock(&g_fs_mu);
}

static int mkdir_p(const char *path)
{
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/') {
            continue;
        }
        *p = 0;
        mkdir(tmp, 0755);
        *p = '/';
    }
    return (mkdir(tmp, 0755) == 0 || errno == EEXIST) ? 0 : -1;
}

static void trim_slash(char *p)
{
    size_t n = strlen(p);
    while (n > 1 && p[n - 1] == '/') {
        p[--n] = 0;
    }
}

static void mount_add(const char *name, const char *host, int writable)
{
    if (g_nmnt >= FS_MNT_MAX) {
        return;
    }
    snprintf(g_mnt[g_nmnt].name, sizeof(g_mnt[0].name), "%s", name);
    snprintf(g_mnt[g_nmnt].host, sizeof(g_mnt[0].host), "%s", host);
    trim_slash(g_mnt[g_nmnt].host);
    g_mnt[g_nmnt].writable = writable;
    g_nmnt++;
}

/* 客体的挂载名由游戏自己定(实测 3.3.94012:rom="$",临时="^"),必须按名字登记,
 * 否则 "^:/Diablo II/GameLogs" 找不到挂载点 → 写成只读失败 → 游戏初始化退出。 */
static void mount_set(const char *name, const char *host, int writable)
{
    if (name == NULL || name[0] == 0) {
        return;
    }
    for (int i = 0; i < g_nmnt; i++) {
        if (strcmp(g_mnt[i].name, name) == 0) {
            snprintf(g_mnt[i].host, sizeof(g_mnt[0].host), "%s", host);
            trim_slash(g_mnt[i].host);
            g_mnt[i].writable = writable;
            return;
        }
    }
    mount_add(name, host, writable);
}

void dmFsInit(void)
{
    pthread_mutex_lock(&g_fs_mu);
    if (g_inited) {
        pthread_mutex_unlock(&g_fs_mu);
        return;
    }
    g_inited = 1;
    const char *base = dmPlatGetFilesDir();
    if (base == NULL || base[0] == 0) {
        base = "/data/storage/el2/base/files";
    }
    const char *e;
    if (g_romfs[0] == 0) {
        e = getenv("DM_ROMFS");
        if (e != NULL && e[0] != 0) {
            snprintf(g_romfs, sizeof(g_romfs), "%s", e);
        } else {
            snprintf(g_romfs, sizeof(g_romfs), "%s/romfs", base);
        }
    }
    if (g_save[0] == 0) {
        e = getenv("DM_SAVE");
        if (e != NULL && e[0] != 0) {
            snprintf(g_save, sizeof(g_save), "%s", e);
        } else {
            snprintf(g_save, sizeof(g_save), "%s/save", base);
        }
    }
    if (g_temp[0] == 0) {
        e = getenv("DM_TEMP");
        if (e != NULL && e[0] != 0) {
            snprintf(g_temp, sizeof(g_temp), "%s", e);
        } else {
            snprintf(g_temp, sizeof(g_temp), "%s/temp", base);
        }
    }
    mkdir_p(g_romfs);
    mkdir_p(g_save);
    mkdir_p(g_temp);
    g_nmnt = 0;
    mount_add("rom", g_romfs, 0);
    mount_add("save", g_save, 1);
    mount_add("temp", g_temp, 1);
    DMLOG_I("fs: rom=%{public}s save=%{public}s temp=%{public}s",
            g_romfs, g_save, g_temp);
    pthread_mutex_unlock(&g_fs_mu);
}

/* 客体路径 → 宿主路径。返回 0 成功,-1 无挂载/越界。for_write 只用于无挂载前缀的缺省。 */
static int fs_resolve(const char *guest, char *out, size_t n, int for_write)
{
    dmFsInit();
    if (guest == NULL || guest[0] == 0) {
        return -1;
    }
    const char *rel = guest;
    const fs_mnt *m = NULL;
    const char *colon = strchr(guest, ':');
    if (colon != NULL) {
        size_t l = (size_t)(colon - guest);
        if (l == 0 || l >= 16) {
            return -1;
        }
        for (int i = 0; i < g_nmnt; i++) {
            if (strlen(g_mnt[i].name) == l && memcmp(g_mnt[i].name, guest, l) == 0) {
                m = &g_mnt[i];
                break;
            }
        }
        if (m == NULL) {
            return -1;   /* 没这个挂载点 */
        }
        rel = colon + 1;
    }
    if (m == NULL) {
        m = &g_mnt[0];   /* 缺省 rom */
        if (for_write) {
            for (int i = 0; i < g_nmnt; i++) {
                if (g_mnt[i].writable) {
                    m = &g_mnt[i];
                    break;
                }
            }
        }
    }
    /* 逐段规范化:空段/'.'跳过, '..' 立即失败 */
    char relbuf[FS_PATH_MAX];
    size_t o = 0;
    const char *p = rel;
    while (*p != 0) {
        while (*p == '/') {
            p++;
        }
        if (*p == 0) {
            break;
        }
        const char *seg = p;
        while (*p != 0 && *p != '/') {
            p++;
        }
        size_t sl = (size_t)(p - seg);
        if (sl == 1 && seg[0] == '.') {
            continue;
        }
        if (sl == 2 && seg[0] == '.' && seg[1] == '.') {
            return -1;
        }
        if (o + sl + 2 >= sizeof(relbuf)) {
            return -1;
        }
        if (o > 0) {
            relbuf[o++] = '/';
        }
        memcpy(relbuf + o, seg, sl);
        o += sl;
    }
    relbuf[o] = 0;
    int w = snprintf(out, n, "%s%s%s", m->host, o > 0 ? "/" : "", relbuf);
    if (w <= 0 || (size_t)w >= n) {
        return -1;
    }
    return 0;
}

static int fs_writable(const char *guest)
{
    char host[FS_PATH_MAX];
    if (fs_resolve(guest, host, sizeof(host), 1) != 0) {
        return 0;
    }
    /* 反查挂载:按宿主路径前缀判断是否落在可写挂载内 */
    for (int i = 0; i < g_nmnt; i++) {
        size_t l = strlen(g_mnt[i].host);
        if (!g_mnt[i].writable && strncmp(host, g_mnt[i].host, l) == 0) {
            return 0;
        }
    }
    return 1;
}

static Result fs_err(int e)
{
    if (e == ENOENT || e == ENOTDIR) {
        return RES_PATH_NOT_FOUND;
    }
    if (e == EEXIST) {
        return RES_PATH_EXISTS;
    }
    return RES_FAIL;
}

static uint64_t fh_alloc(int fd, int mode)
{
    pthread_mutex_lock(&g_fs_mu);
    for (uint64_t i = 1; i < FS_FH_MAX; i++) {
        if (!g_fh[i].used) {
            g_fh[i].used = 1;
            g_fh[i].fd = fd;
            g_fh[i].mode = mode;
            pthread_mutex_unlock(&g_fs_mu);
            return i;
        }
    }
    pthread_mutex_unlock(&g_fs_mu);
    return 0;
}

static fs_fh *fh_get(uint64_t h)
{
    if (h == 0 || h >= FS_FH_MAX || !g_fh[h].used) {
        return NULL;
    }
    return &g_fh[h];
}

static int rm_rf(const char *path)
{
    struct stat st;
    if (lstat(path, &st) != 0) {
        return -1;
    }
    if (!S_ISDIR(st.st_mode)) {
        return unlink(path);
    }
    DIR *d = opendir(path);
    if (d == NULL) {
        return -1;
    }
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }
        char child[FS_PATH_MAX];
        snprintf(child, sizeof(child), "%s/%s", path, de->d_name);
        rm_rf(child);
    }
    closedir(d);
    return rmdir(path);
}

/* ---------- 强制接管(24 条) ---------- */

HLE_FORCE("_ZN2nn2fs22QueryMountRomCacheSizeEPm", "fsp-srv;rom 直读宿主目录,缓存只给最小值",
          Result, h_fs_QueryRomCache, size_t *out)
{
    if (out != NULL) {
        *out = 0x1000;
    }
    return RES_OK;
}

HLE_FORCE("_ZN2nn2fs8MountRomEPKcPvm", "fsp-srv;rom → 宿主 RomFS 目录",
          Result, h_fs_MountRom, const char *name, void *cache, size_t sz)
{
    (void)cache;
    (void)sz;
    dmFsInit();
    mount_set(name, g_romfs, 0);
    DMLOG_I("fs: MountRom %{public}s → %{public}s", name ? name : "?", g_romfs);
    return RES_OK;
}

HLE_FORCE("_ZN2nn2fs21MountTemporaryStorageEPKc", "fsp-srv;临时区 → 宿主目录",
          Result, h_fs_MountTemp, const char *name)
{
    dmFsInit();
    mkdir_p(g_temp);
    mount_set(name, g_temp, 1);
    DMLOG_I("fs: MountTemporaryStorage %{public}s → %{public}s", name ? name : "?", g_temp);
    return RES_OK;
}

HLE_FORCE("_ZN2nn2fs14EnsureSaveDataERKNS_7account3UidE", "fsp-srv;存档目录在挂载时建",
          Result, h_fs_EnsureSave, const void *uid)
{
    (void)uid;
    return RES_OK;
}

HLE_FORCE("_ZN2nn2fs13MountSaveDataEPKcRKNS_7account3UidE", "fsp-srv;存档 → 宿主目录(单用户)",
          Result, h_fs_MountSave, const char *name, const void *uid)
{
    (void)uid;
    dmFsInit();
    mkdir_p(g_save);
    mount_set(name, g_save, 1);
    DMLOG_I("fs: MountSaveData %{public}s → %{public}s", name ? name : "?", g_save);
    return RES_OK;
}

HLE_FORCE("_ZN2nn2fs6CommitEPKc", "fsp-srv;事务存档 P0 版 = fsync + 成功(临时/只读挂载直接成功)",
          Result, h_fs_Commit, const char *name)
{
    dmFsInit();
    int dfd = open(g_save, O_RDONLY | O_DIRECTORY);
    if (dfd >= 0) {
        fsync(dfd);
        close(dfd);
    }
    DMLOG_I("fs: Commit %{public}s", name ? name : "?");
    return RES_OK;
}

HLE_FORCE("_ZN2nn2fs7UnmountEPKc", "fsp-srv;挂载表常驻,卸载为空操作",
          void, h_fs_Unmount, const char *name) { (void)name; }

HLE_FORCE("_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci", "fsp-srv → open(2)",
          Result, h_fs_OpenFile, uint64_t *out, const char *path, int mode)
{
    if (out == NULL || path == NULL) {
        return RES_FAIL;
    }
    *out = 0;
    int write = mode & (2 | 4);
    if (write && !fs_writable(path)) {
        DMLOG_W("fs: 写只读挂载 %{public}s", path);
        return RES_FAIL;
    }
    char host[FS_PATH_MAX];
    if (fs_resolve(path, host, sizeof(host), write) != 0) {
        return RES_PATH_NOT_FOUND;
    }
    int flags = (mode & 2) ? ((mode & 4) ? O_RDWR | O_APPEND : O_RDWR)
                           : ((mode & 4) ? O_WRONLY | O_APPEND : O_RDONLY);
    int fd = open(host, flags | O_CLOEXEC);
    if (fd < 0) {
        return fs_err(errno);
    }
    uint64_t h = fh_alloc(fd, mode);
    if (h == 0) {
        close(fd);
        return RES_FAIL;
    }
    *out = h;
    return RES_OK;
}

HLE_FORCE("_ZN2nn2fs8ReadFileEPmNS0_10FileHandleElPvm", "fsp-srv → pread(2)",
          Result, h_fs_ReadFile, size_t *out, uint64_t h, int64_t off, void *buf, size_t sz)
{
    if (out != NULL) {
        *out = 0;
    }
    fs_fh *f = fh_get(h);
    if (f == NULL || buf == NULL) {
        return RES_FAIL;
    }
    nvn_dirty_prepare_write(buf, sz);
    size_t got = 0;
    while (got < sz) {
        ssize_t r = pread(f->fd, (uint8_t *)buf + got, sz - got, (off_t)(off + (int64_t)got));
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return fs_err(errno);
        }
        if (r == 0) {
            break;
        }
        got += (size_t)r;
    }
    if (out != NULL) {
        *out = got;
    }
    return RES_OK;
}

HLE_FORCE("_ZN2nn2fs9WriteFileENS0_10FileHandleElPKvmRKNS0_11WriteOptionE", "fsp-srv → pwrite(2)",
          Result, h_fs_WriteFile, uint64_t h, int64_t off, const void *buf, size_t sz,
          const void *opt)
{
    (void)opt;
    fs_fh *f = fh_get(h);
    if (f == NULL || buf == NULL) {
        return RES_FAIL;
    }
    size_t put = 0;
    while (put < sz) {
        ssize_t r = pwrite(f->fd, (const uint8_t *)buf + put, sz - put,
                           (off_t)(off + (int64_t)put));
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return fs_err(errno);
        }
        if (r == 0) {
            return RES_FAIL;
        }
        put += (size_t)r;
    }
    return RES_OK;
}

HLE_FORCE("_ZN2nn2fs11GetFileSizeEPlNS0_10FileHandleE", "fsp-srv → fstat(2)",
          Result, h_fs_GetFileSize, int64_t *out, uint64_t h)
{
    fs_fh *f = fh_get(h);
    struct stat st;
    if (f == NULL || out == NULL || fstat(f->fd, &st) != 0) {
        return RES_FAIL;
    }
    *out = (int64_t)st.st_size;
    return RES_OK;
}

HLE_FORCE("_ZN2nn2fs11SetFileSizeENS0_10FileHandleEl", "fsp-srv → ftruncate(2)",
          Result, h_fs_SetFileSize, uint64_t h, int64_t sz)
{
    fs_fh *f = fh_get(h);
    if (f == NULL) {
        return RES_FAIL;
    }
    return ftruncate(f->fd, (off_t)sz) == 0 ? RES_OK : fs_err(errno);
}

HLE_FORCE("_ZN2nn2fs9FlushFileENS0_10FileHandleE", "fsp-srv;写已进页缓存,刷新为空操作",
          Result, h_fs_FlushFile, uint64_t h) { (void)h; return RES_OK; }

HLE_FORCE("_ZN2nn2fs9CloseFileENS0_10FileHandleE", "fsp-srv → close(2)",
          void, h_fs_CloseFile, uint64_t h)
{
    pthread_mutex_lock(&g_fs_mu);
    if (h != 0 && h < FS_FH_MAX && g_fh[h].used) {
        g_fh[h].used = 0;
        close(g_fh[h].fd);
        g_fh[h].fd = -1;
    }
    pthread_mutex_unlock(&g_fs_mu);
}

HLE_FORCE("_ZN2nn2fs15GetFileOpenModeENS0_10FileHandleE", "fsp-srv;打开方式由宿主记录",
          int, h_fs_GetFileOpenMode, uint64_t h)
{
    fs_fh *f = fh_get(h);
    return f != NULL ? f->mode : 0;
}

HLE_FORCE("_ZN2nn2fs10CreateFileEPKcl", "fsp-srv → open(O_CREAT|O_EXCL)+ftruncate",
          Result, h_fs_CreateFile, const char *path, int64_t sz)
{
    if (path == NULL || !fs_writable(path)) {
        return RES_FAIL;
    }
    char host[FS_PATH_MAX];
    if (fs_resolve(path, host, sizeof(host), 1) != 0) {
        return RES_PATH_NOT_FOUND;
    }
    int fd = open(host, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
    if (fd < 0) {
        return fs_err(errno);
    }
    if (sz > 0 && ftruncate(fd, (off_t)sz) != 0) {
        int e = errno;
        close(fd);
        unlink(host);
        return fs_err(e);
    }
    close(fd);
    return RES_OK;
}

HLE_FORCE("_ZN2nn2fs10DeleteFileEPKc", "fsp-srv → unlink(2)",
          Result, h_fs_DeleteFile, const char *path)
{
    if (path == NULL || !fs_writable(path)) {
        return RES_FAIL;
    }
    char host[FS_PATH_MAX];
    if (fs_resolve(path, host, sizeof(host), 1) != 0) {
        return RES_PATH_NOT_FOUND;
    }
    return unlink(host) == 0 ? RES_OK : fs_err(errno);
}

HLE_FORCE("_ZN2nn2fs15CreateDirectoryEPKc", "fsp-srv → mkdir(2)",
          Result, h_fs_CreateDirectory, const char *path)
{
    if (path == NULL || !fs_writable(path)) {
        return RES_FAIL;
    }
    char host[FS_PATH_MAX];
    if (fs_resolve(path, host, sizeof(host), 1) != 0) {
        return RES_PATH_NOT_FOUND;
    }
    return mkdir(host, 0755) == 0 ? RES_OK : fs_err(errno);
}

HLE_FORCE("_ZN2nn2fs26DeleteDirectoryRecursivelyEPKc", "fsp-srv → 递归删除(仅可写挂载)",
          Result, h_fs_DeleteDirRec, const char *path)
{
    if (path == NULL || !fs_writable(path)) {
        return RES_FAIL;
    }
    char host[FS_PATH_MAX];
    if (fs_resolve(path, host, sizeof(host), 1) != 0) {
        return RES_PATH_NOT_FOUND;
    }
    return rm_rf(host) == 0 ? RES_OK : fs_err(errno);
}

HLE_FORCE("_ZN2nn2fs10RenameFileEPKcS2_", "fsp-srv → rename(2)",
          Result, h_fs_RenameFile, const char *from, const char *to)
{
    if (from == NULL || to == NULL || !fs_writable(to)) {
        return RES_FAIL;
    }
    char hf[FS_PATH_MAX], ht[FS_PATH_MAX];
    if (fs_resolve(from, hf, sizeof(hf), 1) != 0 ||
        fs_resolve(to, ht, sizeof(ht), 1) != 0) {
        return RES_PATH_NOT_FOUND;
    }
    struct stat st;
    if (stat(ht, &st) == 0) {
        return RES_PATH_EXISTS;
    }
    return rename(hf, ht) == 0 ? RES_OK : fs_err(errno);
}

HLE_FORCE("_ZN2nn2fs12GetEntryTypeEPNS0_18DirectoryEntryTypeEPKc", "fsp-srv → stat(2)",
          Result, h_fs_GetEntryType, int32_t *out, const char *path)
{
    if (out == NULL || path == NULL) {
        return RES_FAIL;
    }
    char host[FS_PATH_MAX];
    if (fs_resolve(path, host, sizeof(host), 0) != 0) {
        return RES_PATH_NOT_FOUND;
    }
    struct stat st;
    if (stat(host, &st) != 0) {
        return fs_err(errno);
    }
    *out = S_ISDIR(st.st_mode) ? 0 : 1;
    return RES_OK;
}

HLE_FORCE("_ZN2nn2fs13OpenDirectoryEPNS0_15DirectoryHandleEPKci", "fsp-srv → opendir(3)",
          Result, h_fs_OpenDirectory, uint64_t *out, const char *path, int mode)
{
    if (out == NULL || path == NULL) {
        return RES_FAIL;
    }
    *out = 0;
    char host[FS_PATH_MAX];
    if (fs_resolve(path, host, sizeof(host), 0) != 0) {
        return RES_PATH_NOT_FOUND;
    }
    DIR *d = opendir(host);
    if (d == NULL) {
        return fs_err(errno);
    }
    pthread_mutex_lock(&g_fs_mu);
    for (uint64_t i = 1; i < FS_DH_MAX; i++) {
        if (!g_dh[i].used) {
            g_dh[i].used = 1;
            g_dh[i].dir = d;
            g_dh[i].mode = mode;
            pthread_mutex_unlock(&g_fs_mu);
            *out = i;
            return RES_OK;
        }
    }
    pthread_mutex_unlock(&g_fs_mu);
    closedir(d);
    return RES_FAIL;
}

HLE_FORCE("_ZN2nn2fs13ReadDirectoryEPlPNS0_14DirectoryEntryENS0_15DirectoryHandleEl",
          "fsp-srv → readdir(3)", Result, h_fs_ReadDirectory, int64_t *count,
          DirectoryEntry *ents, uint64_t hh, int64_t max)
{
    if (count != NULL) {
        *count = 0;
    }
    if (hh == 0 || hh >= FS_DH_MAX || !g_dh[hh].used || ents == NULL || max <= 0) {
        return RES_FAIL;
    }
    int64_t n = 0;
    struct dirent *de;
    while (n < max && (de = readdir(g_dh[hh].dir)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }
        /* 用 fstatat 判类型/大小,避开 DT_UNKNOWN 文件系统 */
        struct stat st;
        int isdir = 0;
        int64_t size = 0;
        int dfd = dirfd(g_dh[hh].dir);
        if (dfd >= 0 && fstatat(dfd, de->d_name, &st, 0) == 0) {
            isdir = S_ISDIR(st.st_mode);
            size = isdir ? 0 : (int64_t)st.st_size;
        }
        if (g_dh[hh].mode == 1 && !isdir) {
            continue;
        }
        if (g_dh[hh].mode == 2 && isdir) {
            continue;
        }
        memset(&ents[n], 0, sizeof(ents[n]));
        snprintf(ents[n].name, sizeof(ents[n].name), "%s", de->d_name);
        ents[n].type = (int8_t)(isdir ? 0 : 1);
        ents[n].size = size;
        n++;
    }
    if (count != NULL) {
        *count = n;
    }
    return RES_OK;
}

HLE_FORCE("_ZN2nn2fs14CloseDirectoryENS0_15DirectoryHandleE", "fsp-srv → closedir(3)",
          void, h_fs_CloseDirectory, uint64_t hh)
{
    pthread_mutex_lock(&g_fs_mu);
    if (hh != 0 && hh < FS_DH_MAX && g_dh[hh].used) {
        g_dh[hh].used = 0;
        closedir(g_dh[hh].dir);
        g_dh[hh].dir = NULL;
    }
    pthread_mutex_unlock(&g_fs_mu);
}

/* ---------- 自检:走同一套 HLE 入口建/写/读/查/列/改名/删 ---------- */
int dmFsSelfTest(char *report, int report_size)
{
    dmFsInit();
    const char *dir = "save:/selftest";
    h_fs_DeleteDirRec(dir);
    if (h_fs_CreateDirectory(dir) != RES_OK) {
        snprintf(report, report_size, "fs: 建目录失败(%s)", strerror(errno));
        return -1;
    }
    const char *file = "save:/selftest/a.txt";
    if (h_fs_CreateFile(file, 0) != RES_OK) {
        snprintf(report, report_size, "fs: 建文件失败(%s)", strerror(errno));
        return -1;
    }
    uint64_t h = 0;
    int ok = 0;
    if (h_fs_OpenFile(&h, file, 2) == RES_OK) {
        const char payload[] = "d2r-hos-fs";
        size_t wr = 0;
        ok = h_fs_WriteFile(h, 0, payload, sizeof(payload) - 1, NULL) == RES_OK;
        int64_t sz = 0;
        ok = ok && h_fs_GetFileSize(&sz, h) == RES_OK && sz == (int64_t)(sizeof(payload) - 1);
        char back[32] = {0};
        size_t rd = 0;
        ok = ok && h_fs_ReadFile(&rd, h, 0, back, sizeof(payload) - 1) == RES_OK &&
             rd == sizeof(payload) - 1 && memcmp(back, payload, rd) == 0;
        h_fs_CloseFile(h);
        (void)wr;
    }
    /* 列目录 */
    uint64_t dh = 0;
    int64_t cnt = 0;
    DirectoryEntry ent;
    int listed = 0;
    if (h_fs_OpenDirectory(&dh, dir, 0) == RES_OK) {
        if (h_fs_ReadDirectory(&cnt, &ent, dh, 1) == RES_OK && cnt == 1) {
            listed = strcmp(ent.name, "a.txt") == 0 && ent.type == 1;
        }
        h_fs_CloseDirectory(dh);
    }
    /* 改名 + 删 */
    int renamed = h_fs_RenameFile(file, "save:/selftest/b.txt") == RES_OK;
    int removed = h_fs_DeleteFile("save:/selftest/b.txt") == RES_OK;
    int rmdir_ok = h_fs_DeleteDirRec(dir) == RES_OK;
    ok = ok && listed && renamed && removed && rmdir_ok;
    snprintf(report, report_size,
             "fs: rom=%s | 写/读/大小=%s 列目录=%s 改名=%s 删除=%s",
             g_romfs, ok ? "OK" : "FAIL",
             listed ? "OK" : "FAIL", renamed ? "OK" : "FAIL",
             (removed && rmdir_ok) ? "OK" : "FAIL");
    return ok ? 0 : -1;
}
