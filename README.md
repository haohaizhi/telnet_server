# telnet_server
简单实现telnet的服务端，使用字符模式

# 前言
之前很想自己实现一个命令行交互程序，然后去研究了quagga中vtysh的代码。

然后阉割出来一个精简的vtysh，项目地址：[https://github.com/haohaizhi/mini_vtysh](https://github.com/haohaizhi/mini_vtysh)


# 实现

本项目为简单telnet服务端实现，目前只是将客户端输入进行显示，关闭行模式

```
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
```

# 编译
```
make -B
```