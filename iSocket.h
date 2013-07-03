#ifndef I_SOCKET_H
#define I_SOCKET_H

#include "Define.h"

class iSocket
{
public:
	iSocket();
	~iSocket();

	int Bind(char *bindaddr, unsigned short nPort, int &sockfd);

	int Listen(int lsockfd, int &sockfd);

	int Connect(char *pRemoteIp, unsigned short nPort, int &sockfd);

	int setNonBlock(int sockfd);

	int sendn(int sockfd, const void *pStr, unsigned int nLen);

	int recvn(int sockfd, void *pStr, unsigned int nLen);

private:
};

#endif
