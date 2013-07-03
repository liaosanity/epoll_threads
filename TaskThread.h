#ifndef TASK_THREAD_H
#define TASK_THREAD_H

#include "Define.h"
#include "iThread.h"
#include "iEpoll.h"
#include "iSocket.h"
#include "iAtomic.h"

class TaskThread : public iThread, public iEpoll
{
public:
	TaskThread(int nCapacity);
	~TaskThread();

	int GetActiveConns();

	void IncrementActiveConns();

protected:
	void Run();

	int iRead(int sockfd);
	int iWrite(int sockfd);
	int iError(int sockfd);
	int iClose(int sockfd);
	int iTimeout(int sockfd);

private:
	atomic_t m_ActiveConns;

	iSocket *m_socket;
};

#endif
