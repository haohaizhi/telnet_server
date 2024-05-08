#include "mini_vtysh.h"

const char *safe_strerror(int errnum)
{
    const char *s = strerror(errnum);
    return (s != NULL) ? s : "Unknown error";
}

int set_nonblocking(int fd)
{
    int flags;
    if ((flags = fcntl(fd, F_GETFL)) < 0)
    {
        Syslog(LOG_ERR, "fcntl(F_GETFL) failed for fd %d: %s",fd, safe_strerror(errno));
        return -1;
    }
    if (fcntl(fd, F_SETFL, (flags | O_NONBLOCK)) < 0)
    {
        Syslog(LOG_ERR, "fcntl failed setting fd %d non-blocking: %s",fd, safe_strerror(errno));
        return -1;
    }
    return 0;
}

/* VTY standard output function. */
int vty_out(int socket_fd, const char *format, ...) 
{
	va_list args;
	int len = 0;

    int stdout_backup = dup(STDOUT_FILENO);

    if (stdout_backup == -1) {
        perror("dup");
        return 1;
    }

   // 将标准输出重定向到套接字
    if (dup2(socket_fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        // 处理重定向失败的情况
        close(stdout_backup);
        return 1;
    }
	/* Try to write to initial buffer.  */
	va_start(args, format);
    len = vprintf (format, args);
    va_end(args);

    fflush(stdout); // 刷新输出缓冲区

    // 恢复标准输出
    if (dup2(stdout_backup, STDOUT_FILENO) == -1) {
        perror("dup2");
        close(stdout_backup);
        // 处理恢复标准输出失败的情况
        return 1;
    }

    close(stdout_backup);

	return len;
}

/* Send WILL TELOPT_ECHO to remote server. */
static void vty_hello_echo(int socket_fd) {
    unsigned char cmd[] = { 0xff, 0xfb , 0x01 , 0xff , 0xfb , 0x03 , 0xff , 0xfe , 0x22 , 0xff , 0xfd , 0x1f, '\0' };
	vty_out(socket_fd, "%s", cmd);
}

// static void vty_will_echo(int socket_fd) {
// 	unsigned char cmd[] = { IAC, WILL, TELOPT_ECHO, '\0' };
// 	vty_out(socket_fd, "%s", cmd);
// }

// /* Make suppress Go-Ahead telnet option. */
// static void vty_will_suppress_go_ahead(int socket_fd) {
// 	unsigned char cmd[] = { IAC, WILL, TELOPT_SGA, '\0' };
// 	vty_out(socket_fd, "%s", cmd);
// }

// /* Make don't use linemode over telnet. */
// static void vty_dont_linemode(int socket_fd) {
// 	unsigned char cmd[] = { IAC, DONT, TELOPT_LINEMODE, '\0' };
// 	vty_out(socket_fd, "%s", cmd);
// }

// /* Use window size. */
// static void vty_do_window_size(int socket_fd) {
// 	unsigned char cmd[] = { IAC, DO, TELOPT_NAWS, '\0' };
// 	vty_out(socket_fd, "%s", cmd);
// }


const char *sockunion2str (const union sockunion *su, char *buf, size_t len)
{
    switch (sockunion_family(su))
    {
        case AF_UNSPEC:
            snprintf (buf, len, "(unspec)");
            return buf;
        case AF_INET:
            return inet_ntop (AF_INET, &su->sin.sin_addr, buf, len);
    }
    snprintf (buf, len, "(af %d)", sockunion_family(su));
    return buf;
}

// 定义命令解析器
int vty_execute(int sock_fd, const unsigned char *cmd)
{
    printf("socket: %d Command received:%s \n", sock_fd,cmd);
    // HexPrint(cmd,strlen(cmd));
    
    fflush(stdout); // 刷新输出缓冲区
    vty_out(sock_fd,"%s %s",cmd, VTY_NEWLINE);
    return 0;
}

void signal_handler(int signal) {
    printf("Received signal %d. Exiting...\n", signal);
    fflush(stdout); // 刷新输出缓冲区
    exit(EXIT_SUCCESS);
}

int analyze_char(char c) {
    if (isalnum(c) || ispunct(c) || c == 0x03 || c == 0x04) {
        return 1;
    } else {
        return 0;
    }
}

void *handle_client(void *args) {
    int client_socket = *((int *) args);
    unsigned char buffer[VTY_READ_BUFSIZ];
    char buf[SU_ADDRSTRLEN] = {0};
    union sockunion su;
    unsigned char *p;
    int length = 0;

    memset (&su, 0, sizeof (union sockunion));
    socklen_t len;
    len = sizeof (union sockunion);

    // 获取客户端地址信息
    if (getpeername(client_socket,(struct sockaddr *)&su, &len) == -1) {
        connect_num--;
        perror("getpeername");
        close(client_socket);
        return NULL;
    }

    if(set_nonblocking(client_socket) < 0)
    {
        connect_num--;
        perror("set_nonblocking");
        close(client_socket);
        return NULL;
    }

    vty_hello_echo(client_socket);
    // vty_will_echo(client_socket);
    // vty_will_suppress_go_ahead(client_socket);
    // vty_dont_linemode(client_socket);
    // vty_do_window_size(client_socket);

    vty_out(client_socket, "Vty connection from %s. %s",sockunion2str (&su, buf, SU_ADDRSTRLEN), VTY_NEWLINE);

    // 发送欢迎消息
    vty_out(client_socket, "Welcome to my Telnet server![%d]. %s", connect_num, VTY_NEWLINE);
    // 创建epoll句柄
    int epoll_fd = epoll_create(5);

    // 将client_socket添加到epoll监听
    struct epoll_event event,events[EVENT_NUM];
    event.events = EPOLLIN;
    event.data.fd = client_socket;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);

    while (1) 
    {
        int n = epoll_wait(epoll_fd, events, 5, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == client_socket) {
                memset(buffer, 0, sizeof(buffer));

                int valread = read(client_socket, buffer, VTY_READ_BUFSIZ);
                if (valread == 0) {
                    goto CLOSE;
                }
                for (p = buffer; p < buffer+valread; p++)
                {
                    length++;
                    if (*p == '\0')
                        break;
                }
                unsigned char *cmd = (unsigned char *)malloc(length * sizeof(unsigned char));
                memcpy(cmd, buffer,length);
                if(cmd[0] == IAC)
                {
                }
                else
                    vty_execute(client_socket, cmd);
                free(cmd);
                length = 0;

            }
        }
    }

    connect_num--;
    printf("Client disconnected.\n");
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, NULL);
    close(client_socket);
    
    return NULL;
