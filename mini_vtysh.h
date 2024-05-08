#ifndef MINI_VTYSH_H
#define MINI_VTYSH_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <sys/epoll.h>

#define HexPrint(_buf, _len) \
        {\
            int _m_i = 0;\
            char *_m_buf = (char *)(_buf);\
            int _m_len = (int)(_len);\
            printf("[%s:%d] \r\n", __FUNCTION__, __LINE__);\
            printf("***************************************************\n");\
            for(_m_i = 0; _m_i < _m_len; _m_i++)\
            {\
                printf("\033[32m%02x \033[0m", _m_buf[_m_i] & 0xff);\
                if(!((_m_i+1) % 16))  printf("\n");\
            }\
            printf("\nsize = %d\n***************************************************\n", _m_len);\
        }

#define Syslog(_level, fmt, ...) \
                do{openlog("[MINI_VTYSH]", LOG_NDELAY | LOG_CONS | LOG_PERROR , LOG_LOCAL5); \
                syslog(_level, " " fmt "", ##__VA_ARGS__); \
                closelog();}while(0)

#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */

# define IAC         255  /* interpret as command: */
# define DONT        254  /* you are not to use option */
# define DO          253  /* please, you use option */
# define WONT        252  /* I won't use option */
# define WILL        251  /* I will use option */
# define SB          250  /* interpret as subnegotiation */
# define SE          240  /* end sub negotiation */
# define TELOPT_ECHO   1  /* echo */
# define TELOPT_SGA    3  /* suppress go ahead */
# define TELOPT_TTYPE 24  /* terminal type */
# define TELOPT_NAWS  31  /* window size */

#define EVENT_NUM 5
#define MAX_INPUT_LENGTH 128
#define VTY_READ_BUFSIZ 512
static int connect_num = 0;
const char *prompt = "SWITCH# ";

#define sockunion_family(X)  (X)->sa.sa_family
#define VTY_NEWLINE "\r\n"

union sockunion 
{
  struct sockaddr sa;
  struct sockaddr_in sin;
};

/* Structure of command element. */
struct cmd_element 
{
  const char *string;			/* Command specification by string. */
  int (*func) (struct cmd_element *, struct vty *, int, const char *[]);
  const char *doc;			/* Documentation of this command. */
  int daemon;                   /* Daemon to which this command belong. */
  u_char attr;			/* Command attributes */
};

/* VTY struct. */
struct vty 
{
  /* File descripter of this vty. */
  int fd;

  /* output FD, to support stdin/stdout combination */
  int wfd;

  /* Is this vty connect to file or not */
  enum {VTY_TERM, VTY_FILE, VTY_SHELL, VTY_SHELL_SERV} type;

  /* Node status of this vty */
  int node;

  /* Failure count */
  int fail;

  /* Output buffer. */
  struct buffer *obuf;

  /* Command input buffer */
  char *buf;

  /* Command cursor point */
  int cp;

  /* Command length */
  int length;

  /* Command max length. */
  int max;

  /* Timeout seconds and thread. */
  unsigned long v_timeout;
#define SU_ADDRSTRLEN 16
  /* What address is this vty comming from. */
  char address[SU_ADDRSTRLEN];
};


#define DEFUN_CMD_ELEMENT(funcname, cmdname, cmdstr, helpstr, attrs, dnum) \
  struct cmd_element cmdname = \
  { \
    .string = cmdstr, \
    .func = funcname, \
    .doc = helpstr, \
    .attr = attrs, \
    .daemon = dnum, \
  };

#define DEFUN_CMD_FUNC_DECL(funcname) \
  static int funcname (struct cmd_element *, struct vty *, int, const char *[]);

#define DEFUN_CMD_FUNC_TEXT(funcname) \
  static int funcname \
    (struct cmd_element *self __attribute__ ((unused)), \
     struct vty *vty __attribute__ ((unused)), \
     int argc __attribute__ ((unused)), \
     const char *argv[] __attribute__ ((unused)) )


#define DEFUN(funcname, cmdname, cmdstr, helpstr) \
  DEFUN_CMD_FUNC_DECL(funcname) \
  DEFUN_CMD_ELEMENT(funcname, cmdname, cmdstr, helpstr, 0, 0) \
  DEFUN_CMD_FUNC_TEXT(funcname)



typedef socklen_t SOCKLEN_T;
typedef struct timeval TIMEVAL_T;
typedef struct sockaddr SOCKADDR_T;
typedef struct sockaddr_in SOCKADDR_IN_T;
//UDP 服务端类
class CUdpServer
{
public:
    CUdpServer();
    CUdpServer(const char *pcHost, unsigned int uiPort);
    ~CUdpServer();

    int CUdpSocket(const char *pcHost, unsigned int uiPort);
    int CUdpRecvData(void *pvBuff, unsigned int uiBuffLen, SOCKADDR_IN_T *pstClientInfo);
    int CUdpSendData(const void *pvData, unsigned int uiDataLen, SOCKADDR_IN_T stClientInfo);

private:
    int m_iSerSock;
    unsigned int m_uiPort;
    unsigned char m_uiHost[16];
};

CUdpServer::CUdpServer()
{
    m_uiPort = 0;
    m_iSerSock = -1;
    memset(m_uiHost, 0x00, sizeof(m_uiHost));
}

CUdpServer::CUdpServer(const char *pcHost, unsigned int uiPort)
{
    m_uiPort = uiPort;
    memset(m_uiHost, 0x00, sizeof(m_uiHost));
    strcpy((char*)m_uiHost, pcHost);
    CUdpSocket(pcHost, uiPort);
}

CUdpServer::~CUdpServer()
{
    m_uiPort = 0;
    if(0 < m_iSerSock) close(m_iSerSock);
}

int CUdpServer::CUdpSocket(const char *pcHost, unsigned int uiPort)
{
    int iRet = 0;
    m_uiPort = uiPort;
    memset(m_uiHost, 0x00, sizeof(m_uiHost));
    strcpy((char*)m_uiHost, pcHost);

    if( (0 == m_uiPort) || (NULL == m_uiHost))
    {
        assert(true);
        //LOG(LOG_ERR, "Para is error!");
        return -1;
    }

    m_iSerSock = socket(AF_INET, SOCK_DGRAM, 0);
    if(0 > m_iSerSock)
    {
        //LOG(LOG_ERR, "Udp server socket failed!");
        return -1;
    }

    TIMEVAL_T stTimeout;
    stTimeout.tv_sec = 3;
    stTimeout.tv_usec = 0;
    setsockopt(m_iSerSock, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(stTimeout));

    stTimeout.tv_sec = 0;
    stTimeout.tv_usec = 0;
    setsockopt(m_iSerSock, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(stTimeout));

    int iOptval = 1;
    setsockopt(m_iSerSock, SOL_SOCKET, SO_REUSEADDR, &iOptval, sizeof(int));

    struct sockaddr_in stAddr;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(m_uiPort);
    stAddr.sin_addr.s_addr = inet_addr((const char *)m_uiHost);
    iRet = bind(m_iSerSock, (struct sockaddr *)&stAddr, (socklen_t)sizeof(stAddr));
    if(0 > iRet)
    {
        //LOG(LOG_ERR, "Udp server bind failed!");
        return -1;
    }

    return m_iSerSock;
}

int CUdpServer::CUdpRecvData(void *pvBuff, unsigned int uiBuffLen, SOCKADDR_IN_T *pstClientInfo)
{
    int inRead = 0;
    SOCKLEN_T stAddrLen;
    
    if((NULL == pvBuff) || (NULL == pstClientInfo))
    {
        //LOG(LOG_ERR, "Para is error!");
        return -1;
    }

    stAddrLen = sizeof(SOCKADDR_IN_T);
    inRead = recvfrom(m_iSerSock, pvBuff, uiBuffLen, 0, (SOCKADDR_T *)pstClientInfo,(SOCKLEN_T *)&stAddrLen);

    return inRead;
}

int CUdpServer::CUdpSendData(const void *pvData, unsigned int uiDataLen, SOCKADDR_IN_T stClientInfo)
{
    int inSend = 0;

    if(NULL == pvData)
    {
        //LOG(LOG_ERR, "Para is error!");
        return -1;
    }

    inSend = sendto(m_iSerSock, pvData, uiDataLen, 0, (SOCKADDR_T *)&stClientInfo, sizeof(SOCKADDR_T));

    return inSend;
}

//UDP客户端类
class CUdpClient
{
public:
    CUdpClient();
    CUdpClient(const char *pcHost, unsigned int uiPort);
    ~CUdpClient();

    int CUdpRecvData(void *pcBuff, unsigned int uiBuffLen);
    int CUdpSendData(const void *pcData, unsigned int uiDataLen);
    int CUdpRecvData(void *pcBuff, unsigned int uiBuffLen, const char *pcHost, unsigned int uiPort);
    int CUdpSendData(const void *pcData, unsigned int uiDataLen, const char *pcHost, unsigned int uiPort);

    int CUdpSetSendTimeout(unsigned int uiSeconds = 3);
    int CUdpSetRecvTimeout(unsigned int uiSeconds = 3);
    int CUdpSetBroadcastOpt();

private:
    int CUdpSocket();
    int CUdpGetSockaddr(const char * pcHost, unsigned int uiPort, SOCKADDR_IN_T *pstSockaddr);

private:
    int m_iClientSock;
    unsigned int m_uiPort;
    unsigned char m_ucHost[16];
    SOCKADDR_IN_T m_stServerInfo;
};

CUdpClient::CUdpClient()
{
    m_iClientSock = -1;
    m_uiPort = 0;
    memset(m_ucHost, 0x00, sizeof(m_ucHost));
    memset(&m_stServerInfo, 0x00, sizeof(m_stServerInfo));
    CUdpSocket();
}
    
CUdpClient::CUdpClient(const char *pcHost, unsigned int uiPort)
{
    m_iClientSock = -1;
    m_uiPort = uiPort;
    memset(m_ucHost, 0x00, sizeof(m_ucHost));
    strcpy((char*)m_ucHost, pcHost);
    memset(&m_stServerInfo, 0x00, sizeof(m_stServerInfo));
    CUdpSocket();
}

CUdpClient::~CUdpClient()
{
    if(0 < m_iClientSock) close(m_iClientSock);
}

int CUdpClient::CUdpGetSockaddr(const char * pcHost, unsigned int uiPort, SOCKADDR_IN_T *pstSockaddr)
{
    SOCKADDR_IN_T stSockaddr;
    
    if((NULL == pcHost) || (0 == uiPort) || (NULL == pstSockaddr))
    {
        //SysLog(LOG_ERR, "Para is error!");
        return -1;
    }

    memset(&stSockaddr, 0x00, sizeof(stSockaddr));
    stSockaddr.sin_family = AF_INET;
    stSockaddr.sin_port = htons(uiPort);
    stSockaddr.sin_addr.s_addr = inet_addr(pcHost);
    if(stSockaddr.sin_addr.s_addr == INADDR_ANY)
    {
        //SysLog(LOG_ERR, "Incorrect ip address!");
        return -1;
    }

    memcpy(pstSockaddr, &stSockaddr, sizeof(SOCKADDR_IN_T));

    return 0;
}


int CUdpClient::CUdpSocket()
{
    m_iClientSock = socket(AF_INET, SOCK_DGRAM, 0);
    if(0 > m_iClientSock)
    {
        //SysLog(LOG_ERR, "Udp client socket failed!");
        return 0;
    }

    CUdpSetRecvTimeout(3);
    CUdpSetSendTimeout(0);

    return 0;
}

int CUdpClient::CUdpSetSendTimeout(unsigned int uiSeconds)
{
    TIMEVAL_T stTimeout;
    
    stTimeout.tv_sec = uiSeconds;
    stTimeout.tv_usec = 0;
    
    return setsockopt(m_iClientSock, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(stTimeout));
}

int CUdpClient::CUdpSetRecvTimeout(unsigned int uiSeconds)
{
    TIMEVAL_T stTimeout;
    
    stTimeout.tv_sec = uiSeconds;
    stTimeout.tv_usec = 0;
    
    return setsockopt(m_iClientSock, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(stTimeout));
}

int CUdpClient::CUdpSetBroadcastOpt()
{
    int iOptval = 1;
    return setsockopt(m_iClientSock, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &iOptval, sizeof(int));
}

int CUdpClient::CUdpRecvData(void *pcBuff, unsigned int uiBuffLen)
{
    return  CUdpRecvData(pcBuff, uiBuffLen, (const char *)m_ucHost, m_uiPort);
}

int CUdpClient::CUdpRecvData(void *pcBuff, unsigned int uiBuffLen, const char *pcHost, unsigned int uiPort)
{
    SOCKLEN_T stSockLen = 0;
    SOCKADDR_IN_T stSockaddr;
    
    if((NULL == pcBuff) || (NULL == pcHost) || (0 == uiPort))
    {
        //SysLog(LOG_ERR, "Para is error!");
        return -1;
    }

    memset(&stSockaddr, 0x00, sizeof(stSockaddr));
    if(0 != CUdpGetSockaddr(pcHost, uiPort, &stSockaddr))
    {
        //SysLog(LOG_ERR, "Get sockaddr failed!");
        return -1;
    }

    stSockLen = sizeof(SOCKADDR_IN_T);
    return recvfrom(m_iClientSock, pcBuff, uiBuffLen, 0, (SOCKADDR_T *)&stSockaddr, (SOCKLEN_T *)&stSockLen);
}

int CUdpClient::CUdpSendData(const void *pcData, unsigned int uiDataLen) 
{ 
    return CUdpSendData(pcData, uiDataLen, (const char*)m_ucHost, m_uiPort); 
}

int CUdpClient::CUdpSendData(const void *pcData, unsigned int uiDataLen, const char *pcHost, unsigned int uiPort)
{
    SOCKADDR_IN_T stSockaddr;
    
    if((NULL == pcData) || (NULL == pcHost) || (0 == uiPort))
    {
        //SysLog(LOG_ERR, "Para is error!");
        return -1;
    }

    memset(&stSockaddr, 0x00, sizeof(stSockaddr));
    if(0 != CUdpGetSockaddr(pcHost, uiPort, &stSockaddr))
    {
        //SysLog(LOG_ERR, "Get sockaddr failed!");
        return -1;
    }

    return sendto(m_iClientSock, pcData, uiDataLen, 0, (SOCKADDR_T *)&stSockaddr, (SOCKLEN_T)sizeof(stSockaddr));
}


#endif /*MINI_VTYSH_H*/