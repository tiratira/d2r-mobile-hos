#include "loader/dm_guest_loader.h"

#include <stdio.h>
#include <string.h>
#include <hilog/log.h>

#define DM_LOG_TAG "D2RMobileHOS"
#define DMLOG_I(...) OH_LOG_INFO(LOG_APP, __VA_ARGS__)
#define DMLOG_E(...) OH_LOG_ERROR(LOG_APP, __VA_ARGS__)

#define HDR_SIZE ((uint64_t)sizeof(DmGuestImageHeader))

static int parse_module(const uint8_t *blob, uint64_t blob_size, const char *name,
                        DmGuestModule *out)
{
    if (blob_size < HDR_SIZE && blob_size != UINT64_MAX) {
        DMLOG_E("guest blob too small: %{public}llu", (unsigned long long)blob_size);
        return -1;
    }
    DmGuestImageHeader h;
    memcpy(&h, blob, sizeof(h));
    if (h.magic != DM_GUEST_MAGIC) {
        DMLOG_E("bad guest magic 0x%{public}x", h.magic);
        return -1;
    }
    if (h.version != DM_GUEST_VERSION) {
        DMLOG_E("unsupported guest container v%{public}u", h.version);
        return -1;
    }
    memset(out, 0, sizeof(*out));
    out->hdr = h;
    out->blob = blob;
    out->image = blob + HDR_SIZE;
    out->gates = (const DmGuestGate *)(blob + HDR_SIZE + h.image_size);
    strncpy(out->name, name ? name : "?", sizeof(out->name) - 1);
    return 0;
}

int dmGuestLoadEmbedded(const void *blob, uint64_t blob_size, const char *name,
                        DmGuestModule *out)
{
    /* 内嵌容器随 .so 签名,仅登记;blob_size 未知时传 UINT64_MAX。 */
    return parse_module((const uint8_t *)blob, blob_size, name, out);
}

int dmGuestVerify(const DmGuestModule *m)
{
    if (m == NULL || m->image == NULL || m->hdr.magic != DM_GUEST_MAGIC) {
        return -1;
    }
    if (m->hdr.entry_offset >= m->hdr.image_size) {
        return -1;
    }
    for (uint32_t i = 0; i < m->hdr.gate_count; i++) {
        const DmGuestGate *g = &m->gates[i];
        if (g->ret_off >= m->hdr.image_size || g->orig_off >= m->hdr.image_size) {
            DMLOG_E("gate %{public}u out of range (ret=%{public}llu orig=%{public}llu image=%{public}llu)",
                    i, (unsigned long long)g->ret_off,
                    (unsigned long long)g->orig_off,
                    (unsigned long long)m->hdr.image_size);
            return -1;
        }
        if (g->kind > DM_GATE_PARAM) {
            DMLOG_E("gate %{public}u unknown kind %{public}u", i, g->kind);
            return -1;
        }
    }
    return 0;
}

int dmGuestGateAt(const DmGuestModule *m, uint32_t index, DmGuestGate *out)
{
    if (m == NULL || index >= m->hdr.gate_count) {
        return -1;
    }
    *out = m->gates[index];
    return 0;
}

void dmGuestUnload(DmGuestModule *m)
{
    if (m) {
        m->image = NULL;
        m->gates = NULL;
    }
}

int dmGuestSelfTest(char *report, int report_size)
{
    DmGuestModule m;
    if (dmGuestLoadEmbedded(__dm_guest_placeholder_start, UINT64_MAX, "placeholder", &m) != 0) {
        snprintf(report, report_size, "embedded guest: load FAILED");
        return -1;
    }
    if (dmGuestVerify(&m) != 0) {
        snprintf(report, report_size, "embedded guest: verify FAILED");
        return -1;
    }
    DmGuestGate g0;
    int have_gate = dmGuestGateAt(&m, 0, &g0) == 0;
    snprintf(report, report_size,
             "embedded guest: OK (kind=%u image=%lluB gates=%u g0=%s ret=%llu)",
             m.hdr.module_kind, (unsigned long long)m.hdr.image_size, m.hdr.gate_count,
             have_gate ? "yes" : "no", have_gate ? (unsigned long long)g0.ret_off : 0ull);
    dmGuestUnload(&m);
    return 0;
}
