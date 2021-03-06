// ConsoleApplication2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Windows.h>

#include <mmintrin.h>
#include <emmintrin.h>

#include <stdlib.h>
#include <assert.h>

#include <fstream>

///////////////////////////

void calculateUsingMmxInt(char* data, unsigned size)
{
	assert(size % 8 == 0);

	__m64 step = _mm_set_pi8(10, 10, 10, 10, 10, 10, 10, 10);
	__m64* dst = reinterpret_cast<__m64*>(data);
	for (unsigned i = 0; i < size; i += 8)
	{
		auto sum = _mm_adds_pi8(step, *dst);
		*dst++ = sum;
	}

	_mm_empty();
}

void calculateUsingSseInt(char* data, unsigned size)
{
	assert(size % 16 == 0);

	__m128i step = _mm_set_epi8(10, 10, 10, 10, 10, 10, 10, 10,
								10, 10, 10, 10, 10, 10, 10, 10);
	__m128i* dst = reinterpret_cast<__m128i*>(data);
	for (unsigned i = 0; i < size; i += 16)
	{
		auto sum = _mm_add_epi8(step, *dst);
		*dst++ = sum;
	}

	//	no need to clear flags like mmx because SSE and FPU can be used at the same time.
}

////////////////////////

void calculateUsingAsmFloat(float* data, unsigned count)
{
	auto singleFloatBytes = sizeof(float);
	auto step = 10.0;
	__asm
	{
			push ecx
			push edx

			mov edx, data
			mov ecx, count
			fld	step	//	fld only accept FPU or Memory

		calcLoop:
			fld [edx]
			fadd st(0), st(1)
			fstp [edx]
			add edx, singleFloatBytes
			dec ecx
			jnz calcLoop

			pop edx
			pop ecx
	}
}

void calculateUsingSseFloat(float* data, unsigned count)
{
	assert(count % 4 == 0);
	assert(sizeof(float) == 4);

	__m128 step = _mm_set_ps(10.0, 10.0, 10.0, 10.0);
	__m128* dst = reinterpret_cast<__m128*>(data);
	for (unsigned i = 0; i < count; i += 4)
	{
		__m128 sum = _mm_add_ps(step, *dst);
		*dst++ = sum;
	}
}

//////////////////////////

LARGE_INTEGER g_counterBegin;
LARGE_INTEGER g_counterEnd;

void beginBenchmark()
{
	if (QueryPerformanceCounter(&g_counterBegin) == 0) {
		fprintf(stderr, "benchmark error:QueryPerformanceCounter");
		exit(-3);
	}
}

void endBenchmark()
{
	if (QueryPerformanceCounter(&g_counterEnd) == 0) {
		fprintf(stderr, "benchmark error:QueryPerformanceCounter");
		exit(-4);
	}
}

void printBenchmarkTime()
{
	LARGE_INTEGER frequency;
	if (QueryPerformanceFrequency(&frequency) == 0) {
		fprintf(stderr, "benchmark error:QueryPerformanceFrequency");
		exit(-5);
	}

	fprintf(stdout, "Benchmark time: %f s\n", (g_counterEnd.QuadPart - g_counterBegin.QuadPart) / (float)frequency.QuadPart);
}

/////////////////////////

void saveToFile(const char* filename, char* data, unsigned size)
{
	std::ofstream outfile(filename);
	outfile.write(data, size);
}

/////////////////////////

int main()
{
	constexpr unsigned intDataSizeInBytes = 10000000;

	auto dataInt = static_cast<char*>(calloc(intDataSizeInBytes, 1));
	if (dataInt == 0) {
		fprintf(stderr, "allocate 10000000-byte memory failed!");
		return -1;
	}

	beginBenchmark();
	calculateUsingMmxInt(dataInt, intDataSizeInBytes);
	endBenchmark();
	printBenchmarkTime();

	///////

	beginBenchmark();
	calculateUsingSseInt(dataInt, intDataSizeInBytes);
	endBenchmark();
	printBenchmarkTime();

	///////////////////////////////////

	constexpr unsigned floatCount = 10000000;
	constexpr unsigned floatDataSizeInBytes = floatCount * sizeof(float);
	auto dataFloat = static_cast<float*>(malloc(floatDataSizeInBytes));
	if (dataFloat == 0) {
		fprintf(stderr, "allocate 10000000-float memory failed!");
		return -1;
	}

	//	init float to 0.0
	float* tmp = reinterpret_cast<float*>(dataFloat);
	for (unsigned i = 0; i < floatCount; ++i)
		*tmp++ = 0.0;

	beginBenchmark();
	calculateUsingAsmFloat(dataFloat, floatCount);
	endBenchmark();
	printBenchmarkTime();

	///////

	beginBenchmark();
	calculateUsingSseFloat(dataFloat, floatCount);
	endBenchmark();
	printBenchmarkTime();

    return 0;
}

