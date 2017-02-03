#pragma once
///> Include FFMpeg

#define _CRT_SECURE_NO_DEPRECATE
#pragma warning(disable:4996)

#include <iostream>
#include <windows.h>
#include <string>
#include "RGBImage.h"
#include "stdafx.h"
#include <cstdlib>
#include <ctime>

using namespace std;

// 시간 측정용 변수
struct timeval _start_time, _stop_time, _start_time_2, _stop_time_2;

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

// 필터 구조체
typedef struct _FilterContext {
	AVFilterGraph	*filter_graph;	// Filter's Context
	AVFilterContext	*src_ctx;		// Linked Buffer Source
	AVFilterContext	*sink_ctx;		// Linked Buffer Sink
	AVFilterInOut	*inputs;		// Filter Input
	AVFilterInOut	*outputs;		// Filter Output
	int last_filter_idx = 0;		// Last filter index for Linking
} FilterContext;

// 코덱 구조체
typedef struct _Codec_Set {
	AVCodec *codec = NULL;
	AVCodecContext *V_codec_ctx = NULL;
	AVCodecContext *A_codec_ctx = NULL;
} Codec_Set;

// 파일 구조체
typedef struct _Format_ctx_Set {
	AVInputFormat   *ifmt = NULL;
	AVOutputFormat  *ofmt = NULL;
	AVFormatContext *ifmt_ctx = NULL;
	AVFormatContext *ofmt_ctx = NULL;
	int nVSI = -1;
	int nASI = -1;
} Format_ctx_Set;

// 사용 할 변수
AVFrame *pictureFrame;					// 받아올 이미지 프레임
AVFrame *m_picture_frame;				// 변환 될 이미지 프레임
Format_ctx_Set	*fmt_ctx = NULL;		// 비디오 파일
Format_ctx_Set	*fmt_ctx_audio = NULL;	// 오디오 파일
AVInputFormat   *ifmt = NULL;			// input file
AVOutputFormat  *ofmt = NULL;			// output file
AVFormatContext *picFormatCtx = NULL;	// 이미지 파일의 정보

										// Filter Option
int angle = 0;
int dst_width = 80 * 8;
int dst_height = 60 * 8;

// Codec_id
AVCodecID v_codec_id = AV_CODEC_ID_H264;
AVCodecID a_codec_id = AV_CODEC_ID_MP3;

// File_Path
const char *szFilePath = "./Test_Video/Test_Video.mp4";
const char *szFilePath_2 = "./Test_Video/Test_Video_2.mp4";
const char *szFilePath_3 = "./Test_Video/Canon.mp3";
const char *szImagePath = "./Test_Video/sample.png";
const char *outputfile = "./Test_Video/output.mp4";
const char *outputFormat = "mp4";

// Filter
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

	pCodecCtx->width = 128;
	pCodecCtx->height = 128;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVA420P;

	// Find the decoder for the video stream
	AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

	if (!pCodec)
	{
		printf("Codec not found\n");
		return NULL;
	}

	// Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Could not open codec\n");
		return NULL;
	}




	m_picture_frame = av_frame_alloc();
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
			FilterContext* imagefilter_ctx = &afilter_ctx;
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

static int init_video_filter(FilterContext* filter_ctx)
{
	AVStream			*stream = fmt_ctx->ifmt_ctx->streams[0];
	AVCodecContext		*codec_ctx = stream->codec;

	AVFilterInOut		*inputs, *outputs;

	char args[512];

	filter_ctx->filter_graph = NULL;
	filter_ctx->src_ctx = NULL;
	filter_ctx->sink_ctx = NULL;

	// Allocate memory for filter graph
	filter_ctx->filter_graph = avfilter_graph_alloc();
	if (filter_ctx->filter_graph == NULL)
	{
		return -1;
	}

	// Link input and output with filter graph.
	if (avfilter_graph_parse2(filter_ctx->filter_graph, "null", &inputs, &outputs) < 0)
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
		&filter_ctx->src_ctx
		, avfilter_get_by_name("buffer")
		, "in", args, NULL, filter_ctx->filter_graph) < 0)
	{
		printf("Failed to create video buffer source\n");
		return -3;
	}

	// Link Buffer Source with input filter
	if (avfilter_link(filter_ctx->src_ctx, 0, inputs->filter_ctx, 0) < 0)
	{
		printf("Failed to link video buffer source\n");
		return -4;
	}

	// Create Output filter
	// Create Buffer Sink
	if (avfilter_graph_create_filter(
		&filter_ctx->sink_ctx
		, avfilter_get_by_name("buffersink")
		, "out", NULL, NULL, filter_ctx->filter_graph) < 0)
	{
		printf("Failed to create video buffer sink\n");
		return -3;
	}

	filter_ctx->last_filter_idx = filter_ctx->filter_graph->nb_filters - 1;
	filter_ctx->inputs = inputs;
	filter_ctx->outputs = outputs;
}

