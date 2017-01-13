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
	AVFilterGraph* filter_graph;
	AVFilterContext* src_ctx;
	AVFilterContext* sink_ctx;
} FilterContext;

typedef struct _Codec_Set {
	AVCodec *codec = NULL;
	AVCodecContext *V_codec_ctx = NULL;
	AVCodecContext *A_codec_ctx = NULL;
} Codec_Set;

AVInputFormat		*ifmt = NULL;
AVOutputFormat		*ofmt = NULL;
AVFormatContext		*ifmt_ctx = NULL;
AVFormatContext		*ofmt_ctx = NULL;

int nVSI = -1;
int nASI = -1;

AVCodecID			v_codec_id	= AV_CODEC_ID_H264;
AVCodecID			a_codec_id	= AV_CODEC_ID_MP2;

const char			*szFilePath = "./Test_Video/Test_Video.mp4";
const char			*outputfile = "./Test_Video/output.mp4";

static FilterContext vfilter_ctx, afilter_ctx;

static AVFormatContext* init_input_format(const char* input_file_path) {

	AVCodec			*codec			= NULL;
	AVCodecContext	*c				= NULL;
	AVFormatContext	*p_ifmt_ctx		= NULL;
	nVSI = -1;
	nASI = -1;

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
			nVSI = i;
		}
		else if (nASI < 0 && p_ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			nASI = i;
		}
	}

	if (nVSI < 0 && nASI < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "No Video & Audio Streams were Found\n");
		return 0;
	}

	return p_ifmt_ctx;
}

