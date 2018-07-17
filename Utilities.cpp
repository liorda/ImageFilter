#include "Utilities.h"
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // required for the high precision timer and win32 threads

//////////////////////////////////////////////////////////////////////////
// Timer

struct Timer::Impl
{
	double PCFreq = 0.0;
	__int64 CounterStart = 0;
	double total_ms = 0.0;

	void StartCounter()
	{
		LARGE_INTEGER li;
		if (!QueryPerformanceFrequency(&li))
			exit(-1);

		PCFreq = double(li.QuadPart) / 1000.0;

		QueryPerformanceCounter(&li);
		CounterStart = li.QuadPart;
	}
	void StopCounter()
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		total_ms = double(li.QuadPart - CounterStart) / PCFreq;
	}
	
	double CountMS() const
	{
		return total_ms;
	}
};

Timer::Timer()
{
	pImpl = new Impl;
}

Timer::~Timer()
{
	delete pImpl;
}

void Timer::StartCounter()
{
	pImpl->StartCounter();
}

void Timer::StopCounter()
{
	pImpl->StopCounter();
}

double Timer::CountMS() const
{
	return pImpl->CountMS();
}

//////////////////////////////////////////////////////////////////////////
// Thread

// this implementation also wraps LPTHREAD_START_ROUTINE with a simple thread
// signature: void (*func)(void*)
struct Thread::Impl
{
	HANDLE hThread = nullptr;
	void(*func_)(void*);
	void* context_;
	Impl(void(*func)(void* context), void* context) : func_(func), context_(context)
	{
		hThread = CreateThread(NULL, 0, WinThreadFunc, this, CREATE_SUSPENDED, NULL);
	}
	~Impl()
	{
		CloseHandle(hThread);
	}
	static DWORD WINAPI WinThreadFunc(LPVOID lpThreadParameter)
	{
		Thread::Impl* wrapper = (Thread::Impl*)lpThreadParameter;
		void* context = wrapper->context_;
		void(*func)(void*) = wrapper->func_;
		func(context);
		return 0;
	}
};

Thread::Thread(void(*func)(void* context), void* context)
{
	pImpl = new Impl(func, context);
}

Thread::~Thread()
{
	delete pImpl;
}


void Thread::Start()
{
	ResumeThread(pImpl->hThread);
}

void Thread::Join()
{
	WaitForSingleObject(pImpl->hThread, INFINITE);
}
