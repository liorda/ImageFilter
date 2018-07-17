#include <stdio.h>
#include <assert.h>
#include <thread> // for thread::hardware_concurrency
#include <vector>

#include "Utilities.h"
#include "ImageProcessing.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static constexpr const char* FILENAME = "input image.jpg";
//static constexpr const char* FILENAME = "lenna.png";

int main(int /*argc*/, char* /*argv*/[])
{
	int result = 0;	
	// load image and create destination buffer
	int img_x, img_y, img_n;
	unsigned char* img = stbi_load(FILENAME, &img_x, &img_y, &img_n, 0);
	if (img != NULL)
	{
		printf("input image \"%s\" loaded\n", FILENAME);
	}
	else
	{
		fprintf(stderr, "Could not load input file %s\n", FILENAME);
		exit(-2);
	}
	assert(img_n == 3);
	unsigned char* img_dst = new unsigned char[img_x * img_y * img_n];
	memset(img_dst, 101, img_x * img_y * img_n); // debug / background color

	// note that the convolution assumes the corners are zeros. simple to generalize
	// to any kernel
	static Mat3<PRECISION> SHARPEN_KERNEL = {
		0, -1, 0,
		-1, 5, -1,
		0, -1, 0
	};
// 	static Mat3 LAPLACE = {
// 		0, -2, 0,
// 		-2, 8, -2,
// 		0, -2, 0
// 	};

	// create contexts
	unsigned int thread_count = std::thread::hardware_concurrency();
	std::vector<Thread*> threads;
	std::vector<ImageFilter*> contexts;
	unsigned int rows_per_worker = img_y / thread_count;
	for (unsigned int i=0; i<thread_count; ++i)
	{
		unsigned int initial_pixel_byte = rows_per_worker * i * img_x * img_n;
		// last worker gets the reminder to overcome situations where the number of
		// workers does not fully divide the number of rows:
		if (i == thread_count - 1)
			rows_per_worker += (img_y % thread_count);
		bool up = (i != 0);
		bool down = (i != thread_count - 1);
		unsigned char* src = img + initial_pixel_byte;
		unsigned char* dst = img_dst + initial_pixel_byte;
		auto* context = new ImageFilter(SHARPEN_KERNEL, src, dst, img_x, rows_per_worker, img_n, up, down);
		contexts.push_back(context);
		threads.push_back(new Thread(RunFilter, context));
	}

	// timestamp
	Timer timer;
	timer.StartCounter();
	
	// run
	for (auto thread : threads)
		thread->Start();

	// wait for all to finish
	for (unsigned int i = 0; i < thread_count; ++i)
	{
		threads[i]->Join();
		printf("worker %d time %.2fms\n", i, contexts[i]->ElapsedMS());
	}

	timer.StopCounter();
	printf("total time %.2fms\n", timer.CountMS());

	// save image
	char dst_filename[2048] = { 0 };
	sprintf_s(dst_filename, "sharp_%s", FILENAME);
	//int ret = stbi_write_png(dst_filename, img_x, img_y, img_n, img_dst, img_x * img_n);
	int ret = stbi_write_jpg(dst_filename, img_x, img_y, img_n, img_dst, 100);
	if (ret != 0)
	{
		printf("result written to \"%s\"\n", dst_filename);
	}
	else
	{
		fprintf(stderr, "Could not write output file %s\n", dst_filename);
		result = 1;
	}

	// cleanup
	stbi_image_free(img);
	delete[] img_dst;
	for (auto w : threads)
		delete w;
	for (auto c : contexts)
		delete c;

	return result;
}