static int set_video_filter(FilterContext* F_ctx) {

	// stream 관련 수정 필요. (수정 다함)
	AVFilterContext* last_filter;
	last_filter = F_ctx->filter_graph->filters[F_ctx->last_filter_idx];

	AVStream			*stream = fmt_ctx->ifmt_ctx->streams[fmt_ctx->nVSI];
	AVCodecContext		*codec_ctx = stream->codec;

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
	else if (input_format.find("matroska") != -1)
	{
		return "matroska";
	}
	else if (input_format.find("png") != -1) {
		return "png";
	}
	else
	{
		return "ts";
	}
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
	fmt_ctx->ifmt_ctx = init_input_format(szFilePath, fmt_ctx);
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

	// Read Var (file을 읽으면 값이 저장 될 변수)
	AVPacket read_pkt;				// 읽은 파일의 패킷
	AVPacket out_pkt;				// 저장할 파일의 패킷
	AVFrame* pVFrame;				// 디코딩된 비디오 프레임
	AVFrame* pAFrame;				// 디코딩된 오디오 프레임
	AVFrame* filtered_frame;		// 필터된 프레임

									// Flag
	int bGotPicture = 0;			// flag for video decoding
	int bGotSound = 0;				// flag for audio decoding				
	pVFrame = av_frame_alloc();		// Frame Alloc
	pAFrame = av_frame_alloc();
	filtered_frame = av_frame_alloc();

	// Packet init
	av_init_packet(&out_pkt);



	/* 필터 설정 부분 */

	// Video Filter Setting
	init_video_filter(&vfilter_ctx);										// Initialize Filter
	insert_filter(&vfilter_ctx, "hue", "H=t*PI/2:s=1", 0);					// Apply hue Filter
	insert_filter(&vfilter_ctx, "format", "yuva420p", 0);					// Apply Alpha
	set_video_filter(&vfilter_ctx);											// Link Output Filter

																			// Image Filter Setting
	int V_width = V_enc_codec_ctx->width;
	int V_height = V_enc_codec_ctx->height;

	string str_left = to_string(V_width);									// 필터에 입력할 명령어
	string str_right = to_string(V_height);									// 필터에 입력할 명령어
	string str = str_left + ":" + str_right;
	const char *cstr;														// 명령어 변환 (String to char)
	cstr = str.c_str();														// 명령어 변환 실행
	init_video_filter(&afilter_ctx);
	insert_filter(&afilter_ctx, "scale", "128:128", 0);
	insert_filter(&afilter_ctx, "format", "yuva420p", 0);
	set_video_filter(&afilter_ctx);

	/* 필터 설정 부분 종료 */



	/* 이미지 입력 부분 */

	if (!OpenImage(szImagePath)) {
		return 0;
	}

	/* 이미지 입력 종료 */

	/* 입력한 이미지 좌표 변환용 임시 변수 */
	int ret = -1;
	int valo = 0;
	bool dire = true;
	srand((unsigned int)time(NULL));


	int d_x = 0;				// 마스킹 위치
	int d_y = 0;
	int stream_index;			// 프레임 인덱스 (비디오, 오디오)
	int i = 0;
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

				/* YUV -> RGB 변환 부분 */
				RGBImage* frame;
				frame = new RGBImage(dst_width, dst_height);
				frame->copyFrame(filtered_frame);						// Video frame을 RGB로 Copy.
				u_char s_R, s_G, s_B;									// Source
				u_char d_R, d_G, d_B;									// Destination
				u_char r_R, r_G, r_B;									// Result
				u_char Alpha_value;										// Alpha

				RGBImage* m_frame;
				m_frame = new RGBImage(dst_width, dst_height);
				m_frame->copyFrame_Alpha(m_picture_frame);				// Mask의 frame을 RGB로 Copy.
																		/* YUV -> RGB 변환 부분 종료 */


																		/* 비디오를 편집 할 부분 정의*/
				int start = 5 * fmt_ctx->ofmt_ctx->streams[0]->codec->time_base.den;
				int end = 10 * fmt_ctx->ofmt_ctx->streams[0]->codec->time_base.den;
				/* 비디오를 편집 할 부분 정의 종료 */

				/* 편집 될 위치 */
				int W = filtered_frame->linesize[0];
				int H = 480 + 74;
				/* 편집 될 위치 종료 */

				/* 마스킹할 이미지의 정보 */
				int Image_S_W = 0;	// 이미지 시작점
				int Image_S_H = 0;
				int Image_W = 128;	// 이미지 크기
				int Image_H = 128;

				/* 이미지가 이동하는 방향, 속도 정의 (예시)*/
				if (dire) {
					valo = valo + 1;
				}
				else {
					valo = valo - 1;
				}
				if (valo >= 4) {
					dire = false;
				}
				if (valo <= -3) {
					dire = true;
				}
				d_x = d_x + valo;
				d_y = d_y + valo;
				if (d_x > (300) || d_y > (300)) {
					d_x = 0;
					d_y = 0;
				}
				/* 이미지가 이동하는 방향, 속도 정의 종료 */

				// Video의 Start 부터 End 까지 편집 ( 구간 설정 )
				if (i >= start && i <= end) {
					// 마스킹 할 Image의 사이즈 만큼 편집
					for (int i = Image_S_H; i < Image_H + Image_S_H; i++) {
						for (int j = Image_S_W; j < Image_W + Image_S_W; j++) {
							// Alpha 값이 있는 Data만 편집
							if (!m_frame->checkPixelAlpha(i*Image_W + j)) {
								// frame data를 가져옴
								m_frame->getPixelColor_Alpha(s_R, s_G, s_B, Alpha_value, i*Image_W + j);
								frame->getPixelColor(d_R, d_G, d_B, ((i + d_y)*W) + (j + d_x));
								// Alpha blending.
								if (Alpha_value > 254) {
									r_R = ((float)((Alpha_value) / 255) * s_R) + (((float)(255 - Alpha_value) / 255) * d_R);
									r_B = ((float)((Alpha_value) / 255) * s_B) + (((float)(255 - Alpha_value) / 255) * d_B);
									r_G = ((float)((Alpha_value) / 255) * s_G) + (((float)(255 - Alpha_value) / 255) * d_G);
									// blending한 data를 다시 frame에 설정해줌.
									frame->setPixelColor(r_R, r_G, r_B, ((i + d_y)*W) + (j + d_x));
								}
							}
						}
					}
					// YUV로 다시 설정.
					frame->RGB2YUV(*filtered_frame);
					delete frame;
				}

				// Encoding
				if (avcodec_send_frame(V_enc_codec_ctx, filtered_frame) < 0) {
					av_log(NULL, AV_LOG_ERROR, "Video Send Frame Error\n");
				}
				if (avcodec_receive_packet(V_enc_codec_ctx, &out_pkt) >= 0) {
					cout << "Video Write Frame [pts]=>" << out_pkt.pts << "  [size]=>" << out_pkt.size << endl;
					av_interleaved_write_frame(fmt_ctx->ofmt_ctx, &out_pkt);
					av_free_packet(&out_pkt);
					i++;
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
	//
	//while (av_read_frame(fmt_ctx_audio->ifmt_ctx, &read_pkt) >= 0) {
	//	// Check Audio stream & Read Packet
	//	if (read_pkt.stream_index == fmt_ctx_audio->nASI)
	//	{
	//		if (read_pkt.data != NULL)
	//		{
	//			// Decode
	//			if (avcodec_send_packet(dec_A_ctx, &read_pkt) < 0)
	//				av_log(NULL, AV_LOG_ERROR, "Audio Send Packet Error\n");
	//			if (avcodec_receive_frame(dec_A_ctx, pAFrame) < 0)
	//				av_log(NULL, AV_LOG_ERROR, "Audio Receive Frame Error\n");
	//			// Encode
	//			ret = avcodec_send_frame(A_enc_codec_ctx, pAFrame);
	//			if (ret < 0)
	//				av_log(NULL, 16, "Audio Send Frame Error\n");
	//			ret = avcodec_receive_packet(A_enc_codec_ctx, &out_pkt);
	//			if (ret < 0)
	//				av_log(NULL, 16, "Audio Receive Packet Error\n");
	//			// Write
	//			cout << "Audio Write Frame [pts]=>" << read_pkt.pts << "  [size]=>" << read_pkt.size << endl;
	//			av_interleaved_write_frame(fmt_ctx->ofmt_ctx, &read_pkt);
	//			av_free_packet(&out_pkt);
	//		}
	//	}
	//}
	//


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