static AVFormatContext* init_output_format(AVFormatContext* p_ifmt_ctx, const char* output_file_path, AVCodecContext* enc_codec_ctx) {
	AVOutputFormat	*p_ofmt		= NULL;
	AVFormatContext	*p_ofmt_ctx = NULL;

	avformat_alloc_output_context2(&p_ofmt_ctx, NULL, NULL, outputfile);
	if (!p_ofmt_ctx)
	{
		fprintf(stderr, "Could not create output context\n");
	}

	p_ofmt = p_ofmt_ctx->oformat;

	int ret = 0;
	for (int i = 0; i < p_ifmt_ctx->nb_streams; i++)
	{
		AVStream *in_stream		= p_ifmt_ctx->streams[i];
		AVStream *out_stream	= avformat_new_stream(p_ofmt_ctx, in_stream->codec->codec);

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

static AVCodecContext* init_encoder(AVFormatContext* p_ifmt_ctx, AVCodecID V_Codec_ID, AVCodecID A_Codec_ID) {

	AVCodec			*codec	= NULL;
	AVCodecContext	*c		= NULL;

	codec = avcodec_find_encoder(v_codec_id);
	if (!codec)
	{
		fprintf(stderr, "Codec not found\n");
		return 0;
	}

	c = avcodec_alloc_context3(codec);
	if (!c)
	{
		fprintf(stderr, "Could not allocate video codec context\n");
		return 0;
	}

	// Example
	c->profile		= FF_PROFILE_H264_BASELINE;
	c->width		= p_ifmt_ctx->streams[nVSI]->codec->width;
	c->height		= p_ifmt_ctx->streams[nVSI]->codec->height;
	c->pix_fmt		= p_ifmt_ctx->streams[nVSI]->codec->pix_fmt;
	c->time_base	= { 1,25 };
	c->bit_rate		= 50000;

	if (avcodec_open2(c, codec, NULL) < 0)
	{
		fprintf(stderr, "Could not open codec\n");
		return 0;
	}

	return c;
}

static AVCodecContext* init_decoder(AVFormatContext *p_ifmt_ctx, int flag) {
	AVCodecContext *pVCtx = p_ifmt_ctx->streams[nVSI]->codec;
	AVCodecContext *pACtx = p_ifmt_ctx->streams[nASI]->codec;

	for (int i = 0; i < p_ifmt_ctx->nb_streams; i++)
	{
		if (nVSI < 0 && p_ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			nVSI = i;
		}
		else if (nASI < 0 && p_ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			nASI = i;
		}
	}
	if (nVSI < 0 && nASI < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "No Video & Audio Streams were Found\n");
		return 0;
	}

	///> Find Video Decoder
	AVCodec	*pVideoCodec = avcodec_find_decoder(p_ifmt_ctx->streams[nVSI]->codec->codec_id);

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

	///> Find Audio Decoder
	AVCodec *pAudioCodec = avcodec_find_decoder(p_ifmt_ctx->streams[nASI]->codec->codec_id);

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

	if (flag) {
		return pACtx;
	}
	else {
		return pVCtx;
	}
}

static int init_video_filter()
{
	AVStream		*stream		= ifmt_ctx->streams[0];
	AVCodecContext	*codec_ctx	= stream->codec;

	AVFilterContext	*rescale_filter;
	AVFilterContext	*rotate_filter;
	AVFilterInOut	*inputs, *outputs;
	char args[512];

	vfilter_ctx.filter_graph	= NULL;
	vfilter_ctx.src_ctx			= NULL;
	vfilter_ctx.sink_ctx		= NULL;

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

	snprintf(args, sizeof(args), "%f/%f", 0.1, 0.4);

	if (avfilter_graph_create_filter(
		&rescale_filter
		, avfilter_get_by_name("negate")
		, "F_Name", NULL, NULL, vfilter_ctx.filter_graph) < 0)
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

int main()
{
	// register
	av_register_all();
	avfilter_register_all();

#ifdef _NETWORK
	avformat_network_init();
#endif

	ifmt_ctx = init_input_format(szFilePath);

	AVCodecContext* enc_codec_ctx	= init_encoder(ifmt_ctx, v_codec_id, a_codec_id);

	AVCodecContext* dec_V_ctx		= init_decoder(ifmt_ctx, Video_flag);
	AVCodecContext* dec_A_ctx		= init_decoder(ifmt_ctx, Audio_flag);

	ofmt_ctx = init_output_format(ifmt_ctx, outputfile, enc_codec_ctx);

	AVPacket pkt, outpkt;
	AVFrame* pVFrame, *pAFrame;

	int bGotPicture = 0;	// flag for video decoding
	int bGotSound	= 0;      // flag for audio decoding

	pVFrame = av_frame_alloc();
	pAFrame = av_frame_alloc();

	av_init_packet(&outpkt);

	// 비디오 필터 설정
	init_video_filter();
	int stream_index;

	int ret = 0;
	int i	= 0;
	AVFrame* filtered_frame = av_frame_alloc();
	while (av_read_frame(ifmt_ctx, &pkt) >= 0)
	{
		///> Decoding
		if (pkt.stream_index == nVSI)
		{
			if (avcodec_decode_video2(dec_V_ctx, pVFrame, &bGotPicture, &pkt) >= 0)
			{
				FilterContext* filter_ctx;

				stream_index = pkt.stream_index;

				if (stream_index == nVSI)
				{
					filter_ctx = &vfilter_ctx;
				}
				else
				{
					filter_ctx = &afilter_ctx;
				}

				// put frame into filter.
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

				if (bGotPicture)
				{
					ret = avcodec_encode_video2(enc_codec_ctx, &outpkt, filtered_frame, &bGotPicture);
					if (bGotPicture)
					{
						cout << "Video Write Frame [pts]=>" << outpkt.pts << "  [size]=>" << outpkt.size << endl;
						av_interleaved_write_frame(ofmt_ctx, &outpkt);
						av_free_packet(&outpkt);
					}
				}
			}
		}
		else if (pkt.stream_index == nASI)
		{
			if (pkt.data != NULL)
			{
				if (avcodec_decode_audio4(dec_A_ctx, pAFrame, &bGotSound, &pkt) >= 0)
				{
					if (bGotSound)
					{
						//ret = avcodec_encode_audio2(pACtx, &outpkt, pAFrame, &bGotSound);
						if (bGotSound)
						{
							cout << "Audio Write Frame [pts]=>" << pkt.pts << "  [size]=>" << pkt.size << endl;
							av_interleaved_write_frame(ofmt_ctx, &pkt);
							av_free_packet(&outpkt);
						}
					}
				}
			}
		}
		av_free_packet(&pkt);
	}

	for (bGotPicture = 1; bGotPicture; i++)
	{
		fflush(stdout);
		ret = avcodec_encode_video2(enc_codec_ctx, &pkt, NULL, &bGotPicture);
		if (ret < 0)
		{
			fprintf(stderr, "Error encoding frame\n");
			exit(1);
		}
		if (bGotPicture)
		{
			cout << "Video Write Frame [pts]=>" << pkt.pts << "  [size]=>" << pkt.size << "\t\ttrailer" << endl;
			av_interleaved_write_frame(ofmt_ctx, &pkt);
			av_free_packet(&pkt);
		}
	}

	av_write_trailer(ofmt_ctx);
	avcodec_close(enc_codec_ctx);
	av_free(enc_codec_ctx);
	av_free(pVFrame);
	av_free(pAFrame);

	///> Close an opened input AVFormatContext. 
	avformat_close_input(&ifmt_ctx);
	avformat_close_input(&ofmt_ctx);

#ifdef _NETWORK
	///> Undo the initialization done by avformat_network_init.
	avformat_network_deinit();
#endif
	system("pause");
	return 0;
}