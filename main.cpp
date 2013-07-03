#include "Define.h"
#include "ListenThread.h"

/**
 * Global vars 
 */
struct myServer g_server; // server global state
static ListenThread *g_ListenThread = NULL;

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

void initServerConfig() 
{
    memset(&g_server, 0x00, sizeof(struct myServer));
    strcpy(g_server.pidfile, "/var/run/redis.pid");
    g_server.port = LISTEN_PORT;
    g_server.verbosity = MY_DEBUG;
    g_server.maxidletime = MY_MAXIDLETIME;
    g_server.maxclients = MY_MAX_CLIENTS;
    g_server.maxthreads = THREAD_POOL_SZ;
}

int loadServerConfig(char *filename) 
{
    if (NULL == filename)
    {
        myepollLog(MY_WARNING, 
                   "Fatal error, config file '%s' is NULL", filename);
        
        return -1;
    }
    
    FILE *fp = fopen(filename, "r");
    if (NULL == fp) 
    {
        myepollLog(MY_WARNING, 
                   "Can't open config file '%s'", filename);
        
        return -1;
    }

    int nRet = 0;
    int i = 0, line = 0;
    char buffer[MY_CONFIGLINE_MAX] = "", *buff = NULL;
    const char *name = NULL;
    char *value = NULL;
           
    while (fgets(buffer, sizeof(buffer), fp))
    {
        buff = buffer;
        
        for (i = 0; i < (int) strlen(buff) - 1 
             && (buff[i] == ' ' || buff[i] == '\t'); i++);
        
        buff += i;
        line++;
        
        if (strlen(buff) <= 2 || buff[0] == '#')
        {
            continue;
        }
        
        if (!(value = strpbrk(buff, " \t"))) 
        {
            myepollLog(MY_WARNING, 
                       "Missing argument for option at line %d", line);
                       
            nRet = -1;
            
			break;
		}
		
		name = buff;
		*value++ = 0;
		
		for (i = 0; i < (int) strlen(value) - 1 
		     && (value[i] == ' ' || value[i] == '\t'); i++);
		
		value += i;
		
		for (i = strlen(value); i >= 1 
		     && (value[i-1] == ' ' || value[i-1] == '\t' || value[i-1] == '\n'); 
		     i--);
		
		if (!i) 
		{
			myepollLog(MY_WARNING, 
			           "Missing argument for option at line %d", line);
            
            nRet = -1;
            
			break;
		}
		
		value[i] = 0;

        if (0 == strcasecmp(name, "daemonize"))
        {
            if (0 == strcasecmp(value, "yes"))
            {
                g_server.daemonize = 1;
            }
            else if (0 == strcasecmp(value, "no")) 
            {
                g_server.daemonize = 0;
            }
            else
            {
                myepollLog(MY_WARNING, 
                           "Argument must be 'yes' or 'no' at line %d", line);
                
                nRet = -1;
            
			    break;
            }
        }
        else if (0 == strcasecmp(name, "pidfile"))
        {
            strcpy(g_server.pidfile, value);
        }
        else if (0 == strcasecmp(name, "port"))
        {
            g_server.port = atoi(value);
            if (g_server.port < 0 || g_server.port > 65535) 
            {
                myepollLog(MY_WARNING, "Invalid port at line %d", line);
                
                nRet = -1;
            
			    break;
            }
        }
        else if (0 == strcasecmp(name, "bind"))
        {
            strcpy(g_server.bindaddr, value);
        }
        else if (0 == strcasecmp(name, "timeout"))
        {
            g_server.maxidletime = atoi(value);
            if (g_server.maxidletime < 0) 
            {
                myepollLog(MY_WARNING, 
                           "Invalid timeout value at line %d", line);
                
                nRet = -1;
            
			    break;
            }
        }
        else if (0 == strcasecmp(name, "loglevel"))
        {
            if (0 == strcasecmp(value, "debug")) 
            {
                g_server.verbosity = MY_DEBUG;
            }
            else if (0 == strcasecmp(value, "verbose")) 
            {
                g_server.verbosity = MY_VERBOSE;
            }
            else if (0 == strcasecmp(value, "notice")) 
            {
                g_server.verbosity = MY_NOTICE;
            }
            else if (0 == strcasecmp(value, "warning"))
            {
                g_server.verbosity = MY_WARNING;
            }
            else 
            {
                myepollLog(MY_WARNING, 
                           "Argument must be debug, notice or warning at line %d", line);
                
                nRet = -1;
            
			    break;
            }
        }
        else if (0 == strcasecmp(name, "logfile"))
        {
            if (0 != strcasecmp(value, "stdout")) 
            {
                strcpy(g_server.logfile, value);
                
                /**
                 * Test if we are able to open the file. The server will not
                 * be able to abort just for this problem later... 
                 */
                FILE *logfp = fopen(g_server.logfile, "a");
                if (NULL == logfp)
                {
                    myepollLog(MY_WARNING, 
                               "Can't open the log file '%s'", g_server.logfile);
                
                    nRet = -1;
            
			        break;
                }
                
                fclose(logfp);
            }
        }
        else if (0 == strcasecmp(name, "maxclients"))
        {
            g_server.maxclients = atoi(value);
            if (g_server.maxclients < 1) 
            {
                myepollLog(MY_WARNING, 
                           "Invalid max clients limit at line %d", line);
                
                nRet = -1;
            
			    break;
            }
        }
        else if (0 == strcasecmp(name, "maxthreads"))
        {
            g_server.maxthreads = atoi(value);
            if (g_server.maxthreads < 1) 
            {
                myepollLog(MY_WARNING, 
                           "Invalid max threads limit at line %d", line);
                
                nRet = -1;
            
			    break;
            }
        }
        else
        {
            myepollLog(MY_NOTICE, 
                       "Invalid argument at line %d", line);
        }
    }
       
    if (fp != stdin) 
    {
        fclose(fp);
    }
    
    return nRet;
}

