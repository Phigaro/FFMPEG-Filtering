#pragma once
#include "stdafx.h"
typedef uint8_t u_char;
class RGBImage {
private:
	int width;
	int height;
	u_char *Rdata;
	u_char *Gdata;
	u_char *Bdata;
	u_char *Adata;
public:
	RGBImage();
	RGBImage(int width, int height);
	~RGBImage();
	void copyFrame(AVFrame* frame);
	void copyFrame_Alpha(AVFrame* frame);
	bool checkPixelWhite(int index);
	bool checkPixelAlpha(int index);
	void getPixelColor(u_char& R, u_char& G, u_char& B, int index);
	void setPixelColor(u_char R, u_char G, u_char B, int index);

	void getPixelColor_Alpha(u_char& R, u_char& G, u_char& B, u_char& A, int index);
	void setPixelColor_Alpha(u_char& R, u_char& G, u_char& B, u_char& A, int index);

	void RGB2YUV(AVFrame& frame);
	void RGB2YUV_Alpha(AVFrame& frame);
};
