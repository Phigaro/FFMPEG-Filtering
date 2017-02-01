#pragma once
///> Include FFMpeg

#define _CRT_SECURE_NO_DEPRECATE
#pragma warning(disable:4996)

#include <iostream>
#include <windows.h>

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
#include <libavfilter\avfilter.h>
#include <libavfilter\buffersink.h>
#include <libavfilter\buffersrc.h>

#endif
}

#define Video_flag 0
#define Audio_flag 1

///> Library Link On Windows System
#pragma comment( lib, "avformat.lib" )   
#pragma comment( lib, "avutil.lib" )
#pragma comment( lib, "avcodec.lib" )
#pragma comment( lib, "swscale.lib")
#pragma comment( lib, "avfilter.lib")

typedef struct _FilterContext {
	AVFilterGraph  *filter_graph;
	AVFilterContext* src_ctx;
	AVFilterContext* sink_ctx;
} FilterContext;

typedef struct _Codec_Set {
	AVCodec *codec = NULL;
	AVCodecContext *V_codec_ctx = NULL;
	AVCodecContext *A_codec_ctx = NULL;
} Codec_Set;
AVFrame *pictureFrame;
AVFrame *m_picture_frame;
AVInputFormat   *ifmt = NULL;
AVOutputFormat  *ofmt = NULL;
AVFormatContext *ifmt_ctx = NULL;
AVFormatContext *a_ifmt_ctx = NULL;
AVFormatContext *ofmt_ctx = NULL;
AVFormatContext *picFormatCtx = NULL;

int nVSI = -1;
int nASI = -1;

static const int dst_width = 16 * 50;
static const int dst_height = 9 * 50;

AVCodecID v_codec_id = AV_CODEC_ID_H264;
AVCodecID a_codec_id = AV_CODEC_ID_MP3;

const char *szFilePath = "sample2.mp4";
const char *outputfile = "output.mp4";
const char *audio_file = "audio.mp3";
const char *outputFormat = "mp4";

static FilterContext vfilter_ctx, afilter_ctx;

static AVFormatContext* init_input_format(const char* input_file_path) {

	AVCodec              *codec = NULL;
	AVCodecContext            *c = NULL;
	AVFormatContext *p_ifmt_ctx = NULL;

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
		if (nVSI < 0 && p_ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			nVSI = 0;
		}
		else if (nASI < 0 && p_ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			nASI = 1;
		}
	}

	if (nVSI < 0 && nASI < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "No Video & Audio Streams were Found\n");
		return 0;
	}

	return p_ifmt_ctx;
}

