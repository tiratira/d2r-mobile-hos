#include "dm_guest_loader.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <hilog/log.h>

#define DM_LOG_TAG "D2RMobileHOS"
#define DMLOG_I(...) OH_LOG_INFO(LOG_APP, __VA_ARGS__)
#define DMLOG_E(...) OH_LOG_ERROR(LOG_APP, __VA_ARGS__)

static int parse_module(const uint8_t *blob, uint64_t blob_size, const char *name,
                        DmGuestModule *out)
{
    if (blob_size < sizeof(DmGuestImageHeader)) {
        DMLOG_E("guest blob too small: %{public}llu", (unsigned long long)blob_size);
        return -1;
    }
    DmGuestImageHeader h;
    memcpy(&h, blob, sizeof(h));
    if (h.magic != DM_GUEST_MAGIC) {
        DMLOG_E("bad guest magic 0x%{public}x", h.magic);
        return -1;
    }
    if (h.version != 1) {
        DMLOG_E("unsupported guest container v%{public}u", h.version);
        return -1;
    }
    uint64_t need = sizeof(h) + h.image_size + (uint64_t)h.gate_count * sizeof(DmGuestGate);
    if (need > blob_size) {
        DMLOG_E("guest blob truncated: need %{public}llu have %{public}llu",
                (unsigned long long)need, (unsigned long long)blob_size);
        return -1;
    }
    memset(out, 0, sizeof(*out));
    out->hdr = h;
    out->base = blob;
    out->mapped_size = need;
    out->gates = (const DmGuestGate *)(blob + sizeof(h) + h.image_size);
    strncpy(out->name, name ? name : "?", sizeof(out->name) - 1);
    return 0;
}

int dmGuestLoadEmbedded(const void *blob, const char *name, DmGuestModule *out)
{
    /* 内嵌镜像位于 .so 的可执行段,已随包签名;仅登记,不再复制。 */
    return parse_module((const uint8_t *)blob, UINT64_MAX, name, out);
}

int dmGuestLoad(const char *path, const char *name, DmGuestModule *out)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        DMLOG_E("open %{public}s failed", path);
        return -1;
    }
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size < (off_t)sizeof(DmGuestImageHeader)) {
        close(fd);
        return -1;
    }
    void *p = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (p == MAP_FAILED) {
        DMLOG_E("mmap %{public}s failed", path);
        return -1;
    }
    int rc = parse_module((const uint8_t *)p, (uint64_t)st.st_size, name, out);
    if (rc != 0) {
        munmap(p, (size_t)st.st_size);
    }
    return rc;
}

int dmGuestVerify(const DmGuestModule *m)
{
    if (m == NULL || m->base == NULL || m->hdr.magic != DM_GUEST_MAGIC) {
        return -1;
    }
    if (m->hdr.entry_offset >= m->hdr.image_size) {
        return -1;
    }
    for (uint32_t i = 0; i < m->hdr.gate_count; i++) {
        if (m->gates[i].image_offset >= m->hdr.image_size) {
            DMLOG_E("gate %{public}u out of range", m->gates[i].gate_id);
            return -1;
        }
    }
    return 0;
}

int64_t dmGuestGateOffset(const DmGuestModule *m, uint32_t gate_id)
{
    if (m == NULL) {
        return -1;
    }
    for (uint32_t i = 0; i < m->hdr.gate_count; i++) {
        if (m->gates[i].gate_id == gate_id) {
            return (int64_t)m->gates[i].image_offset;
        }
    }
    return -1;
}

void dmGuestUnload(DmGuestModule *m)
{
    /* 内嵌镜像随 .so 生命周期;文件映射的释放策略见加载来源。 */
    if (m) {
        m->base = NULL;
    }
}

int dmGuestSelfTest(char *report, int report_size)
{
    DmGuestModule m;
    int rc = dmGuestLoadEmbedded(__dm_guest_placeholder_start, "placeholder", &m);
    if (rc != 0) {
        snprintf(report, report_size, "embedded guest: load FAILED");
        return -1;
    }
    rc = dmGuestVerify(&m);
    if (rc != 0) {
        snprintf(report, report_size, "embedded guest: verify FAILED");
        return -1;
    }
    int64_t g0 = dmGuestGateOffset(&m, 0);
    snprintf(report, report_size,
             "embedded guest: OK (kind=%u image=%lluB gates=%u gate0=%lld)",
             m.hdr.module_kind, (unsigned long long)m.hdr.image_size,
             m.hdr.gate_count, (long long)g0);
    dmGuestUnload(&m);
    return 0;
}
