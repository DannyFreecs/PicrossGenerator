#ifndef PIXEL_H
#define PIXEL_H

struct Pixel
{
	int R;
	int G;
	int B;
	
	Pixel(int r, int g, int b) : R(r), G(g), B(b) {}
	Pixel(){}
	
	Pixel& operator=(const Pixel& other)
	{
		R = other.R;
		G = other.G;
		B = other.B;
		
		return *this;
	}
};

#endif