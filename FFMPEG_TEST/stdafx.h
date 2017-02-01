#pragma once
extern "C"
{
#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavfilter\avfilter.h>		// Main libfilter
#include <libavfilter\buffersink.h>		// 필터링이 된 프레임이 들어가는 싱크 필터
#include <libavfilter\buffersrc.h>		// 원본 프레임이 들어가는 소스 필터

#endif
}