#include "iThread.h"

iThread::iThread()
{
	MaskSIGUSR1();
	
	sigemptyset(&m_waitSig);
	sigaddset(&m_waitSig, SIGUSR1);
}

iThread::~iThread()
{
}

void iThread::MaskSIGUSR1()
{
	sigset_t sig;
	sigemptyset(&sig);
	sigaddset(&sig, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &sig, NULL);
}

pthread_t iThread::GetThreadID()
{
	return m_pid;
}

void *iThread::ThreadFun(void *arg)
{
	iThread *piThread = (iThread*)arg;
	piThread->Run();

	return (void *)0;
}

int iThread::Create()
{
	int iRet = pthread_create(&m_pid, NULL, ThreadFun, this);
	if (0 == iRet)
	{
		iRet = pthread_detach(m_pid);
		if (0 == iRet)
		{
			return 0;
		}
	}

	return -1;
}

void iThread::Suspend()
{
	int sig;
	sigwait(&m_waitSig, &sig);
}

void iThread::Resume()
{
	pthread_kill(m_pid, SIGUSR1);
}