static AVFormatContext* init_output_format(AVFormatContext* p_ifmt_ctx, AVFormatContext* pa_ifmt_ctx, const char* output_file_path, AVCodecContext* enc_codec_ctx, AVCodecContext* A_enc_codec_ctx) {
	AVOutputFormat   *p_ofmt = NULL;
	AVFormatContext  *p_ofmt_ctx = NULL;

	avformat_alloc_output_context2(&p_ofmt_ctx, NULL, outputFormat, outputfile);
	if (!p_ofmt_ctx)
	{
		fprintf(stderr, "Could not create output context\n");
	}

	p_ofmt = p_ofmt_ctx->oformat;

	int ret = 0;
	for (int i = 0; i < p_ifmt_ctx->nb_streams - 1; i++)
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
			ret = avcodec_copy_context(out_stream->codec, A_enc_codec_ctx);
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
	AVStream *in_stream = pa_ifmt_ctx->streams[0];
	AVStream *out_stream = avformat_new_stream(p_ofmt_ctx, in_stream->codec->codec);

	if (pa_ifmt_ctx->streams[0]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		ret = avcodec_copy_context(out_stream->codec, enc_codec_ctx);
		out_stream->codec->codec_tag = 0;//문서
	}
	else if (pa_ifmt_ctx->streams[0]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
	{
		ret = avcodec_copy_context(out_stream->codec, A_enc_codec_ctx);
		out_stream->codec->codec_tag = 0;
	}

	if (ret < 0) {
		fprintf(stderr, "Failed to copy context from input to output stream codec context\n");
	}

	if (p_ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
	{
		out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
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

static AVCodecContext* init_encoder(AVFormatContext* p_ifmt_ctx, int flag) {

	AVCodec        *codec = NULL;
	AVCodecContext *c_ctx = NULL;

	if (flag == Video_flag) {
		if (nVSI != -1) {
			codec = avcodec_find_encoder(v_codec_id);
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
			if (v_codec_id == AV_CODEC_ID_H264)
				c_ctx->profile = FF_PROFILE_H264_BASELINE;
			c_ctx->width = p_ifmt_ctx->streams[nVSI]->codec->width;
			c_ctx->height = p_ifmt_ctx->streams[nVSI]->codec->height;
			c_ctx->pix_fmt = p_ifmt_ctx->streams[nVSI]->codec->pix_fmt;
			c_ctx->time_base = { 1,25 };
			c_ctx->bit_rate = 50 * 1000;

			if (avcodec_open2(c_ctx, codec, NULL) < 0)
			{
				fprintf(stderr, "Could not open codec\n");
				return 0;
			}
		}
	}

	if (flag == Audio_flag) {
		if (nASI != -1) {
			codec = avcodec_find_encoder(a_codec_id);
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
			c_ctx->channel_layout = p_ifmt_ctx->streams[0]->codec->channel_layout;
			c_ctx->channels = p_ifmt_ctx->streams[0]->codec->channels;
			c_ctx->sample_fmt = p_ifmt_ctx->streams[0]->codec->sample_fmt;

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

static AVCodecContext* init_decoder(AVFormatContext *p_ifmt_ctx, int flag) {
	AVCodecContext *pVCtx = p_ifmt_ctx->streams[nVSI]->codec;
	AVCodecContext *pACtx = p_ifmt_ctx->streams[0]->codec;

	for (int i = 0; i < p_ifmt_ctx->nb_streams; i++)
	{
		if (nVSI < 0 && p_ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			nVSI = 0;
		}
		else if (nASI < 0 && p_ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			nASI = 1;
		}
	}
	if (nVSI < 0 && nASI < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "No Video & Audio Streams were Found\n");
		return 0;
	}

	///> Find Video Decoder
	if (nVSI != -1) {
		AVCodec   *pVideoCodec = avcodec_find_decoder(p_ifmt_ctx->streams[nVSI]->codec->codec_id);

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
	if (nASI != -1) {
		AVCodec *pAudioCodec = avcodec_find_decoder(p_ifmt_ctx->streams[0]->codec->codec_id);

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

	avpicture_fill((AVPicture *)pictureFrame, buffer, ifmt_ctx->streams[nVSI]->codec->pix_fmt, pCodecCtx->width, pCodecCtx->height);

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
static int init_video_filter()
{
	AVStream       *stream = ifmt_ctx->streams[0];
	AVCodecContext   *codec_ctx = stream->codec;

	AVFilterContext   *rescale_filter;
	AVFilterContext   *rotate_filter;
	AVFilterInOut     *inputs, *outputs;

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

	// Create output filter
	// Create Buffer Sink
	if (avfilter_graph_create_filter(
		&vfilter_ctx.sink_ctx
		, avfilter_get_by_name("buffersink")
		, "out", NULL, NULL, vfilter_ctx.filter_graph) < 0)
	{
		printf("Failed to create video buffer sink\n");
		return -3;
	}

	snprintf(args, sizeof(args), "%d:%d", dst_width, dst_height);

	if (avfilter_graph_create_filter(
		&rescale_filter
		, avfilter_get_by_name("scale")
		, "scale", NULL, args, vfilter_ctx.filter_graph) < 0)
	{
		printf("Failed to create video scale filter\n");
		return -4;
	}

	// link rescaler filter with aformat filter
	if (avfilter_link(outputs->filter_ctx, 0, rescale_filter, 0) < 0)
	{
		printf("Failed to link video format filter\n");
		return -4;
	}

	// aformat is linked with Buffer Sink filter.
	if (avfilter_link(rescale_filter, 0, vfilter_ctx.sink_ctx, 0) < 0)
	{
		printf("Failed to link video format filter\n");
		return -4;
	}

	// Configure all prepared filters.
	if (avfilter_graph_config(vfilter_ctx.filter_graph, NULL) < 0)
	{
		printf("Failed to configure video filter context\n");
		return -5;
	}

	av_buffersink_set_frame_size(vfilter_ctx.sink_ctx, codec_ctx->frame_size);

	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);
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
int main()
{
	// register
	av_register_all();
	avfilter_register_all();

#ifdef _NETWORK
	avformat_network_init();
#endif

	ifmt_ctx = init_input_format(szFilePath);
	a_ifmt_ctx = init_input_format(audio_file);
	outputFormat = find_Format(ifmt_ctx->iformat->name);

	AVCodecContext* V_enc_codec_ctx = NULL;
	AVCodecContext* A_enc_codec_ctx = NULL;

	V_enc_codec_ctx = init_encoder(ifmt_ctx, Video_flag);
	A_enc_codec_ctx = init_encoder(a_ifmt_ctx, Audio_flag);

	AVCodecContext* dec_V_ctx = NULL;
	AVCodecContext* dec_A_ctx = NULL;

	dec_V_ctx = init_decoder(ifmt_ctx, Video_flag);
	dec_A_ctx = init_decoder(a_ifmt_ctx, Audio_flag);

	ofmt_ctx = init_output_format(ifmt_ctx, a_ifmt_ctx, outputfile, V_enc_codec_ctx, A_enc_codec_ctx);

	AVPacket read_pkt;
	AVPacket out_pkt;
	AVFrame* pVFrame;
	AVFrame* pAFrame;

	int bGotPicture = 0;   // flag for video decoding
	int bGotSound = 0;      // flag for audio decoding

	pVFrame = av_frame_alloc();
	pAFrame = av_frame_alloc();

	av_init_packet(&out_pkt);

	// 비디오 필터 설정
	init_video_filter();
	int stream_index;

	AVFrame* filtered_frame = av_frame_alloc();

	if (init_video_filter() < 0)
	{
		return 0;
	}

	if (!OpenImage("testImage.jpg")) {
		return 0;
	}

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
	int i = 0;
	while (av_read_frame(ifmt_ctx, &read_pkt) >= 0) {
		///> Decoding
		//av_read_frame(a_ifmt_ctx, &apkt);
		if (read_pkt.stream_index == nVSI) {
			if (avcodec_decode_video2(dec_V_ctx, pVFrame, &bGotPicture, &read_pkt) >= 0) {
				if (bGotPicture) {
					FilterContext* filter_ctx = &vfilter_ctx;
					out_pkt.data = NULL;    // packet data will be allocated by the encoder
					out_pkt.size = 0;
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
					} // while
					  //효과 필터 적용 시점
					int start = 5 * ofmt_ctx->streams[0]->codec->time_base.den;
					int end = 10 * ofmt_ctx->streams[0]->codec->time_base.den;
					if (i < start) {
						//filtered_frame->pts = av_frame_get_best_effort_timestamp(pVFrame);
						avcodec_encode_video2(V_enc_codec_ctx, &out_pkt, filtered_frame, &bGotPicture);

					}
					else if (i > end) {
						//filtered_frame->pts = av_frame_get_best_effort_timestamp(pVFrame);
						avcodec_encode_video2(V_enc_codec_ctx, &out_pkt, filtered_frame, &bGotPicture);

					}
					else {
						m_picture_frame->pts = pVFrame->pts;
						avcodec_encode_video2(V_enc_codec_ctx, &out_pkt, m_picture_frame, &bGotPicture);
					}
					av_frame_unref(filtered_frame);
					if (bGotPicture) {
						cout << "    [pts] => " << out_pkt.pts << " [size]=>" << out_pkt.size << endl;
						av_interleaved_write_frame(ofmt_ctx, &out_pkt);
						i++;
						av_free_packet(&out_pkt);

					}
				}
			}
		}
		av_free_packet(&read_pkt);
	}
	//
	for (bGotPicture = 1; bGotPicture; ) {
		fflush(stdout);
		if (avcodec_encode_video2(V_enc_codec_ctx, &read_pkt, NULL, &bGotPicture) < 0) {
			fprintf(stderr, "Error encoding frame\n");
			exit(1);
		}
		if (bGotPicture) {
			av_interleaved_write_frame(ofmt_ctx, &read_pkt);
			av_free_packet(&read_pkt);
		}
	}
	while (av_read_frame(a_ifmt_ctx, &read_pkt) >= 0) {
		if (read_pkt.stream_index == 0) {
			if (read_pkt.data != NULL) {
				cout << "Audio Write Frame [pts]=>" << out_pkt.pts << "  [size]=>" << out_pkt.size << endl;
				av_interleaved_write_frame(ofmt_ctx, &out_pkt);
				av_free_packet(&out_pkt);
				if (avcodec_decode_audio4(dec_A_ctx, pAFrame, &bGotSound, &read_pkt) >= 0) {
					if (bGotSound) {
						avcodec_encode_audio2(A_enc_codec_ctx, &out_pkt, pAFrame, &bGotSound);
						if (bGotSound) {
							out_pkt.stream_index = 1;
							cout << "Write Frame [dts]=>" << out_pkt.dts << " [pts]=>" << out_pkt.pts << endl;
							av_interleaved_write_frame(ofmt_ctx, &out_pkt);
							av_free_packet(&out_pkt);
						}
					}
				}
			}
		}
		av_free_packet(&read_pkt);
	}
	av_write_trailer(ofmt_ctx);
	avcodec_close(V_enc_codec_ctx);
	av_free(V_enc_codec_ctx);
	av_free(pVFrame);
	av_free(pAFrame);

	///> Close an opened input AVFormatContext. 
	avformat_close_input(&ifmt_ctx);
	avformat_close_input(&ofmt_ctx);

#ifdef _NETWORK
	///> Undo the initialization done by avformat_network_init.
	avformat_network_deinit();
#endif
	return 0;
}