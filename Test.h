#ifndef TEST_H
#define TEST_H

#include "Define.h"
#include "iThread.h"
#include "iEpoll.h"
#include "iSocket.h"

class Test : public iThread, public iEpoll
{
public:
	Test(int nCapacity);
	~Test();

	int Connect(char *pRemoteIp, unsigned short nPort);
	int AddEpollEvents(int init_mode, int behavior);

protected:
	void Run();

	int iRead(int sockfd);
	int iWrite(int sockfd);
	int iError(int sockfd);
	int iClose(int sockfd);
	int iTimeout(int sockfd);

private:
	iSocket *m_socket;

	int m_sockfd;
};

#endif
