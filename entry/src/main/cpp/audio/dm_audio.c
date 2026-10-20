/*
 * dm_audio.c — OHAudio 后端(上游 CoreAudio 的 OHOS 对应物)。
 *
 * 上游 dm/host/hle_audio.c 把 nn::audio 服务接成 CoreAudio 播放队列;HOS
 * 这里先落「最小可用播放通路」:OHAudio 48 kHz / 2ch / S16LE 渲染器 +
 * 写数据回调,先播一段正弦自检音,证明音频硬件与权限链路可用。游戏真正的
 * nn::audio HLE 后续按同一渲染器投递 PCM(nn::audio 的 AudioOutBuffer 语义
 * 与 OHAudio 回调同构:填满一块、交给系统、拿回已播完的块)。
 *
 * API 依据(均在 NDK sysroot 核实):
 *   ohaudio/native_audiostreambuilder.h / native_audiorenderer.h
 *   OH_AudioRenderer_OnWriteDataCallback(audioDataSize 为字节;须整帧填满
 *   填满后返回 AUDIO_DATA_CALLBACK_RESULT_VALID)。
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include "host/dm.h"

#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

#include <ohaudio/native_audiostream_base.h>
#include <ohaudio/native_audiorenderer.h>
#include <ohaudio/native_audiostreambuilder.h>

#define DM_AUDIO_RATE 48000
#define DM_AUDIO_CH 2
#define DM_AUDIO_TONE_HZ 440.0
#define DM_AUDIO_PI 3.14159265358979323846

static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static OH_AudioRenderer *g_renderer;
static double g_phase;
static volatile int g_playing;
static volatile int g_suspended;
static char g_audio_report[192] = "audio: 未初始化";

/* ---------- OHAudio 写数据回调(自有线程,不可阻塞/不可 Stop) ---------- */
static OH_AudioData_Callback_Result on_write_data(OH_AudioRenderer *renderer, void *user_data,
                                                  void *audio_data, int32_t audio_data_size)
{
    (void)renderer;
    (void)user_data;
    const int32_t frame_bytes = DM_AUDIO_CH * (int32_t)sizeof(int16_t);
    if (audio_data_size <= 0 || audio_data_size % frame_bytes != 0) {
        return AUDIO_DATA_CALLBACK_RESULT_INVALID;
    }
    if (g_suspended) {
        memset(audio_data, 0, (size_t)audio_data_size);
        return AUDIO_DATA_CALLBACK_RESULT_VALID;
    }
    int16_t *out = (int16_t *)audio_data;
    const int32_t samples = audio_data_size / (int32_t)sizeof(int16_t);
    const double step = 2.0 * DM_AUDIO_PI * DM_AUDIO_TONE_HZ / (double)DM_AUDIO_RATE;
    for (int32_t i = 0; i < samples; i += DM_AUDIO_CH) {
        int16_t v = (int16_t)(sin(g_phase) * 0.20 * 32767.0);
        out[i] = v;
        if (i + 1 < samples) {
            out[i + 1] = v;
        }
        g_phase += step;
        if (g_phase >= 2.0 * DM_AUDIO_PI) {
            g_phase -= 2.0 * DM_AUDIO_PI;
        }
    }
    return AUDIO_DATA_CALLBACK_RESULT_VALID;
}

