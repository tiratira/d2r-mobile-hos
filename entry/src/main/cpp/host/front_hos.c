#include "host/front_hos.h"
#include "host/dm.h"

static uint32_t g_res[2][2] = {
    {1280, 720},    /* 0 掌机(上游 Mac 缺省) */
    {1600, 900}     /* 1 主机(上游 Mac 缺省) */
};

uint64_t front_game_res(int mode)
{
    if (mode < 0 || mode > 1) {
        mode = 0;
    }
    return (uint64_t)g_res[mode][0] | ((uint64_t)g_res[mode][1] << 32);
}

void front_set_game_res(int mode, uint32_t w, uint32_t h)
{
    if (mode < 0 || mode > 1 || w == 0 || h == 0) {
        return;
    }
    g_res[mode][0] = w;
    g_res[mode][1] = h;
}
