#include "loader/dm_guest_loader.h"

#include <stdio.h>
#include <string.h>
#include <hilog/log.h>

#define DM_LOG_TAG "D2RMobileHOS"
#define DMLOG_I(...) OH_LOG_INFO(LOG_APP, __VA_ARGS__)
#define DMLOG_E(...) OH_LOG_ERROR(LOG_APP, __VA_ARGS__)

static uint64_t align_up(uint64_t v, uint64_t a)
{
    return (v + a - 1) & ~(a - 1);
}

static int parse_meta(const uint8_t *image, const DmGuestMeta *meta, const char *name,
                      DmGuestModule *out)
{
    if (meta->magic != DM_GUEST_MAGIC) {
        DMLOG_E("guest meta bad magic 0x%{public}x", meta->magic);
        return -1;
    }
    if (meta->version != DM_GUEST_VERSION) {
        DMLOG_E("unsupported guest container v%{public}u", meta->version);
        return -1;
    }
    uint64_t want = align_up(meta->image_size, DM_GUEST_PAGE);
    if ((const uint8_t *)meta != image + want) {
        DMLOG_E("meta not at image+align(image_size) (+%{public}llu want +%{public}llu)",
                (unsigned long long)((const uint8_t *)meta - image),
                (unsigned long long)want);
        return -1;
    }
    memset(out, 0, sizeof(*out));
    out->hdr = *meta;
    out->image = image;
    out->gates = (const DmGuestGate *)(meta + 1);
    strncpy(out->name, name ? name : "?", sizeof(out->name) - 1);
    return 0;
}

int dmGuestLoad(const void *image, const void *meta, const char *name, DmGuestModule *out)
{
    return parse_meta((const uint8_t *)image, (const DmGuestMeta *)meta, name, out);
}

int dmGuestLoadEmbedded(const void *blob, uint64_t blob_size, const char *name,
                        DmGuestModule *out)
{
    /* 仅占位容器(镜像 16B 补齐到一页,元数据恰在 +PAGE) */
    (void)blob_size;
    return parse_meta((const uint8_t *)blob,
                      (const DmGuestMeta *)((const uint8_t *)blob + DM_GUEST_PAGE), name, out);
}

int dmGuestVerify(const DmGuestModule *m)
{
    if (m == NULL || m->image == NULL || m->hdr.magic != DM_GUEST_MAGIC) {
        return -1;
    }
    const DmGuestMeta *h = &m->hdr;
    if (h->entry_off >= h->image_size) {
        return -1;
    }
    if (h->patch_off > align_up(h->image_size, DM_GUEST_PAGE) ||
            h->patch_off % DM_GUEST_PAGE || h->text_end > h->patch_off ||
            /* ro_off 是 text_end 向上取整到页的边界(NSO 段尾不落页) */
            h->ro_off > align_up(h->text_end, DM_GUEST_PAGE) || h->ro_off % 0x1000 ||
            h->data_off < h->ro_off || h->data_off >= h->image_size) {
        DMLOG_E("layout sanity failed (patch=%{public}llu text_end=%{public}llu "
                "ro=%{public}llu data=%{public}llu image=%{public}llu)",
                (unsigned long long)h->patch_off, (unsigned long long)h->text_end,
                (unsigned long long)h->ro_off, (unsigned long long)h->data_off,
                (unsigned long long)h->image_size);
        return -1;
    }
    for (uint32_t i = 0; i < h->gate_count; i++) {
        const DmGuestGate *g = &m->gates[i];
        if (g->ret_off >= h->image_size || g->orig_off >= h->image_size) {
            DMLOG_E("gate %{public}u out of range (ret=%{public}llu orig=%{public}llu image=%{public}llu)",
                    i, (unsigned long long)g->ret_off,
                    (unsigned long long)g->orig_off,
                    (unsigned long long)h->image_size);
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
