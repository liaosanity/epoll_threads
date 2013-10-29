#include "TaskThread.h"

TaskThread::TaskThread(int nCapacity) 
: iEpoll(nCapacity)
{
	m_ActiveConns = {0};

	m_socket = new iSocket();
	if (NULL == m_socket)
	{
		myepollLog(MY_WARNING, "new iSocket() err");

		return;
	}
}

TaskThread::~TaskThread()
{
	if (NULL != m_socket)
	{
		delete m_socket;
		m_socket = NULL;
	}
}

void TaskThread::Run()
{
	i_epoll_wait();
}

int TaskThread::GetActiveConns()
{
	return m_ActiveConns.counter;
}

void TaskThread::IncrementActiveConns()
{
	atomic_inc(&m_ActiveConns);
}

int TaskThread::iRead(int sockfd)
{
	char sBuf[MIN_BUFSZ] = "";

	int bytes = m_socket->recvn(sockfd, sBuf, sizeof(sBuf));
	if (bytes <= 0)
	{
		myepollLog(MY_WARNING, "recv %d err, sockfd=%d", bytes, sockfd);

		return -1;
	}

	myepollLog(MY_DEBUG, "Recv %d bytes, data=%s", bytes, sBuf);

	i_epoll_change_mode(sockfd, I_EPOLL_WRITE, I_EPOLL_EDGE_TRIGGERED);

	return 0;
}

int TaskThread::iWrite(int sockfd)
{
	struct timeval t_timeval;
	gettimeofday(&t_timeval, NULL);

	char sBuf[MIN_BUFSZ] = "";
	sprintf(sBuf, "Hi %d welcome %d", sockfd, (int)t_timeval.tv_usec);

	int bytes = m_socket->sendn(sockfd, sBuf, strlen(sBuf));
	if (bytes <= 0)
	{
		myepollLog(MY_WARNING, "send %d err, sockfd=%d", bytes, sockfd);

		return -1;
	}

	i_epoll_change_mode(sockfd, I_EPOLL_READ, I_EPOLL_EDGE_TRIGGERED);

	return 0;
}

int TaskThread::iError(int sockfd)
{
	myepollLog(MY_WARNING, "Get an err(%s) on sockfd %d", 
	           strerror(errno), sockfd);

	return -1;
}

int TaskThread::iClose(int sockfd)
{
	myepollLog(MY_DEBUG, "Client %d is closed", sockfd);

	i_epoll_del(sockfd);

	close(sockfd);

	atomic_dec(&m_ActiveConns);

	myepollLog(MY_DEBUG, "The totally of ActiveConns=%d", 
	           m_ActiveConns.counter);

	return 0;
}

int TaskThread::iTimeout(int sockfd)
{
	return 0;
}
