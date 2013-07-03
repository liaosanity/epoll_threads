#include "iSocket.h"

iSocket::iSocket()
{
}

iSocket::~iSocket()
{
}

int iSocket::setNonBlock(int sockfd)
{
	int opts = fcntl(sockfd, F_GETFL);
	if (-1 == opts)
	{
		myepollLog(MY_WARNING, "fcntl err");

		return -1;
	}

	opts = opts | O_NONBLOCK;

	if (fcntl(sockfd, F_SETFL, opts) < 0)
	{
		myepollLog(MY_WARNING, "fcntl err");

		return -1;
	}

	return 0;
}

int iSocket::Bind(char *bindaddr, unsigned short nPort, int &sockfd)
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd)
	{
		myepollLog(MY_WARNING, "Create Socket err");

		return -1;
	}

	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(nPort);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (bindaddr && inet_aton(bindaddr, &servaddr.sin_addr) == 0) 
	{
        myepollLog(MY_WARNING, "Invalid bind address(%s)", bindaddr);
        
        close(sockfd);
        
        return -1;
    }

	int nRet = bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if (-1 == nRet)
	{
		myepollLog(MY_WARNING, "Bind err");
		
		close(sockfd);

		return -1;
	}

	nRet = listen(sockfd, LISTEN_POOL_SZ);
	if (-1 == nRet)
	{
		myepollLog(MY_WARNING, "Listen err");
		
		close(sockfd);

		return -1;
	}

	return 0;
}

int iSocket::Listen(int lsockfd, int &sockfd)
{
	struct sockaddr_in cliaddr;
	socklen_t sock_len = sizeof(struct sockaddr_in);

	while (1)
	{
		bzero(&cliaddr, sizeof(cliaddr));

		sockfd = accept(lsockfd, (struct sockaddr *)&cliaddr, &sock_len);
		if (-1 == sockfd)
		{
		    if (errno == EINTR)
		    {
		        continue;
		    }
		    else
		    {
		        myepollLog(MY_WARNING, "accept() err(%s)", strerror(errno));
		        
		        return -1;
		    }
		}
		
		int rcvbufsize = 12288000;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbufsize, sizeof(int));
        
        int sndbufsize = 12288000;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbufsize, sizeof(int));

        struct linger ling1 = {1, 0};
		setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &ling1, sizeof(ling1));

		setNonBlock(sockfd);

		myepollLog(MY_DEBUG, "New client[%s:%d] is coming", 
		           inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

		return 0;
	}
}

int iSocket::Connect(char *pRemoteIp, unsigned short nPort, int &sockfd)
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd)
	{
		myepollLog(MY_WARNING, "Create Socket err");

		return -1;
	}

	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	
	int rcvbufsize = 12288000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbufsize, sizeof(int));
    
    int sndbufsize = 12288000;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbufsize, sizeof(int));

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(nPort);
	servaddr.sin_addr.s_addr = inet_addr(pRemoteIp);

	int nRet = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if (nRet < 0) 
	{
		myepollLog(MY_WARNING, "connect() err");

		return -1;
	}

	struct linger ling1 = {1, 0};
	setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &ling1, sizeof(ling1));

	setNonBlock(sockfd);

	return 0;
}

int iSocket::sendn(int sockfd, const void *pStr, unsigned int nLen)
{
	int n = nLen, nRet = 0;
	char *str = (char *)pStr;

	while (n > 0)
	{
		nRet = send(sockfd, str, n, MSG_NOSIGNAL);
		if (nRet <= 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			return -1;
		}

		n -= nRet;
		str += nRet;
	}

	return (nLen - n);
}

int iSocket::recvn(int sockfd, void *pStr, unsigned int nLen)
{
	int n = nLen;
	char *str = (char *)pStr;

	while (n > 0)
	{
		int nRet = recv(sockfd, str, n, MSG_NOSIGNAL);
		if (nRet <= 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			break;
		}

		n -= nRet;
		str += nRet;
	}

	return (nLen - n);
}