void createPidFile(void) 
{
    /**
     * Try to write the pid file in a best-effort way. 
     */
    FILE *fp = fopen(g_server.pidfile, "w");
    if (fp) 
    {
        fprintf(fp, "%d\n", (int)getpid());
        fclose(fp);
    }
}

void daemonize(void) 
{
    if (0 != fork()) 
    {
        exit(0); // parent exits
    }
    
    setsid(); // create a new session

    /**
     * Every output goes to /dev/null. If Myepoll is daemonized but
     * the 'logfile' is set to 'stdout' in the configuration file
     * it will not log at all. 
     */
    int fd = open("/dev/null", O_RDWR, 0);
    if (-1 != fd) 
    {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        
        if (fd > STDERR_FILENO) 
        {
            close(fd);
        }
    }
}

static void sigtermHandler(int sig) 
{
    MY_NOTUSED(sig);

    myepollLog(MY_NOTICE, "Received SIGTERM, scheduling shutdown >:( ...");
    
    if (NULL != g_ListenThread)
	{
		delete g_ListenThread;
		g_ListenThread = NULL;
	}
	
	if (g_server.daemonize) 
	{
        myepollLog(MY_NOTICE, "Removing the pid file.");
        
        unlink(g_server.pidfile);
    }
    
    _exit(EXIT_SUCCESS);
}

void setupSignalHandlers(void) 
{
    struct sigaction act;
	memset(&act, 0x00, sizeof(act));
	
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sigtermHandler;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);
}

void initServer()
{
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    setupSignalHandlers();
}

static void help(int argc, char **argv)
{		
    fprintf(stderr, "Usage: %s -c /path/to/%s.conf\n", argv[0], "myepoll");
}

int main(int argc, char **argv)
{
    if (3 != argc) 
    {
        help(argc, argv);
        
        return -1;
    }
	
    int opt = -1;
    char configfile[MIN_BUFSZ] = "";
	
    while ((opt = getopt(argc, argv, "c:")) != -1) 
    {
        switch (opt) 
        {
        case 'c':
            strcpy(configfile, optarg);
            break;

        default:
            help(argc, argv);
            
            return -1;
        }
    }
	
	initServerConfig();
	
    if (-1 == loadServerConfig(configfile))
    {
        return -1;
    }
    
    if (g_server.daemonize) 
    {
        daemonize();
    }
    
    initServer();
    
    if (g_server.daemonize) 
    {
        createPidFile();
    }
    
	g_ListenThread = new ListenThread();
	if (NULL == g_ListenThread)
	{
		myepollLog(MY_WARNING, "new ListenThread() err");
		
		return -1;
	}

	if (-1 == g_ListenThread->Bind())
	{
	    if (NULL != g_ListenThread)
    	{
    		delete g_ListenThread;
    		g_ListenThread = NULL;
    	}
	
	    return -1;
	}

	myepollLog(MY_NOTICE, "Epoll server is running :) ... (press CTRL-C to stop)");

	poll(NULL, 0, -1);

    return 0;
}
