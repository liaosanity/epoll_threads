#include "iEpoll.h"

iEpoll::iEpoll(int nCapacity) 
: m_epfd(-1)
, m_maxevents(nCapacity)
{
	if (-1 == i_epoll_create(nCapacity))
	{
		return;
	}

	m_events = new struct epoll_event[m_maxevents];
	if (NULL == m_events)
	{
		myepollLog(MY_WARNING, "new struct epoll_event[%d] err", m_maxevents);

		return;
	}
	
	memset(m_events, 0x00, m_maxevents);
}

iEpoll::~iEpoll()
{
	close(m_epfd);

	if (NULL != m_events)
	{
		delete []m_events;
		m_events = NULL;
	}
}

int iEpoll::i_epoll_create(int nCapacity)
{
	m_epfd = epoll_create(nCapacity);
	if (-1 == m_epfd)
	{
		myepollLog(MY_WARNING, "epoll_create() err");

		return -1;
	}

	return 0;
}

int iEpoll::i_epoll_wait()
{
	int i = 0;
	int sockfd = -1;
	int num_fds = 0;
	int nRet = -1;

	while (1) 
	{
		num_fds = epoll_wait(m_epfd, m_events, m_maxevents, I_EPOLL_WAIT_TIMEOUT);

		for (i=0; i<num_fds; i++) 
		{
			sockfd = m_events[i].data.fd;

			if (m_events[i].events & EPOLLIN) 
			{
				//myepollLog(MY_DEBUG, "EPoll Event READ sockfd=%d", sockfd);

				nRet = iRead(sockfd);
			}
			else if (m_events[i].events & EPOLLOUT) 
			{
				//myepollLog(MY_DEBUG, "EPoll Event WRITE sockfd=%d", sockfd);

				nRet = iWrite(sockfd); 
			}
			else if (m_events[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) 
			{
				//myepollLog(MY_DEBUG, "EPoll Event EPOLLHUP/EPOLLER sockfd=%d", sockfd);

				nRet = iError(sockfd);
			}

			if (nRet < 0) 
			{
				//myepollLog(MY_DEBUG, "Client %d is closed, nRet=%d", sockfd, nRet);

				iClose(sockfd);
			}
		}
	}

	return 0;
}

int iEpoll::i_epoll_add(int sockfd, int init_mode, int behavior)
{
	struct epoll_event ev;
	memset(&ev, 0x00, sizeof(struct epoll_event));
	ev.data.fd = sockfd;
	ev.events = EPOLLERR | EPOLLHUP | EPOLLRDHUP;

	if (I_EPOLL_EDGE_TRIGGERED == behavior) 
	{
		ev.events |= EPOLLET;
	}

	switch (init_mode) 
	{
	case I_EPOLL_READ:
		ev.events |= EPOLLIN;
		break;

	case I_EPOLL_WRITE:
		ev.events |= EPOLLOUT;
		break;

	case I_EPOLL_RW:
		ev.events |= EPOLLIN | EPOLLOUT;
		break;
	}

	int nRet = epoll_ctl(m_epfd, EPOLL_CTL_ADD, sockfd, &ev);
	if (nRet < 0) 
	{
		myepollLog(MY_WARNING, "epoll_ctl() err, sockfd=%d", sockfd);
	}

	return nRet;
}

int iEpoll::i_epoll_del(int sockfd)
{
	int nRet = epoll_ctl(m_epfd, EPOLL_CTL_DEL, sockfd, NULL);
	if (nRet < 0) 
	{
		myepollLog(MY_WARNING, "epoll_ctl() err, sockfd=%d", sockfd);
	}

	return nRet;
}

int iEpoll::i_epoll_change_mode(int sockfd, int mode, int behavior)
{
	struct epoll_event ev;
	memset(&ev, 0x00, sizeof(struct epoll_event));
	ev.data.fd = sockfd;
	ev.events = EPOLLERR | EPOLLHUP;

	if (I_EPOLL_EDGE_TRIGGERED == behavior) 
	{
		ev.events |= EPOLLET;
	}

	switch (mode) 
	{
	case I_EPOLL_READ:
		//myepollLog(MY_DEBUG, "EPoll changing mode to READ, sockfd=%d", sockfd);

		ev.events |= EPOLLIN;
		break;

	case I_EPOLL_WRITE:
		//myepollLog(MY_DEBUG, "EPoll changing mode to WRITE, sockfd=%d", sockfd);

		ev.events |= EPOLLOUT;
		break;

	case I_EPOLL_RW:
		//myepollLog(MY_DEBUG, "Epoll changing mode to READ/WRITE, sockfd=%d", sockfd);

		ev.events |= EPOLLIN | EPOLLOUT;
		break;
	}

	int nRet = epoll_ctl(m_epfd, EPOLL_CTL_MOD, sockfd, &ev);
	if (nRet < 0) 
	{
		myepollLog(MY_WARNING, "epoll_ctl() err, sockfd=%d", sockfd);
	}

	return nRet;
}
