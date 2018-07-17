#include "ImageProcessing.h"
#include "Utilities.h"

#include <malloc.h> // aligned_malloc
#include <new> // placement new

#pragma warning(disable: 4324) // padding Mat3

// channels
enum
{
	RED = 0,
	GREEN = 1,
	BLUE = 2
};

struct ImageFilter::Impl
{
	ImageFilter::Impl(Mat3<PRECISION>& kernel, unsigned char* src, unsigned char* dst, int x, int y, int n, bool up, bool down)
		: kernel_(kernel) // by val
		, src_(src)
		, dst_(dst)
		, x_(x)
		, y_(y)
		, n_(n)
		, up_(up)
		, down_(down)
	{
	}
	unsigned char* dst_ = nullptr;
	unsigned char* src_ = nullptr;
	int x_;
	int y_;
	int n_;
	bool up_; // allow accessing the line above, on all lines (assuming 3x3 kernel)
	bool down_; // allow accessing the line below, on all lines (assuming 3x3 kernel)
	Mat3<PRECISION> kernel_;
	Timer timer_;
};

ImageFilter::ImageFilter(Mat3<PRECISION>& kernel, unsigned char* src, unsigned char* dst, int x, int y, int n, bool up, bool down)
{
	void* newObj = _aligned_malloc(sizeof(Impl), 16);
	pImpl = new(newObj) Impl(kernel, src, dst, x, y, n, up, down);
}

ImageFilter::~ImageFilter()
{
	_aligned_free(pImpl);
}

double ImageFilter::ElapsedMS() const
{
	return pImpl->timer_.CountMS();
}

//////////////////////////////////////////////////////////////////////////
// naive convolution implementation

template <typename TSrc, typename TDest>
inline void addmul3(TSrc* src, TDest s, TDest* dst)
{
	dst[RED] += s * (TDest)(src[RED]);
	dst[GREEN] += s * (TDest)(src[GREEN]);
	dst[BLUE] += s * (TDest)(src[BLUE]);
}

void RunFilterSimple(ImageFilter::Impl* ctx)
{
	// local copies
	int x_ = ctx->x_;
	int y_ = ctx->y_;
	int n_ = ctx->n_;
	bool up_ = ctx->up_, down_ = ctx->down_;
	Mat3<PRECISION> kernel = ctx->kernel_;
	unsigned char *s = ctx->src_, *d = ctx->dst_;
	bool go_up, go_down;
	const int stride_bytes = n_ * x_;
	PRECISION r[3] = { 0 };

	for (int y = 0; y < y_; ++y)
	{
		go_up = (y > 0 || up_);
		go_down = (y < y_ - 1 || down_);
		for (int x = 0; x < x_; ++x)
		{
			// 2d convolution with KERNEL, on all 3 channels
			// channel is manually unrolled
			// using "nearest-neighbor" for edges
			// left
			addmul3((x > 0) ? (s - n_) : s, kernel[5], r);
			// center
			addmul3(s, kernel[4], r);
			// right
			addmul3((x < x_ - 1) ? (s + n_) : s, kernel[3], r);
			// up
			addmul3(go_up ? (s - stride_bytes) : s, kernel[7], r);
			// down
			addmul3(go_down ? (s + stride_bytes) : s, kernel[1], r);

			d[RED] =   (unsigned char)CLAMP(r[RED], 0, 255);
			d[GREEN] = (unsigned char)CLAMP(r[GREEN], 0, 255);
			d[BLUE] =  (unsigned char)CLAMP(r[BLUE], 0, 255);

			s += n_;
			d += n_;
			r[RED] = r[GREEN] = r[BLUE] = 0;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void RunFilter(void* context)
{
	ImageFilter* image_filter = (ImageFilter*)context;
	ImageFilter::Impl* ctx = image_filter->pImpl;
	
	ctx->timer_.StartCounter();
	RunFilterSimple(ctx);
	ctx->timer_.StopCounter();
}
