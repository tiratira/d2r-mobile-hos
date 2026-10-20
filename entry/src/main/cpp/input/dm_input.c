#include "dm_input.h"

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static DmTouchEvent g_touch_ring[DM_INPUT_RING];
static int g_touch_head, g_touch_tail;
static pthread_mutex_t g_touch_mu = PTHREAD_MUTEX_INITIALIZER;

static DmSkillEvent g_skill_ring[DM_SKILL_RING];
static int g_skill_head, g_skill_tail;
static pthread_mutex_t g_skill_mu = PTHREAD_MUTEX_INITIALIZER;

static float g_stick_dx, g_stick_dy;
static pthread_mutex_t g_stick_mu = PTHREAD_MUTEX_INITIALIZER;

static uint64_t now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
}

void dmInputInit(void)
{
    pthread_mutex_lock(&g_touch_mu);
    g_touch_head = g_touch_tail = 0;
    pthread_mutex_unlock(&g_touch_mu);
    pthread_mutex_lock(&g_skill_mu);
    g_skill_head = g_skill_tail = 0;
    pthread_mutex_unlock(&g_skill_mu);
    pthread_mutex_lock(&g_stick_mu);
    g_stick_dx = g_stick_dy = 0.0f;
    pthread_mutex_unlock(&g_stick_mu);
}

void dmInputPushTouch(int32_t pointer_id, float x, float y, DmTouchPhase phase)
{
    pthread_mutex_lock(&g_touch_mu);
    int next = (g_touch_head + 1) % DM_INPUT_RING;
    if (next == g_touch_tail) { /* 满:丢最旧 */
        g_touch_tail = (g_touch_tail + 1) % DM_INPUT_RING;
    }
    g_touch_ring[g_touch_head].pointer_id = pointer_id;
    g_touch_ring[g_touch_head].x = x;
    g_touch_ring[g_touch_head].y = y;
    g_touch_ring[g_touch_head].phase = phase;
    g_touch_ring[g_touch_head].t_us = now_us();
    g_touch_head = next;
    pthread_mutex_unlock(&g_touch_mu);
}

int dmInputPollTouch(DmTouchEvent *out, int max)
{
    int n = 0;
    pthread_mutex_lock(&g_touch_mu);
    while (n < max && g_touch_tail != g_touch_head) {
        out[n++] = g_touch_ring[g_touch_tail];
        g_touch_tail = (g_touch_tail + 1) % DM_INPUT_RING;
    }
    pthread_mutex_unlock(&g_touch_mu);
    return n;
}

void dmInputSetStick(float dx, float dy)
{
    pthread_mutex_lock(&g_stick_mu);
    g_stick_dx = dx;
    g_stick_dy = dy;
    pthread_mutex_unlock(&g_stick_mu);
}

void dmInputGetStick(float *dx, float *dy)
{
    pthread_mutex_lock(&g_stick_mu);
    if (dx) *dx = g_stick_dx;
    if (dy) *dy = g_stick_dy;
    pthread_mutex_unlock(&g_stick_mu);
}

void dmInputCastSkill(int32_t slot, float aim_dx, float aim_dy, int32_t hold)
{
    pthread_mutex_lock(&g_skill_mu);
    int next = (g_skill_head + 1) % DM_SKILL_RING;
    if (next == g_skill_tail) {
        g_skill_tail = (g_skill_tail + 1) % DM_SKILL_RING;
    }
    g_skill_ring[g_skill_head].slot = slot;
    g_skill_ring[g_skill_head].aim_dx = aim_dx;
    g_skill_ring[g_skill_head].aim_dy = aim_dy;
    g_skill_ring[g_skill_head].hold = hold;
    g_skill_head = next;
    pthread_mutex_unlock(&g_skill_mu);
}

int dmInputPollSkills(DmSkillEvent *out, int max)
{
    int n = 0;
    pthread_mutex_lock(&g_skill_mu);
    while (n < max && g_skill_tail != g_skill_head) {
        out[n++] = g_skill_ring[g_skill_tail];
        g_skill_tail = (g_skill_tail + 1) % DM_SKILL_RING;
    }
    pthread_mutex_unlock(&g_skill_mu);
    return n;
}
