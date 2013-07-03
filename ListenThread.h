#ifndef LISTEN_THREAD_H
#define LISTEN_THREAD_H

#include "Define.h"
#include "iThread.h"
#include "iSocket.h"
#include "TaskThread.h"

class ListenThread : public iThread
{
public:
	ListenThread();
	~ListenThread();

	int Bind();

protected:
	void Run();

private:
	int InitTaskThread();

	int DelTaskThread();

	int GetWorkerCapacity();

	int AssignTask(int sockfd);

private:
	iSocket *m_socket;

	deque<TaskThread *> m_taskthread;

	int m_sockfd;
};

#endif