CLOSE:
    connect_num--;
    printf("Connection closed by the client\n");
    
    // 从epoll监听中移除套接字
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, NULL);
    // 关闭套接字
    close(client_socket);
    fflush(stdout); 
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // 注册信号处理函数
    signal(SIGINT, signal_handler); // Ctrl+C
    signal(SIGTERM, signal_handler); // 终止信号
    signal(SIGPIPE, signal_handler); // 终止信号

    /* 处理子进程退出以免产生僵尸进程 */
    signal(SIGCHLD, SIG_IGN);

    // 创建 socket 文件描述符
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        return -1;
    }

    // 设置 socket 选项，允许多个连接
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(23); // Telnet 默认端口号

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return -1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        return -1;
    }

    perror("Server started. Waiting for connections..." );

    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))) {
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }
        connect_num++;
        if(connect_num > 3)
        {
            const char msg[] = "\r\n"
            "mini_vtysh just permit 3 socket connect!\r\n"
            "please wait other connect close.\r\n"; 
            
            set_nonblocking(new_socket);
            vty_hello_echo(new_socket);

            vty_out(new_socket, msg); // 发送命令行提示符
            close(new_socket);
            connect_num--;
            continue;
        }
        pthread_t socketID;
        int ret = pthread_create(&socketID,NULL,handle_client,&new_socket);
        if(ret != 0){
            perror("create the thread failed\n");
            close(new_socket);
            connect_num--;
            continue;
        }
        perror("New connection accepted.");
        pthread_detach(socketID); 
        
    }
    close(server_fd);
    return 0;
}
