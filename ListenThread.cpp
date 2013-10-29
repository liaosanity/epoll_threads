#include "ListenThread.h"

ListenThread::ListenThread() : m_sockfd(-1)
{
	m_socket = new iSocket();
	if (NULL == m_socket)
	{
		myepollLog(MY_WARNING, "new iSocket() err");

		return;
	}

	if (-1 == InitTaskThread())
	{
		return;
	}

	Create();
}

ListenThread::~ListenThread()
{
	close(m_sockfd);
	
	if (NULL != m_socket)
	{
		delete m_socket;
		m_socket = NULL;
	}

	DelTaskThread();
}

void ListenThread::Run()
{
	Suspend();

	int sockfd = -1; 

	while (!g_Server.shutdown)
	{
		if (0 == m_socket->Listen(m_sockfd, sockfd))
		{
			AssignTask(sockfd);
		}
	}
}

int ListenThread::Bind()
{
	if (-1 == m_socket->Bind(g_Server.bindaddr, g_Server.port, m_sockfd))
	{
		myepollLog(MY_WARNING, "Bind port %d err" , g_Server.port);

		return -1;
	}

	Resume();

	return 0;
}

int ListenThread::InitTaskThread()
{
	m_taskthread.clear();

	int nCapacity = GetWorkerCapacity();

	for (int i=0; i<g_Server.maxthreads; i++)
	{
		TaskThread *pTaskThread = new TaskThread(nCapacity);
		if (NULL == pTaskThread)
		{
			myepollLog(MY_WARNING, "new TaskThread() err");

			return -1;
		}

		m_taskthread.push_back(pTaskThread);

		pTaskThread->Create();        
	}

	return 0;
}

int ListenThread::DelTaskThread()
{
	for (int i=0; i<(int)m_taskthread.size(); i++)
	{
		TaskThread *pTaskThread = m_taskthread[i];
		if (NULL != pTaskThread)
		{
			delete pTaskThread;
			pTaskThread = NULL;
		}
	}

	m_taskthread.clear();

	return 0;
}

int ListenThread::GetWorkerCapacity()
{
	struct rlimit lim;

	getrlimit(RLIMIT_NOFILE, &lim);
	int max = lim.rlim_cur;

	/** 
	 * Minimum of fds needed:
	 * 3 fds: stdin, stdout, stderr
	 * 1 fd for main socket server
	 * 1 fd for epoll array (per thread)
	 */
	int avl = max - (3 + 1 + g_Server.maxthreads);

	/** 
	 * The avl is divided by two as we need to consider
	 * a possible additional FD for each plugin working
	 * on the same request.
	 */
	return ((avl / 2) / g_Server.maxthreads);
}

int ListenThread::AssignTask(int sockfd)
{
	int target = 1;
	int activeConns = 1;

	for (int i=0; i<(int)m_taskthread.size(); i++)
	{
	    activeConns += m_taskthread[i]->GetActiveConns();
	    
		if (m_taskthread[i]->GetActiveConns() 
		    < m_taskthread[target]->GetActiveConns())
		{
			target = i;
		}
	}
	
	if (activeConns > g_Server.maxclients)
	{
	    myepollLog(MY_NOTICE, "Max number of clients(%d) reached", 
	               g_Server.maxclients);
	    
	    close(sockfd);
	    
	    return 0;
	}

	int nRet = m_taskthread[target]->i_epoll_add(sockfd,
	                                             I_EPOLL_WRITE, 
	                                             I_EPOLL_EDGE_TRIGGERED);
	if (0 == nRet) 
	{
		m_taskthread[target]->IncrementActiveConns();        
	}
	
	return 0;
}
