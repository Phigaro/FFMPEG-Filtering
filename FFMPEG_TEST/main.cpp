#pragma once
///> Include FFMpeg

#define _CRT_SECURE_NO_DEPRECATE
#pragma warning(disable:4996)

#include <iostream>
#include <windows.h>
#include <string>

using namespace std;
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
/////////
struct timeval _start_time, _stop_time, _start_time_2, _stop_time_2;
double _diff_secs = 0, _diff_secs_2 = 0;
#define TIME_THIS( MSG, ... ) \
do { \
    gettimeofday(&_start_time, NULL); \
    __VA_ARGS__ \
    gettimeofday(&_stop_time, NULL); \
    _diff_secs = (double)(_stop_time.tv_usec - _start_time.tv_usec) / 1000000 + (double)(_stop_time.tv_sec - _start_time.tv_sec); \
    DEBUG( "%s timing results: %f seconds", MSG, _diff_secs); \
} while ( 0 );

#define TIME_START \
    gettimeofday(&_start_time_2, NULL);

#define TIME_END( MSG ) \
do { \
    gettimeofday(&_stop_time_2, NULL); \
    _diff_secs_2 = (double)(_stop_time_2.tv_usec - _start_time_2.tv_usec) / 1000000 + (double)(_stop_time_2.tv_sec - _start_time_2.tv_sec); \
    DEBUG( "%s timing results: %f seconds", MSG, _diff_secs_2); \
} while ( 0 );

typedef struct AVContext {
	AVFormatContext *fmt_ctx;
	AVCodecParameters *video_codecpar;
	AVCodecContext * video_dec_ctx;
	AVStream *video_stream;
	int video_stream_idx;
	AVFrame *frame;
	AVPacket pkt;
	enum AVMediaType type;
	AVDictionary *opts;
	AVCodec *dec;
} AVContext;

typedef struct Options {
	char * input_file;
	char * output_file_pattern;
	int32_t * frame_indices;
	double * frame_timestamps;
	int nbframes;
	char* img_size_str;
	int img_width, img_height;
} Options;

////////////////

#define Video_flag 0
#define Audio_flag 1
#define All_flag 2
#define Filter_default_idx 2

///> Library Link On Windows System
#pragma comment( lib, "avformat.lib" )   
#pragma comment( lib, "avutil.lib" )
#pragma comment( lib, "avcodec.lib" )
#pragma comment( lib, "swscale.lib")
#pragma comment( lib, "avfilter.lib")

typedef struct _FilterContext {
	AVFilterGraph	*filter_graph;	// Filter's Context
	AVFilterContext	*src_ctx;		// Linked Buffer Source
	AVFilterContext	*sink_ctx;		// Linked Buffer Sink
	AVFilterInOut	*inputs;		// Filter Input
	AVFilterInOut	*outputs;		// Filter Output
	int last_filter_idx = 0;		// Last filter index for Linking
} FilterContext;

typedef struct _Codec_Set {
	AVCodec *codec = NULL;
	AVCodecContext *V_codec_ctx = NULL;
	AVCodecContext *A_codec_ctx = NULL;
} Codec_Set;

typedef struct _Format_ctx_Set {
	AVInputFormat   *ifmt = NULL;
	AVOutputFormat  *ofmt = NULL;
	AVFormatContext *ifmt_ctx = NULL;
	AVFormatContext *ofmt_ctx = NULL;
	int nVSI = -1;
	int nASI = -1;
} Format_ctx_Set;

AVFrame *pictureFrame;
AVFrame *m_picture_frame;
Format_ctx_Set	*fmt_ctx = NULL;
Format_ctx_Set	*fmt_ctx_audio = NULL;
AVInputFormat   *ifmt = NULL;
AVOutputFormat  *ofmt = NULL;
AVFormatContext *picFormatCtx = NULL;

// Filter Option
int angle = 0;
int dst_width = 80 * 8;
int dst_height = 60 * 8;

//AVCodecID v_codec_id = AV_CODEC_ID_JPEG2000;
AVCodecID v_codec_id = AV_CODEC_ID_H264;
AVCodecID a_codec_id = AV_CODEC_ID_MP3;

const char *szFilePath = "./Test_Video/Test_Video.mp4";
const char *szFilePath_2 = "./Test_Video/Test_Video_2.mp4";
const char *szFilePath_3 = "./Test_Video/Canon.mp3";
const char *outputfile = "./Test_Video/output.mp4";
const char *outputFormat = "mp4";

static FilterContext vfilter_ctx, afilter_ctx;

static AVFormatContext* init_input_format(const char* input_file_path, Format_ctx_Set* fmt_ctx) {

	AVCodec			*codec = NULL;
	AVCodecContext	*c = NULL;
	AVFormatContext	*p_ifmt_ctx = NULL;

	if ((avformat_open_input(&p_ifmt_ctx, input_file_path, 0, 0)) < 0)
	{
		fprintf(stderr, "Could not open input file '%s'", input_file_path);
	}

	if ((avformat_find_stream_info(p_ifmt_ctx, 0)) < 0)
	{
		fprintf(stderr, "Failed to retrieve input stream information");
	}

	av_dump_format(p_ifmt_ctx, 0, input_file_path, 0);

	///> Find Video Stream
	for (int i = 0; i < p_ifmt_ctx->nb_streams; i++)
	{
		if (fmt_ctx->nVSI < 0 && p_ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			fmt_ctx->nVSI = i;
		}
		else if (fmt_ctx->nASI < 0 && p_ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			fmt_ctx->nASI = i;
		}
	}

	if (fmt_ctx->nVSI < 0 && fmt_ctx->nASI < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "No Video & Audio Streams were Found\n");
		return 0;
	}

	return p_ifmt_ctx;
}

