/*
 * hle_audio_srv.c — nn::audio 服务入口的 HLE(先把音频服务流程剪短)。
 *
 * 真机实测:纹理构建修好后,引擎走到 nn::audio::ListAudioOuts →
 * CreateAudioOutManagerByHipc → 连 "sm:" → 宿主无服务 → diag::Abort。
 * 这里把游戏直接导入的音频服务入口改成返回失败(Result != 0),
 * 让引擎走「无音频设备」降级路径,先继续把图形初始化跑完;
 * 真正的出声后续用 audio/dm_audio.c 的 OHAudio 渲染器补。
 */
#include "host/dm.h"

typedef uint32_t Result;
#define AUD_FAIL 1u

/* 列表/打开:宿主没有音频服务,统一失败 */
HLE_FORCE("_ZN2nn5audio13ListAudioOutsEPNS0_12AudioOutInfoEi", "无音频服务(剪):返回失败",
          Result, h_aud_list_out, void *infos, int count)
{
    (void)infos; (void)count;
    return AUD_FAIL;
}

HLE_FORCE("_ZN2nn5audio12ListAudioInsEPNS0_11AudioInInfoEi", "无音频服务(剪):返回失败",
          Result, h_aud_list_in, void *infos, int count)
{
    (void)infos; (void)count;
    return AUD_FAIL;
}

HLE_FORCE("_ZN2nn5audio19ListAudioDeviceNameEPNS0_15AudioDeviceNameEi", "无音频服务(剪):返回失败",
          Result, h_aud_list_dev, void *names, int count)
{
    (void)names; (void)count;
    return AUD_FAIL;
}

HLE_FORCE("_ZN2nn5audio12OpenAudioOutEPNS0_8AudioOutEPKcRKNS0_17AudioOutParameterE", "无音频服务(剪)",
          Result, h_aud_open_out, void *out, const char *name, const void *param)
{
    (void)out; (void)name; (void)param;
    return AUD_FAIL;
}

HLE_FORCE("_ZN2nn5audio12OpenAudioOutEPNS0_8AudioOutEPNS_2os11SystemEventEPKcRKNS0_17AudioOutParameterE", "无音频服务(剪)",
          Result, h_aud_open_out_ev, void *out, void *ev, const char *name, const void *param)
{
    (void)out; (void)ev; (void)name; (void)param;
    return AUD_FAIL;
}

HLE_FORCE("_ZN2nn5audio11OpenAudioInEPNS0_7AudioInEPKcRKNS0_16AudioInParameterE", "无音频服务(剪)",
          Result, h_aud_open_in, void *in, const char *name, const void *param)
{
    (void)in; (void)name; (void)param;
    return AUD_FAIL;
}

HLE_FORCE("_ZN2nn5audio11OpenAudioInEPNS0_7AudioInEPNS_2os11SystemEventEPKcRKNS0_16AudioInParameterE", "无音频服务(剪)",
          Result, h_aud_open_in_ev, void *in, void *ev, const char *name, const void *param)
{
    (void)in; (void)ev; (void)name; (void)param;
    return AUD_FAIL;
}

HLE_FORCE("_ZN2nn5audio26CreateAudioRendererManagerEb", "无音频服务(剪)",
          Result, h_aud_renderer_mgr, int flag)
{
    (void)flag;
    return AUD_FAIL;
}
