#pragma once

// platform independent utilities

#define CLAMP(val, min, max) ( (val)<(max) ? ( (val)>(min) ? (val) : (min) ) : (max) )

struct Timer
{
	Timer();
	~Timer();
	void StartCounter();
	void StopCounter();
	double CountMS() const;

	struct Impl;
	Impl* pImpl;
};

struct Thread
{
	Thread(void(*func)(void* context), void* context);
	~Thread();
	void Start();
	void Join();

	struct Impl;
	Impl* pImpl;
};
