#include "stdafx.h"
#include "RGBImage.h"
RGBImage::RGBImage() {

}
RGBImage::RGBImage(int width, int height) {
	this->width = width;
	this->height = height;
}
RGBImage::~RGBImage() {
	delete[]Rdata;
	delete[]Gdata;
	delete[]Bdata;
	delete[]Adata;
}
void RGBImage::copyFrame(AVFrame* frame) {
	width = frame->width;
	height = frame->height;
	int H = frame->height;
	int W = frame->linesize[0];
	Rdata = new u_char[W*H];
	Gdata = new u_char[W*H];
	Bdata = new u_char[W*H];
	int Y, U, V, A;
	for (int i = 0; i < H; i++) {
		for (int j = 0; j < W; j++) {
			Y = frame->data[0][i*W + j];
			U = frame->data[1][((i / 2)*(W / 2) + j / 2)];
			V = frame->data[2][((i / 2)*(W / 2) + j / 2)];
			//A = frame->data[3][((i / 2)*(W / 2) + j / 2)];
			int R, G, B, A;
			//Rdata[i*W + j] = frame->data[0][i*W + j] + (1.370705 * (frame->data[2][((i / 2)*(W / 2) + j / 2)] - 128));
			//Gdata[i*W + j] = frame->data[0][i*W + j] - (0.698001 * (frame->data[2][((i / 2)*(W / 2) + j / 2)] - 128)) - (0.337633 * (frame->data[1][((i / 2)*(W / 2) + j / 2)] - 128));
			//Bdata[i*W + j] = frame->data[0][i*W + j] + (1.732446 * frame->data[1][((i / 2)*(W / 2) + j / 2)]);
			R = (int)(1.164*(Y - 16) + 1.596*(V - 128));
			G = (int)(1.164*(Y - 16) - 0.813*(V - 128) - 0.391*(U - 128));
			B = (int)(1.164*(Y - 16) + 2.018*(U - 128));
			if (R < 0) {
				R = 0;
			}
			else if (R > 255) {
				R = 255;
			}
			if (G < 0) {
				G = 0;
			}
			else if (G > 255) {
				G = 255;
			}
			if (B < 0) {
				B = 0;
			}
			else if (B > 255) {
				B = 255;
			}
			Rdata[i*W + j] = R;
			Gdata[i*W + j] = G;
			Bdata[i*W + j] = B;
			//Rdata[i*W + j] =(int) frame->data[0][i*W + j] + 1.402*frame->data[2][((i / 2)*(W / 2) + j / 2)];
			//Gdata[i*W + j] = frame->data[0][i*W + j] - 0.344*frame->data[1][((i / 2)*(W / 2) + j / 2)] - 0.714*frame->data[2][((i / 2)*(W / 2) + j / 2)];
			//	Bdata[i*W + j] = frame->data[0][i*W + j] + 1.772*frame->data[1][((i / 2)*(W / 2) + j / 2)];

		}
	}
}

void RGBImage::copyFrame_Alpha(AVFrame* frame) {
	width = frame->width;
	height = frame->height;
	int H = frame->height;
	int W = frame->linesize[0];
	Rdata = new u_char[W*H];
	Gdata = new u_char[W*H];
	Bdata = new u_char[W*H];
	Adata = new u_char[W*H];
	int Y, U, V, A;
	for (int i = 0; i < H; i++) {
		for (int j = 0; j < W; j++) {
			Y = frame->data[0][i*W + j];
			U = frame->data[1][((i / 2)*(W / 2) + j / 2)];
			V = frame->data[2][((i / 2)*(W / 2) + j / 2)];
			//A = frame->data[3][((i / 2)*(W / 2) + j / 2)];
			int R, G, B, A;
			R = (int)(1.164*(Y - 16) + 1.596*(V - 128));
			G = (int)(1.164*(Y - 16) - 0.813*(V - 128) - 0.391*(U - 128));
			B = (int)(1.164*(Y - 16) + 2.018*(U - 128));
			A = (int)frame->data[3][i*W + j];
			if (R < 0) {
				R = 0;
			}
			else if (R > 255) {
				R = 255;
			}
			if (G < 0) {
				G = 0;
			}
			else if (G > 255) {
				G = 255;
			}
			if (B < 0) {
				B = 0;
			}
			else if (B > 255) {
				B = 255;
			}
			Rdata[i*W + j] = R;
			Gdata[i*W + j] = G;
			Bdata[i*W + j] = B;
			Adata[i*W + j] = A;
		}
	}
}

bool RGBImage::checkPixelAlpha(int index) {
	if (Adata[index] == 0) {
		return true;
	}
	return false;
}

bool RGBImage::checkPixelWhite(int index) {
	if (Rdata[index] > 100 &&
		Gdata[index] > 100 &&
		Bdata[index] > 100
		)
		return true;
	return false;
}

void RGBImage::getPixelColor(u_char& R, u_char& G, u_char& B, int index) {
	R = Rdata[index];
	G = Gdata[index];
	B = Bdata[index];
}

void RGBImage::getPixelColor_Alpha(u_char& R, u_char& G, u_char& B, u_char& A, int index) {
	R = Rdata[index];
	G = Gdata[index];
	B = Bdata[index];
	A = Adata[index];
}

void RGBImage::setPixelColor(u_char R, u_char G, u_char B, int index) {
	Rdata[index] = R;
	Gdata[index] = G;
	Bdata[index] = B;
}

void RGBImage::setPixelColor_Alpha(u_char& R, u_char& G, u_char& B, u_char& A, int index) {
	Rdata[index] = R;
	Gdata[index] = G;
	Bdata[index] = B;
	Adata[index] = A;
}

void RGBImage::RGB2YUV(AVFrame& frame) {
	int H = height;
	int W = frame.linesize[0];
	int R, G, B, A;

	for (int i = 0; i < H; i++) {
		for (int j = 0; j < W; j++) {
			R = Rdata[i*W + j];
			G = Gdata[i*W + j];
			B = Bdata[i*W + j];
			frame.data[0][i*W + j] = (0.257* R) + (0.504*G) + (0.098*B) + 16;
			frame.data[1][((i / 2)*(W / 2) + j / 2)] = -(0.148*R) - (0.291*G) + (0.439*B) + 128;
			frame.data[2][((i / 2)*(W / 2) + j / 2)] = (0.439*R) - (0.368*G) - (0.071*B) + 128;
		}
	}
}

void RGBImage::RGB2YUV_Alpha(AVFrame& frame) {
	int H = height;
	int W = frame.linesize[0];
	int R, G, B, A;

	for (int i = 0; i < H; i++) {
		for (int j = 0; j < W; j++) {
			R = Rdata[i*W + j];
			G = Gdata[i*W + j];
			B = Bdata[i*W + j];
			A = Adata[i*W + j];
			frame.data[0][i*W + j] = (0.257* R) + (0.504*G) + (0.098*B) + 16;
			frame.data[1][((i / 2)*(W / 2) + j / 2)] = -(0.148*R) - (0.291*G) + (0.439*B) + 128;
			frame.data[2][((i / 2)*(W / 2) + j / 2)] = (0.439*R) - (0.368*G) - (0.071*B) + 128;
			frame.data[3][i*W + j] = A;
		}
	}
}