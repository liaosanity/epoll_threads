#ifndef I_EPOLL_H
#define I_EPOLL_H

#include "Define.h"

class iEpoll
{
public:
	iEpoll(int nCapacity);
	virtual ~iEpoll();

	int i_epoll_create(int nCapacity);
	int i_epoll_wait();
	int i_epoll_add(int sockfd, int mode, int behavior);
	int i_epoll_del(int sockfd); 
	int i_epoll_change_mode(int sockfd, int mode, int behavior);

protected:
	virtual int iRead(int sockfd) = 0;
	virtual int iWrite(int sockfd) = 0;
	virtual int iError(int sockfd) = 0;
	virtual int iClose(int sockfd) = 0;
	virtual int iTimeout(int sockfd) = 0;

private:
	int m_epfd;

	int m_maxevents;

	struct epoll_event *m_events;
};

#endif
