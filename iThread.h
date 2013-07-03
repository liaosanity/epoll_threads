#ifndef I_THREAD_H
#define I_THREAD_H

#include "Define.h"

class iThread
{
public:
	iThread();
	virtual ~iThread();

	pthread_t GetThreadID();

	int Create();

	void Suspend();

	void Resume();

protected:
	virtual void Run() = 0;

private:
	void MaskSIGUSR1();

	static void *ThreadFun(void *arg);

private:
	pthread_t m_pid;

	sigset_t m_waitSig;
};

#endif