/* 建渲染器;成功返回 0。调用方负责 Release/Destroy。 */
static int audio_open(OH_AudioStreamBuilder **out_builder, OH_AudioRenderer **out_renderer)
{
    OH_AudioStreamBuilder *builder = NULL;
    OH_AudioRenderer *renderer = NULL;
    if (OH_AudioStreamBuilder_Create(&builder, AUDIOSTREAM_TYPE_RENDERER) != AUDIOSTREAM_SUCCESS) {
        return -1;
    }
    OH_AudioStream_Result r = OH_AudioStreamBuilder_SetSamplingRate(builder, DM_AUDIO_RATE);
    if (r == AUDIOSTREAM_SUCCESS) {
        r = OH_AudioStreamBuilder_SetChannelCount(builder, DM_AUDIO_CH);
    }
    if (r == AUDIOSTREAM_SUCCESS) {
        r = OH_AudioStreamBuilder_SetSampleFormat(builder, AUDIOSTREAM_SAMPLE_S16LE);
    }
    if (r == AUDIOSTREAM_SUCCESS) {
        r = OH_AudioStreamBuilder_SetEncodingType(builder, AUDIOSTREAM_ENCODING_TYPE_RAW);
    }
    if (r == AUDIOSTREAM_SUCCESS) {
        r = OH_AudioStreamBuilder_SetRendererInfo(builder, AUDIOSTREAM_USAGE_MUSIC);
    }
    if (r == AUDIOSTREAM_SUCCESS) {
        r = OH_AudioStreamBuilder_SetRendererWriteDataCallback(builder, on_write_data, NULL);
    }
    if (r == AUDIOSTREAM_SUCCESS) {
        r = OH_AudioStreamBuilder_GenerateRenderer(builder, &renderer);
    }
    if (r != AUDIOSTREAM_SUCCESS) {
        DMLOG_E("audio: 建渲染器失败(%{public}d)", (int)r);
        if (renderer) {
            OH_AudioRenderer_Release(renderer);
        }
        OH_AudioStreamBuilder_Destroy(builder);
        return -1;
    }
    *out_builder = builder;
    *out_renderer = renderer;
    return 0;
}

static void audio_close(OH_AudioStreamBuilder *builder, OH_AudioRenderer *renderer)
{
    if (renderer != NULL) {
        OH_AudioRenderer_Stop(renderer);
        OH_AudioRenderer_Release(renderer);
    }
    if (builder != NULL) {
        OH_AudioStreamBuilder_Destroy(builder);
    }
}

int dmAudioSelfTest(char *report, int report_size)
{
    OH_AudioStreamBuilder *builder = NULL;
    OH_AudioRenderer *renderer = NULL;
    int rc = audio_open(&builder, &renderer);
    if (rc == 0) {
        audio_close(builder, renderer);
    }
    snprintf(g_audio_report, sizeof(g_audio_report),
             "ohaudio: 48kHz/2ch/S16LE 渲染器%s", rc == 0 ? "创建 OK" : "创建 FAILED");
    snprintf(report, report_size, "%s", g_audio_report);
    return rc;
}

struct tone_arg {
    int ms;
};

static void *tone_thread(void *p)
{
    struct tone_arg *a = (struct tone_arg *)p;
    OH_AudioStreamBuilder *builder = NULL;
    OH_AudioRenderer *renderer = NULL;
    if (audio_open(&builder, &renderer) != 0) {
        snprintf(g_audio_report, sizeof(g_audio_report), "ohaudio: 正弦自检建流失败");
        free(a);
        g_playing = 0;
        return NULL;
    }
    pthread_mutex_lock(&g_mu);
    g_renderer = renderer;
    g_phase = 0;
    pthread_mutex_unlock(&g_mu);
    if (OH_AudioRenderer_Start(renderer) != AUDIOSTREAM_SUCCESS) {
        snprintf(g_audio_report, sizeof(g_audio_report), "ohaudio: Start 失败");
        audio_close(builder, renderer);
        free(a);
        g_playing = 0;
        return NULL;
    }
    snprintf(g_audio_report, sizeof(g_audio_report), "ohaudio: 440Hz 正弦 %dms 播放中", a->ms);
    usleep((useconds_t)(a->ms > 0 ? a->ms : 1000) * 1000);
    pthread_mutex_lock(&g_mu);
    g_renderer = NULL;
    pthread_mutex_unlock(&g_mu);
    audio_close(builder, renderer);
    snprintf(g_audio_report, sizeof(g_audio_report), "ohaudio: 正弦 %dms 播放完成", a->ms);
    free(a);
    g_playing = 0;
    return NULL;
}

int dmAudioPlayTone(int ms)
{
    if (g_playing) {
        return -1;
    }
    g_playing = 1;
    struct tone_arg *a = (struct tone_arg *)calloc(1, sizeof(*a));
    if (a == NULL) {
        g_playing = 0;
        return -1;
    }
    a->ms = ms > 0 ? ms : 800;
    pthread_t th;
    if (pthread_create(&th, NULL, tone_thread, a) != 0) {
        free(a);
        g_playing = 0;
        return -1;
    }
    pthread_detach(th);
    return 0;
}

void dmAudioSuspend(int on)
{
    g_suspended = on ? 1 : 0;
}

const char *dmAudioReport(void)
{
    return g_audio_report;
}
