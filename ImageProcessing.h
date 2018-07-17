#pragma once

using PRECISION = float;

#pragma warning(push)
#pragma warning(disable: 4201 4324)
template <typename T>
struct __declspec(align (16)) Mat3
{
	T v[9];
	inline T operator[](int i) const { return v[i]; }
};
#pragma warning(pop)

struct ImageFilter
{
	// src - the source pixel data buffer
	// dst - the destination pixel data buffer
	// x - width in pixels
	// y - height in pixels
	// n - number of channels
	// up, down - is access to the border line above/below allowed, on all lines (assuming 3x3 kernel)
	ImageFilter(Mat3<PRECISION>& kernel, unsigned char* src, unsigned char* dst, int x, int y, int n, bool up, bool down);
	~ImageFilter();

	double ElapsedMS() const;

	struct Impl;
	Impl* pImpl;
};

// thread function for running the ImageFilter work
// context should be ImageFilter*
void RunFilter(void* context);
