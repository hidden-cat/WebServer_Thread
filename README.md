# WebServer_Thread

一个轻量的、多线程的、linux系统下的web服务器

使用c++语言 + epoll + 线程池

server主要维护用户http_conn类和任务分配

epoll类主要封装了epoll

log类为日志输出

http_conn类主要存储用户信息、处理用户发来的请求也在此处理

pthread是线程池

sql_connection是mysql连接池

locker主要封装了信号量和互斥锁

开发环境为: debian 9 and g++ 6.3.0 and mysql 8.0

编译直接执行make ser 编译后执行./ser -i0.0.0.0 -p5200. 参数i是ip地址，p是端口号