// Don't Use
static AVFormatContext* init_output_format_2(AVFormatContext* p_ifmt_ctx, const char* output_file_path, AVCodecContext* enc_codec_ctx) {
	AVOutputFormat	*p_ofmt = NULL;
	AVFormatContext	*p_ofmt_ctx = NULL;

	avformat_alloc_output_context2(&p_ofmt_ctx, NULL, NULL, output_file_path);
	if (!p_ofmt_ctx)
	{
		fprintf(stderr, "Could not create output context\n");
	}

	p_ofmt = p_ofmt_ctx->oformat;

	int ret = 0;
	for (int i = 0; i < p_ifmt_ctx->nb_streams; i++)
	{
		AVStream *in_stream = p_ifmt_ctx->streams[i];
		AVStream *out_stream = avformat_new_stream(p_ofmt_ctx, in_stream->codec->codec);

		if (p_ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			ret = avcodec_copy_context(out_stream->codec, enc_codec_ctx);
			out_stream->codec->codec_tag = 0;//문서
		}
		else if (p_ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
			out_stream->codec->codec_tag = 0;
		}

		if (ret < 0) {
			fprintf(stderr, "Failed to copy context from input to output stream codec context\n");
		}

		if (p_ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		{
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		}
	}

	av_dump_format(p_ofmt_ctx, 0, output_file_path, 1);

	if (!(p_ofmt->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&p_ofmt_ctx->pb, output_file_path, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			fprintf(stderr, "Could not open output file '%s'", output_file_path);
		}
	}

	ret = avformat_write_header(p_ofmt_ctx, NULL);
	if (ret < 0)
	{
		fprintf(stderr, "Error occurred when opening output file\n");
	}
	return p_ofmt_ctx;
}

static AVFormatContext* init_output_format(AVFormatContext* p_ifmt_ctx, const char* output_file_path, AVCodecContext* enc_codec_ctx) {
	AVFormatContext  *p_ofmt_ctx = NULL;

	avformat_alloc_output_context2(&p_ofmt_ctx, NULL, outputFormat, outputfile);
	if (!p_ofmt_ctx)
	{
		fprintf(stderr, "Could not create output context\n");
	}

	return p_ofmt_ctx;
}

static AVFormatContext* add_stream_output_format(Format_ctx_Set* in_fmt_ctx, Format_ctx_Set* out_fmt_ctx, AVCodecContext* enc_codec_ctx, int flag) {
	int ret = 0;
	AVStream *in_stream = NULL;
	AVStream *out_stream = NULL;

	if (flag == Video_flag) {

		in_stream = in_fmt_ctx->ifmt_ctx->streams[in_fmt_ctx->nVSI];
		out_stream = avformat_new_stream(out_fmt_ctx->ofmt_ctx, in_stream->codec->codec);

		if (out_fmt_ctx->ifmt_ctx->streams[out_fmt_ctx->nVSI]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			// copy 주의
			ret = avcodec_copy_context(out_stream->codec, enc_codec_ctx);
			out_stream->codec->codec_tag = 0;//문서
		}
	}

	if (flag == Audio_flag) {
		in_stream = in_fmt_ctx->ifmt_ctx->streams[in_fmt_ctx->nASI];
		out_stream = avformat_new_stream(out_fmt_ctx->ofmt_ctx, in_stream->codec->codec);

		if (out_fmt_ctx->ifmt_ctx->streams[out_fmt_ctx->nASI]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			// copy 주의
			ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
			//ret = avcodec_copy_context(out_stream->codec, enc_codec_ctx);
			out_stream->codec->codec_tag = 0;//문서
		}
	}

	if (flag == All_flag) {
		// 나중에 구현
	}

	if (ret < 0) {
		fprintf(stderr, "Failed to copy context from input to output stream codec context\n");
	}

	if (out_fmt_ctx->ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
	{
		out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	return out_fmt_ctx->ofmt_ctx;
}

static AVFormatContext* write_output_header(_Format_ctx_Set* fmt_ctx, const char* output_file_path) {
	int ret = 0;

	fmt_ctx->ofmt = fmt_ctx->ofmt_ctx->oformat;

	av_dump_format(fmt_ctx->ofmt_ctx, 0, output_file_path, 1);
	if (!(fmt_ctx->ofmt->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&fmt_ctx->ofmt_ctx->pb, output_file_path, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			fprintf(stderr, "Could not open output file '%s'", output_file_path);
		}
	}

	ret = avformat_write_header(fmt_ctx->ofmt_ctx, NULL);
	if (ret < 0)
	{
		fprintf(stderr, "Error occurred when opening output file\n");
	}
	// Write 부분

	return fmt_ctx->ofmt_ctx;
}

static AVCodecContext* init_encoder(AVFormatContext* p_ifmt_ctx, AVCodecID V_Codec_ID, AVCodecID A_Codec_ID, int flag, Format_ctx_Set* fmt_ctx) {

	AVCodec        *codec = NULL;
	AVCodecContext *c_ctx = NULL;

	if (flag == Video_flag) {
		if (fmt_ctx->nVSI != -1) {
			codec = avcodec_find_encoder(V_Codec_ID);
			if (!codec)
			{
				fprintf(stderr, "Codec not found\n");
				return 0;
			}

			c_ctx = avcodec_alloc_context3(codec);
			if (!c_ctx)
			{
				fprintf(stderr, "Could not allocate video codec context\n");
				return 0;
			}

			// Example
			if (V_Codec_ID == AV_CODEC_ID_H264)
				c_ctx->profile = FF_PROFILE_H264_BASELINE;
			c_ctx->width = p_ifmt_ctx->streams[fmt_ctx->nVSI]->codec->width;
			c_ctx->height = p_ifmt_ctx->streams[fmt_ctx->nVSI]->codec->height;
			c_ctx->pix_fmt = p_ifmt_ctx->streams[fmt_ctx->nVSI]->codec->pix_fmt;
		

			//c_ctx->pix_fmt = AV_PIX_FMT_YUV444P;
			c_ctx->time_base = { 1,25 };
			c_ctx->bit_rate = 500 * 1000;


			if (avcodec_open2(c_ctx, codec, NULL) < 0)
			{
				fprintf(stderr, "Could not open codec\n");
				return 0;
			}
		}
	}

	if (flag == Audio_flag) {
		if (fmt_ctx->nASI != -1) {
			codec = avcodec_find_encoder(A_Codec_ID);
			if (!codec)
			{
				fprintf(stderr, "Codec not found\n");
				return 0;
			}

			c_ctx = avcodec_alloc_context3(codec);
			if (!c_ctx)
			{
				fprintf(stderr, "Could not allocate video codec context\n");
				return 0;
			}

			c_ctx->bit_rate = 192 * 1000;
			c_ctx->time_base = { 1,44100 };
			c_ctx->sample_rate = 44100;
			c_ctx->channel_layout = 3;
			c_ctx->channels = 2;
			c_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;


			// Example

			if (avcodec_open2(c_ctx, codec, NULL) < 0)
			{
				fprintf(stderr, "Could not open codec\n");
				return 0;
			}
		}
	}

	return c_ctx;
}

static AVCodecContext* init_decoder(Format_ctx_Set *fmt_ctx, int flag) {
	for (int i = 0; i < fmt_ctx->ifmt_ctx->nb_streams; i++)
	{
		if (fmt_ctx->nVSI < 0 && fmt_ctx->ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			fmt_ctx->nVSI = i;
		}
		else if (fmt_ctx->nASI < 0 && fmt_ctx->ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			fmt_ctx->nASI = i;
		}
	}

	AVCodecContext *pVCtx = fmt_ctx->ifmt_ctx->streams[fmt_ctx->nVSI]->codec;
	//pVCtx->pix_fmt = AV_PIX_FMT_RGB24;
	AVCodecContext *pACtx = fmt_ctx->ifmt_ctx->streams[fmt_ctx->nASI]->codec;

	if (fmt_ctx->nVSI < 0 && fmt_ctx->nASI < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "No Video & Audio Streams were Found\n");
		return 0;
	}

	///> Find Video Decoder
	if (fmt_ctx->nVSI != -1) {
		AVCodec   *pVideoCodec = avcodec_find_decoder(fmt_ctx->ifmt_ctx->streams[fmt_ctx->nVSI]->codec->codec_id);
		


		if (pVideoCodec == NULL)
		{
			av_log(NULL, AV_LOG_ERROR, "No Video Decoder was Found\n");
			return 0;
		}

		///> Initialize Codec Context as Decoder
		if (avcodec_open2(pVCtx, pVideoCodec, NULL) < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Fail to Initialize Decoder\n");
			return 0;
		}
	}

	///> Find Audio Decoder
	if (fmt_ctx->nASI != -1) {
		AVCodec *pAudioCodec = avcodec_find_decoder(fmt_ctx->ifmt_ctx->streams[fmt_ctx->nASI]->codec->codec_id);

		if (pAudioCodec == NULL)
		{
			av_log(NULL, AV_LOG_ERROR, "No Audio Decoder was Found\n");
			return 0;
		}

		///> Initialize Codec Context as Decoder
		if (avcodec_open2(pACtx, pAudioCodec, NULL) < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Fail to Initialize Decoder\n");
			return 0;
		}
	}

	if (flag) {
		return pACtx;
	}
	else {
		return pVCtx;
	}
}

int OpenImage(const char* imageFileName)
{

	if (avformat_open_input(&picFormatCtx, imageFileName, 0, 0) != 0)
	{
		printf("Can't open image file '%s'\n", imageFileName);
		return NULL;
	}

	av_dump_format(picFormatCtx, 0, imageFileName, false);

	AVCodecContext *pCodecCtx;

	pCodecCtx = picFormatCtx->streams[0]->codec;
	pCodecCtx->width = picFormatCtx->streams[0]->codec->width;
	pCodecCtx->height = picFormatCtx->streams[0]->codec->width;
	pCodecCtx->pix_fmt = picFormatCtx->streams[0]->codec->pix_fmt;

	// Find the decoder for the video stream
	AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (!pCodec)
	{
		printf("Codec not found\n");
		return NULL;
	}

	// Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0)
	{
		printf("Could not open codec\n");
		return NULL;
	}

	// 


	pictureFrame = av_frame_alloc();

	if (!pictureFrame)
	{
		printf("Can't allocate memory for AVFrame\n");
		return NULL;
	}

	int frameFinished;
	int numBytes;

	// Determine required buffer size and allocate buffer
	numBytes = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

	avpicture_fill((AVPicture *)pictureFrame, buffer, fmt_ctx->ifmt_ctx->streams[fmt_ctx->nVSI]->codec->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	// Read frame

	AVPacket packet;

	int framesNumber = 0;
	while (av_read_frame(picFormatCtx, &packet) >= 0)
	{
		if (packet.stream_index != 0)
			continue;

		int ret = avcodec_decode_video2(pCodecCtx, pictureFrame, &frameFinished, &packet);

		if (ret > 0)
		{
			FilterContext* imagefilter_ctx = &vfilter_ctx;
			if (av_buffersrc_add_frame(imagefilter_ctx->src_ctx, pictureFrame) < 0)
			{
				printf("Error occurred when putting frame into filter context\n");
			}
			while (1)
			{
				// Get frame from filter, if it returns < 0 then filter is currently empty.
				if (av_buffersink_get_frame(imagefilter_ctx->sink_ctx, m_picture_frame) < 0)
				{
					break;
				}
			} // while
			printf("Frame is decoded, size %d\n", ret);
			pictureFrame->quality = 4;
			return 1;
		}
		else
			printf("Error [%d] while decoding frame: %s\n", ret, strerror(AVERROR(ret)));
	}
}

static int insert_filter(FilterContext* F_ctx, char* filter_name, const char* command, int flag)
{
	if (flag == 0) {
		AVFilterContext		*filter_ctx;
		char args[512];

		int last_filter_idx = F_ctx->last_filter_idx;
		AVFilterContext* last_filter = F_ctx->filter_graph->filters[last_filter_idx];

		if (command == NULL) {
			command = "";
		}

		AVFilter* What_Filter = avfilter_get_by_name(filter_name);

		snprintf(args, sizeof(args), "%s", command); // write command to buffer
		if (avfilter_graph_create_filter(&filter_ctx, avfilter_get_by_name(filter_name), filter_name, args, NULL, F_ctx->filter_graph) < 0)
		{
			printf("Failed to create video scale filter\n");
			return -4;
		}

		if (F_ctx->last_filter_idx == Filter_default_idx) {
			if (avfilter_link(F_ctx->outputs->filter_ctx, 0, filter_ctx, 0) < 0)
			{
				printf("Failed to link video format filter\n");
				return -4;
			}
		}
		else {
			if (avfilter_link(last_filter, 0, filter_ctx, 0) < 0)
			{
				printf("Failed to link video format filter\n");
				return -4;
			}
		}

		F_ctx->last_filter_idx = F_ctx->filter_graph->nb_filters - 1;
	}
	return 0;
}



static int init_video_filter()
{
	AVStream			*stream = fmt_ctx->ifmt_ctx->streams[0];
	AVCodecContext		*codec_ctx = stream->codec;

	AVFilterInOut		*inputs, *outputs;

	char args[512];

	vfilter_ctx.filter_graph = NULL;
	vfilter_ctx.src_ctx = NULL;
	vfilter_ctx.sink_ctx = NULL;

	// Allocate memory for filter graph
	vfilter_ctx.filter_graph = avfilter_graph_alloc();
	if (vfilter_ctx.filter_graph == NULL)
	{
		return -1;
	}

	// Link input and output with filter graph.
	if (avfilter_graph_parse2(vfilter_ctx.filter_graph, "null", &inputs, &outputs) < 0)
	{
		printf("Failed to parse video filtergraph\n");
		return -2;
	}

	// Create input filter
	// Create Buffer Source -> input filter
	snprintf(args, sizeof(args), "time_base=%d/%d:video_size=%dx%d:pix_fmt=%d:pixel_aspect=%d/%d"
		, stream->time_base.num, stream->time_base.den
		, codec_ctx->width, codec_ctx->height
		, codec_ctx->pix_fmt
		, codec_ctx->sample_aspect_ratio.num, codec_ctx->sample_aspect_ratio.den);

	// Create Buffer Source
	if (avfilter_graph_create_filter(
		&vfilter_ctx.src_ctx
		, avfilter_get_by_name("buffer")
		, "in", args, NULL, vfilter_ctx.filter_graph) < 0)
	{
		printf("Failed to create video buffer source\n");
		return -3;
	}

	// Link Buffer Source with input filter
	if (avfilter_link(vfilter_ctx.src_ctx, 0, inputs->filter_ctx, 0) < 0)
	{
		printf("Failed to link video buffer source\n");
		return -4;
	}

	// Create Output filter
	// Create Buffer Sink
	if (avfilter_graph_create_filter(
		&vfilter_ctx.sink_ctx
		, avfilter_get_by_name("buffersink")
		, "out", NULL, NULL, vfilter_ctx.filter_graph) < 0)
	{
		printf("Failed to create video buffer sink\n");
		return -3;
	}

	vfilter_ctx.last_filter_idx = vfilter_ctx.filter_graph->nb_filters -1;
	vfilter_ctx.inputs = inputs;
	vfilter_ctx.outputs = outputs;
}

static int set_video_filter(FilterContext* F_ctx) {

	// stream 관련 수정 필요.
	AVFilterContext* last_filter;
	last_filter						= F_ctx->filter_graph->filters[F_ctx->last_filter_idx];

	AVStream			*stream		= fmt_ctx->ifmt_ctx->streams[fmt_ctx->nVSI];
	AVCodecContext		*codec_ctx	= stream->codec;

	// aformat is linked with Buffer Sink filter.
	if (avfilter_link(last_filter, 0, F_ctx->sink_ctx, 0) < 0)
	{
		printf("Failed to link video format filter\n");
		return -4;
	}

	av_buffersink_set_frame_size(F_ctx->sink_ctx, codec_ctx->frame_size);

	// Configure all prepared filters.
	if (avfilter_graph_config(F_ctx->filter_graph, NULL) < 0)
	{
		printf("Failed to configure video filter context\n");
		return -5;
	}

	avfilter_inout_free(&F_ctx->inputs);
	avfilter_inout_free(&F_ctx->outputs);

	return 0;
}

static void release_video_filter() {
	avfilter_graph_free(&vfilter_ctx.filter_graph);
	avfilter_graph_free(&afilter_ctx.filter_graph);
}



char* find_Format(string input_format) {
	if (input_format.find("mp4") != -1)
	{
		return "mp4";
	}
	else if (input_format.find("avi") != -1)
	{
		return "avi";
	}
	else if (input_format.find("flv") != -1)
	{
		return "flv";
	}
	else
	{
		return "ts";
	}
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame, char* filename)
{
	FILE *pFile;
	char szFilename[32];
	int  y;

	// Open file
	sprintf_s(szFilename, "dump/frame%d.ppm", iFrame);
	fopen_s(&pFile, szFilename, "wb");
	if (pFile == NULL)
		return;

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for (y = 0; y<height; y++)
		fwrite(pFrame->data[0] + y*pFrame->linesize[0], 1, width * 3, pFile);

	// Close file
	fclose(pFile);
}

int save_frame_as_jpeg(AVCodecContext *pCodecCtx, AVFrame *pFrame, int FrameNo) {
	AVCodec *jpegCodec = avcodec_find_encoder(AV_CODEC_ID_JPEG2000);
	if (!jpegCodec) {
		return -1;
	}
	AVCodecContext *jpegContext = avcodec_alloc_context3(jpegCodec);
	if (!jpegContext) {
		return -1;
	}

	jpegContext->pix_fmt = AV_PIX_FMT_YUV420P;
	jpegContext->height = pFrame->height;
	jpegContext->width = pFrame->width;
	jpegContext->time_base = { 1,25 };

	if (avcodec_open2(jpegContext, jpegCodec, NULL) < 0) {
		return -1;
	}
	FILE *JPEGFile;
	char JPEGFName[256];

	AVPacket packet;
	av_init_packet(&packet);
	int gotFrame;

	if (avcodec_encode_video2(jpegContext, &packet, pFrame, &gotFrame) < 0) {
		return -1;
	}

	sprintf(JPEGFName, "dvr-%06d.jpg", FrameNo);
	JPEGFile = fopen(JPEGFName, "wb");
	fwrite(packet.data, 1, packet.size, JPEGFile);
	fclose(JPEGFile);

	av_free_packet(&packet);
	avcodec_close(jpegContext);
	return 0;
}

int dump_frame_to_jpeg(AVContext * av, Options * options, int frameno, char* timestamp ) {
    AVCodec *jpegCodec = avcodec_find_encoder(AV_CODEC_ID_JPEG2000);
    if (!jpegCodec) {
        return -1;
    }
    AVCodecContext *jpegContext = avcodec_alloc_context3(jpegCodec);
    if (!jpegContext) {
        return -1;
    }

    jpegContext->pix_fmt = av->video_dec_ctx->pix_fmt;
    jpegContext->height = av->frame->height;
    jpegContext->width = av->frame->width;
    jpegContext->sample_aspect_ratio = av->video_dec_ctx->sample_aspect_ratio;
    jpegContext->time_base = av->video_dec_ctx->time_base;
    jpegContext->compression_level = 100;
    jpegContext->thread_count = 1;
    jpegContext->prediction_method = 1;
    jpegContext->flags2 = 0;
    jpegContext->rc_max_rate = jpegContext->rc_min_rate = jpegContext->bit_rate = 80000000;
 //   DEBUG( "after setting jpegContext" );

    if (avcodec_open2(jpegContext, jpegCodec, NULL) < 0) {
        return -1;
    }
  //  DEBUG( "after opening jpegContext" );
    FILE *JPEGFile;
    char JPEGFName[256];

	AVPacket packet;
    av_init_packet(&packet);
    int gotFrame;
  //  DEBUG( "after initializing packet" );
    av_dump_format(av->fmt_ctx, 0, "", 0);

    if (avcodec_encode_video2(jpegContext, &packet, av->frame, &gotFrame) < 0) {
        return -1;
    }

 //   DEBUG( "after encoding video" );

 //   if( DO_TIMESTAMPS ) {
        sprintf(JPEGFName, options->output_file_pattern, frameno);

    JPEGFile = fopen(JPEGFName, "wb");
    fwrite(packet.data, 1, packet.size, JPEGFile);
    fclose(JPEGFile);
 //   DEBUG( "after writing frame" );

    av_free_packet(&packet);
    avcodec_close(jpegContext);

    return 0;
}

void SaveBMP(AVFrame *frame, int width, int height, int iframe) {
	// set filename
	char filename[32];
	sprintf(filename, "frame%d.bmp", iframe);
	// create file
	FILE *fout;
	fout = fopen(filename, "wb");
	// create bmp header
	BITMAPFILEHEADER bmpheader;
	bmpheader.bfReserved1 = 0;
	bmpheader.bfReserved2 = 0;
	bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmpheader.bfSize = bmpheader.bfOffBits + width*height * 24 / 8;
	// create bmp info
	BITMAPINFO bmpinfo;
	bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.bmiHeader.biWidth = width;
	bmpinfo.bmiHeader.biHeight = -height;
	bmpinfo.bmiHeader.biPlanes = 1;
	bmpinfo.bmiHeader.biBitCount = 24;
	bmpinfo.bmiHeader.biCompression = BI_RGB;
	bmpinfo.bmiHeader.biSizeImage = 0;
	bmpinfo.bmiHeader.biXPelsPerMeter = 100;
	bmpinfo.bmiHeader.biYPelsPerMeter = 100;
	bmpinfo.bmiHeader.biClrUsed = 0;
	bmpinfo.bmiHeader.biClrImportant = 0;
	// write file
	fwrite(&bmpheader, sizeof(BITMAPFILEHEADER), 1, fout);
	fwrite(&bmpinfo.bmiHeader, sizeof(BITMAPINFOHEADER), 1, fout);
	fwrite(frame->data[0], width*height * 24 / 8, 1, fout);
	fclose(fout);
}

void SaveAsBMP(AVFrame *pFrameRGB, int width, int height, int index, int bpp)
{
	char buf[5] = { 0 };
	BITMAPFILEHEADER bmpheader;
	BITMAPINFOHEADER bmpinfo;
	FILE *fp;

	char *filename = new char[255];

	sprintf_s(filename, 255, "%s%d.bmp", "C:/temp/", index);
	if ((fp = fopen(filename, "wb+")) == NULL) {
		printf("open file failed!\n");
		return;
	}

	bmpheader.bfType = 0x4d42;
	bmpheader.bfReserved1 = 0;
	bmpheader.bfReserved2 = 0;
	bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmpheader.bfSize = bmpheader.bfOffBits + width*height*bpp / 8;

	bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.biWidth = width;
	bmpinfo.biHeight = height;
	bmpinfo.biPlanes = 1;
	bmpinfo.biBitCount = bpp;
	bmpinfo.biCompression = BI_RGB;
	bmpinfo.biSizeImage = (width*bpp + 31) / 32 * 4 * height;
	bmpinfo.biXPelsPerMeter = 100;
	bmpinfo.biYPelsPerMeter = 100;
	bmpinfo.biClrUsed = 0;
	bmpinfo.biClrImportant = 0;

	fwrite(&bmpheader, sizeof(bmpheader), 1, fp);
	fwrite(&bmpinfo, sizeof(bmpinfo), 1, fp);
	fwrite(pFrameRGB->data[0], width*height*bpp / 8, 1, fp);

	fclose(fp);
}

int main()
{
	// Register
	av_register_all();
	avfilter_register_all();

#ifdef _NETWORK
	avformat_network_init();
#endif
	fmt_ctx = new Format_ctx_Set;
	fmt_ctx_audio = new Format_ctx_Set;

	// Input file Setting
	fmt_ctx->ifmt_ctx		= init_input_format(szFilePath, fmt_ctx);
	fmt_ctx_audio->ifmt_ctx = init_input_format(szFilePath_2, fmt_ctx_audio);

	// Encoder Setting
	AVCodecContext* V_enc_codec_ctx = NULL;
	AVCodecContext* A_enc_codec_ctx = NULL;
	V_enc_codec_ctx = init_encoder(fmt_ctx->ifmt_ctx, v_codec_id, a_codec_id, Video_flag, fmt_ctx);
	A_enc_codec_ctx = init_encoder(fmt_ctx->ifmt_ctx, v_codec_id, a_codec_id, Audio_flag, fmt_ctx_audio);

	// Decoder Setting
	AVCodecContext* dec_V_ctx = NULL;
	AVCodecContext* dec_A_ctx = NULL;
	dec_V_ctx = init_decoder(fmt_ctx, Video_flag);
	dec_A_ctx = init_decoder(fmt_ctx_audio, Audio_flag);

	// Output file Setting
	outputFormat = find_Format(fmt_ctx->ifmt_ctx->iformat->name);
	fmt_ctx->ofmt_ctx = init_output_format(fmt_ctx->ifmt_ctx, outputfile, V_enc_codec_ctx);
	fmt_ctx->ofmt_ctx = add_stream_output_format(fmt_ctx, fmt_ctx, V_enc_codec_ctx, Video_flag);
	fmt_ctx->ofmt_ctx = add_stream_output_format(fmt_ctx_audio, fmt_ctx, A_enc_codec_ctx, Audio_flag);
	fmt_ctx->ofmt_ctx = write_output_header(fmt_ctx, outputfile);
	
	

	// Read Var
	AVPacket read_pkt;
	AVPacket out_pkt;
	AVFrame* pVFrame;
	AVFrame* pAFrame;
	AVFrame* filtered_frame;
	
	// Flag
	int bGotPicture = 0;		// flag for video decoding
	int bGotSound = 0;			// flag for audio decoding

	// Frame Alloc
	pVFrame = av_frame_alloc();
	pAFrame = av_frame_alloc();
	filtered_frame = av_frame_alloc();
	
	// Packet init
	av_init_packet(&out_pkt);

	// Video Filter Setting
	init_video_filter();									// Initialize Filter

//	insert_filter(&vfilter_ctx, "negate", "", 0);			// Apply Negative Filter
	insert_filter(&vfilter_ctx, "rotate", "(t*PI*3)/180", 0);	// Apply Rotate Filter
	insert_filter(&vfilter_ctx, "prewitt", "", 0);				// Apply Null Filter

	set_video_filter(&vfilter_ctx);							// Link Output Filter



	//string str;						// 필터에 입력할 명령어
	//const char *cstr;				// 명령어 변환 (String to char)
	//cstr = str.c_str();				// 명령어 변환 실행

	//insert_filter(&vfilter_ctx, "zoompan", "z='min(zoom+0.0015,1.5)':d=700:x='iw/2-(iw/zoom/2)':y='ih/2-(ih/zoom/2)'", 0);

		// 삽입된 필터 정리

	int stream_index;

	/*if (!OpenImage("./Example.jpg")) {
	return 0;
	}*/

	//8level grayscale 변화 
	/*
	for (int i = 0; i < dst_width * (dst_height); i++) {
	m_picture_frame->data[0][i] = (m_picture_frame->data[0][i] / 32) * 32;//0~255 Y
	if (i < dst_width * ((dst_height+32) / 4)) {
	m_picture_frame->data[1][i] = 127;//0~255 U
	m_picture_frame->data[2][i] = 127;//0~255 V
	}

	}*/
	//Sepia 필터
	/*
	for (int i = 0; i < dst_width * (dst_height); i++) {
	if (i < dst_width * ((dst_height+32) / 4)) {
	m_picture_frame->data[1][i] = 113;//0~255 U
	m_picture_frame->data[2][i] = 143;//0~255 V
	}

	}*/
	//반전
	/*
	for (int i = 0; i < dst_width * (dst_height); i++) {
	m_picture_frame->data[0][i] = 255- m_picture_frame->data[0][i];//0~255 Y
	if (i < dst_width * ((dst_height+32) / 4)) {
	m_picture_frame->data[1][i] = 255- m_picture_frame->data[1][i];//0~255 U
	m_picture_frame->data[2][i] = 255- m_picture_frame->data[2][i];//0~255 V
	}

	}*/
	/*
	for (int i = 0; i < dst_width * (dst_height); i++) {
	m_picture_frame->data[0][i] = 255 - m_picture_frame->data[0][i];//0~255 Y
	if (i < dst_width * ((dst_height+32) / 4)) {
	m_picture_frame->data[1][i] = 255 - m_picture_frame->data[1][i];//0~255 U
	m_picture_frame->data[2][i] = 255 - m_picture_frame->data[2][i];//0~255 V
	}
	}*/

	int ret = -1;
	int flag = 0;

	// Video
	while (av_read_frame(fmt_ctx->ifmt_ctx, &read_pkt) >= 0)
	{
		// Check Video stream & Read Packet
		if (read_pkt.stream_index == fmt_ctx->nVSI)
		{
			if (read_pkt.data != NULL) {
				
				// Decoding
				if (avcodec_send_packet(dec_V_ctx, &read_pkt) < 0)
				av_log(NULL, 16, "Video Send Packet Error\n");
				if (avcodec_receive_frame(dec_V_ctx, pVFrame) < 0)
				av_log(NULL, 16, "Video Receive Frame Error\n");

				// Filter Setting
				FilterContext* filter_ctx;
				stream_index = read_pkt.stream_index;
				if (stream_index == fmt_ctx->nVSI)
				{
					filter_ctx = &vfilter_ctx;
				}
				else
				{
					filter_ctx = &afilter_ctx;
				}

				// Put frame into filter.
				if (av_buffersrc_add_frame(filter_ctx->src_ctx, pVFrame) < 0)
				{
					printf("Error occurred when putting frame into filter context\n");
					break;
				}

				while (1)
				{
					// Get frame from filter, if it returns < 0 then filter is currently empty.
					if (av_buffersink_get_frame(filter_ctx->sink_ctx, filtered_frame) < 0)
					{
						break;
					}
				}

				int H = 480;
				int W = filtered_frame->linesize[0];
				////SaveFrame(filtered_frame, 500, 350, 0, "Before.ppm");
				for (int i = 0; i < H; i++) {
					for (int j = 0; j < W; j++) {
						if (j < W / 2) {
							//filtered_frame->data[0][i*W + j] = 0;
						//	filtered_frame->data[1][(i*W + j) / 4] = 0;
						//	filtered_frame->data[2][(i*W + j) / 4] = 0;
						}

						/*if (i < H / 2) {
							filtered_frame->data[0][i*W + j] = 0;
						}*/

						if (j % 4 != 0) {
						/*	filtered_frame->data[0][i*W + j] = 0;
							filtered_frame->data[1][(i*W + j) / 4] = 127;
							filtered_frame->data[2][(i*W + j) / 4] = 127;*/
						}
						//filtered_frame->data[0][i*W + j] = (filtered_frame->data[0][i*W + j] + filtered_frame->data[0][i*W + j] + filtered_frame->data[0][i*W + j+2] + filtered_frame->data[0][i*W + j+3])/4;
						//filtered_frame->data[1][(i*W + j) / 4] = 127;
						//filtered_frame->data[2][(i*W + j) / 4] = 127;
					//	j = j * 4;
					/*	if (filtered_frame->data[2][(i*W + j) / 4] == 0) {
							filtered_frame->data[1][(i*W + j) / 4] = 127;
							filtered_frame->data[2][(i*W + j) / 4] = 127;
						}*/
					}
				}


				// Encoding
				if (avcodec_send_frame(V_enc_codec_ctx, filtered_frame) < 0) {
					av_log(NULL, AV_LOG_ERROR, "Video Send Frame Error\n");
				}

				if (avcodec_receive_packet(V_enc_codec_ctx, &out_pkt) >= 0) {
					cout << "Video Write Frame [pts]=>" << out_pkt.pts << "  [size]=>" << out_pkt.size << endl;
					av_interleaved_write_frame(fmt_ctx->ofmt_ctx, &out_pkt);
					av_free_packet(&out_pkt);
				}
			}
		}

		av_free_packet(&read_pkt);
	}
	// Video_Trailer
	if (fmt_ctx->nVSI != -1) {
		for (bGotPicture = 1; bGotPicture;)
		{
			fflush(stdout);
			if (avcodec_encode_video2(V_enc_codec_ctx, &read_pkt, NULL, &bGotPicture) < 0)
			{
				fprintf(stderr, "Error encoding frame\n");
				exit(1); 
			}
			if (bGotPicture)
			{
				cout << "Video Write Frame [pts]=>" << read_pkt.pts << "  [size]=>" << read_pkt.size << "\t\ttrailer" << endl;
				av_interleaved_write_frame(fmt_ctx->ofmt_ctx, &read_pkt);
				av_free_packet(&read_pkt);
			}
		}
	}
	
	// Audio
	/*
	while (av_read_frame(fmt_ctx_audio->ifmt_ctx, &read_pkt) >= 0) {
		// Check Audio stream & Read Packet
		if (read_pkt.stream_index == fmt_ctx_audio->nASI)
		{
			if (read_pkt.data != NULL)
			{

				// Decode
				if (avcodec_send_packet(dec_A_ctx, &read_pkt) < 0)
					av_log(NULL, AV_LOG_ERROR, "Audio Send Packet Error\n");
				if (avcodec_receive_frame(dec_A_ctx, pAFrame) < 0)
					av_log(NULL, AV_LOG_ERROR, "Audio Receive Frame Error\n");

				// Encode
				ret = avcodec_send_frame(A_enc_codec_ctx, pAFrame);
				if (ret < 0)
					av_log(NULL, 16, "Audio Send Frame Error\n");
				ret = avcodec_receive_packet(A_enc_codec_ctx, &out_pkt);
				if (ret < 0)
					av_log(NULL, 16, "Audio Receive Packet Error\n");

				// Write
				cout << "Audio Write Frame [pts]=>" << read_pkt.pts << "  [size]=>" << read_pkt.size << endl;
				av_interleaved_write_frame(fmt_ctx->ofmt_ctx, &read_pkt);
				av_free_packet(&out_pkt);
			}
		}
	}
	*/
	// Write File Trailer
	av_write_trailer(fmt_ctx->ofmt_ctx);



	
	// Release
	avcodec_close(V_enc_codec_ctx);
	av_free(V_enc_codec_ctx);
	av_free(pVFrame);
	av_free(pAFrame);
	release_video_filter();

	//> Close an opened input AVFormatContext. 
	avformat_close_input(&fmt_ctx->ifmt_ctx);
	avformat_close_input(&fmt_ctx->ofmt_ctx);

#ifdef _NETWORK
	//> Undo the initialization done by avformat_network_init.
	avformat_network_deinit();
#endif
	//system("pause");
	return 0;
}