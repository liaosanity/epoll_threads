#include "Test.h"

Test::Test(int nCapacity)
: m_sockfd(-1)
, iEpoll(nCapacity)
{
	m_socket = new iSocket();
	if (NULL == m_socket)
	{
		myepollLog(MY_WARNING, "new iSocket() err");

		return;
	}
}

Test::~Test()
{
	close(m_sockfd);

	if (NULL != m_socket)
	{
		delete m_socket;
		m_socket = NULL;
	}
}

void Test::Run()
{
	i_epoll_wait();
}

int Test::Connect(char *pRemoteIp, unsigned short nPort)
{
	return m_socket->Connect(pRemoteIp, nPort, m_sockfd);
}

int Test::AddEpollEvents(int init_mode, int behavior)
{
	return i_epoll_add(m_sockfd, init_mode, behavior);
}

int Test::iRead(int sockfd)
{
	char sBuf[MIN_BUFSZ] = "";

	int bytes = m_socket->recvn(sockfd, sBuf, sizeof(sBuf));
	if (bytes <= 0)
	{
		myepollLog(MY_WARNING, "recv %d err, sockfd=%d", bytes, sockfd);

		return -1;
	}

	myepollLog(MY_DEBUG, "Recv %d bytes, data=%s, sockfd=%d", 
	           bytes, sBuf, sockfd);

	i_epoll_change_mode(sockfd, I_EPOLL_WRITE, I_EPOLL_EDGE_TRIGGERED);

	return 0;
}

int Test::iWrite(int sockfd)
{
	struct timeval t_timeval;
	gettimeofday(&t_timeval, NULL);

	poll(NULL, 0, 1000);

	char sBuf[MIN_BUFSZ] = "";
	sprintf(sBuf, "Hi server, i'am %d %d", sockfd, t_timeval.tv_usec);

	int bytes = m_socket->sendn(sockfd, sBuf, strlen(sBuf));
	if (bytes <= 0)
	{
		myepollLog(MY_WARNING, "send %d err, sockfd=%d", bytes, sockfd);

		return -1;
	}

	i_epoll_change_mode(sockfd, I_EPOLL_READ, I_EPOLL_EDGE_TRIGGERED);

	return 0;
}

int Test::iError(int sockfd)
{
	myepollLog(MY_WARNING, "Get an err(%s) on sockfd %d", strerror(errno), sockfd);

	return -1;
}

int Test::iClose(int sockfd)
{
	myepollLog(MY_DEBUG, "Server is closed. (press CTRL-C to stop)");

	i_epoll_del(sockfd);

	close(sockfd);

	return 0;
}

int Test::iTimeout(int sockfd)
{
	return 0;
}


/**
 * Global handler
 */
struct myServer g_server; // server global state
deque<Test *> g_Test;

/**
 * Low level logging. To use only for very big messages, otherwise
 * myepollLog() is to prefer. 
 */
void myepollLogRaw(int level, const char *msg) 
{
    const char *c = ".-*#";
    int rawmode = (level & MY_LOG_RAW);

    level &= 0xff; // clear flags
    if (level < g_server.verbosity) 
    {
        return;
    }

    FILE * fp = (0 == strcmp(g_server.logfile, "")) 
                ? stdout : fopen(g_server.logfile, "a");
    if (!fp) 
    {
        return;
    }

    if (rawmode) 
    {
        fprintf(fp, "%s", msg);
    } 
    else 
    {
        int off = 0;
        char buf[64] = "";
        struct timeval tv;

        gettimeofday(&tv, NULL);
        off = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S.", 
                       localtime(&tv.tv_sec));
        snprintf(buf+off, sizeof(buf)-off, "%03d", (int)tv.tv_usec/1000);
        
        switch (level)
        {
        case MY_NOTICE:
            fprintf(fp, "%s[%u] %s %c %s%s\n", 
                    ANSI_YELLOW, syscall(__NR_gettid), 
                    buf, c[level], msg, ANSI_RESET);
            break;
            
        case MY_WARNING:
            fprintf(fp, "%s[%u] %s %c %s%s\n", 
                    ANSI_RED, syscall(__NR_gettid), 
                    buf, c[level], msg, ANSI_RESET);
            break;
            
        default:
            fprintf(fp, "[%u] %s %c %s\n", 
                    syscall(__NR_gettid), buf, c[level], msg);
        }
    }
    
    fflush(fp);

    if (0 != strcmp(g_server.logfile, "")) 
    {
        fclose(fp);
    }
}

/**
 * Like myepollLogRaw() but with printf-alike support. This is the funciton that
 * is used across the code. The raw version is only used in order to dump
 * the INFO output on crash. 
 */
void myepollLog(int level, const char *fmt, ...) 
{
    va_list ap;
    char msg[MY_MAX_LOGMSG_LEN] = "";

    if ((level & 0xff) < g_server.verbosity) 
    {
        return;
    }

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    myepollLogRaw(level, msg);
}

static void sigtermHandler(int sig)
{
	myepollLog(MY_NOTICE, "Received SIGTERM, scheduling shutdown >:( ...");

	for (int i=0; i<g_Test.size(); i++)
	{
		Test *pTest = g_Test[i];
		if (NULL != pTest)
		{
			delete pTest;
			pTest = NULL;
		}
	}

	g_Test.clear();

	_exit(EXIT_SUCCESS);
}

static void setupSignalHandlers()
{
	struct sigaction act;
	memset(&act, 0x0, sizeof(act));
	
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sigtermHandler;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);
}

static int GetWorkerCapacity(int threads, int &maxrlimit)
{
	struct rlimit lim;

	getrlimit(RLIMIT_NOFILE, &lim);
	int max = lim.rlim_cur;

	maxrlimit = max;

	int avl = max - (threads + 1);

	return ((avl / 2) / threads);
}

static void help(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [-i ip] [-p port] [-t threads]\n", 
		    argv[0]);
}

int main(int argc, char **argv)
{
	if (7 != argc) 
	{
		help(argc, argv);
		
		return -1;
	}

	int opt = -1;
	char ip[32] = "";
	unsigned short port = 0;
	int threads = 0;
	
	while ((opt = getopt(argc, argv, "i:p:t:")) != -1) 
	{
		switch (opt) 
		{
		case 'i':
			strcpy(ip, optarg);
			break;
			
		case 'p':
			port = strtoul(optarg, NULL, 0);
			break;

		case 't':
			threads = strtoul(optarg, NULL, 0);
			break;

		default:
			help(argc, argv);
			
			return -1;
		}
	}
	
	memset(&g_server, 0x00, sizeof(struct myServer));

	signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
	setupSignalHandlers();

	int maxrlimit = 0;
	int nCapacity = GetWorkerCapacity(threads, maxrlimit);

	for (int i=0; i<threads; i++)
	{
		Test *pTest = new Test(nCapacity);
		if (NULL == pTest)
		{
			myepollLog(MY_WARNING, "new Test() err");
			
			continue;
		}

		g_Test.push_back(pTest);

		pTest->Create();

		if (-1 == pTest->Connect(ip, port))
		{
			myepollLog(MY_WARNING, "[%d]Connect server[%s:%d] err", i, ip, port);
			
			kill(getpid(), SIGTERM);
		}

		pTest->AddEpollEvents(I_EPOLL_READ, I_EPOLL_EDGE_TRIGGERED);
	}

	myepollLog(MY_NOTICE, "Epoll clients is running :) ... (press CTRL-C to stop)");

	poll(NULL, 0, -1);

	return 0;
}
