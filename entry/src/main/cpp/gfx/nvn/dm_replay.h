#pragma once
#include "nvn_obj.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint64_t dmReplaySerial(void);
uint64_t dmReplayFrames(void);
void dmReplaySubmit(int n, const uint64_t *handles);
void dmReplayPresent(const tex_t *t, const int crop[4]);
void dmReplayArmCapture(int frames);
void mtl_pool_add(void *p, size_t n);
void mtl_pool_del(void *p);
void mtl_reg_tex(int id, const tex_t *t, const view_t *v);
void mtl_reg_samp(int id, const samp_par *p);
void mtl_set_tex_pool_n(int n);
void mtl_tex_release(tex_t *t);
void mtl_prog_release(prog_t *p);
void mtl_finish(void);
int mtl_wait(uint64_t serial, uint64_t timeout);
void mtl_read_texels(const tex_t *t, unsigned level, unsigned layer, const NVNcopyRegion *r, uint8_t *dst,
                     size_t row, size_t img);
#ifdef __cplusplus
}
#endif
