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
#include <libavfilter\buffersink.h>		// ���͸��� �� �������� ���� ��ũ ����
#include <libavfilter\buffersrc.h>		// ���� �������� ���� �ҽ� ����

#endif
}