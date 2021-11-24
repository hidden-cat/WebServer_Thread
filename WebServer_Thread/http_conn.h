#ifndef _HTTP_H_
#define _HTTP_H_
#include <sys/stat.h>
#include <sys/wait.h>
#include "sql_connection.h"
#include "epoll.h"

class http_conn {
    public:
        //成员函数
        http_conn();
        ~http_conn();
        void init(int fd, sockaddr_in *addr);
        void send_head(int no, const char* desp, const char* type, int len);
	    int getfilelen(const char* file);
	    void send_file(char* file);
        bool read_http(void);
        int get_line(char *buf, int size);
        void handleevent(void);
	    void handleget(char* path);
	    void handlepost(char* path);
        void get_uandp(void); //解析body得到账号密码
        void login(void);
        void reg(void);
        bool is_outtime(void);
        void dis_conn(void); //关闭连接

        static epoll* m_ep;
        MYSQL *sql;

    private:
        //成员函数
        void init(void);

        //成员变量
        int m_cfd; //当前客户端的fd
        struct sockaddr_in m_addr; //当前客户端的sockaddr_in
        int m_content_length; //post模式下body正文的长度
        char m_body[64]; //post模式的body
        char m_id_pwd[2][16]; //用户账号和密码
        string m_http;
        int m_idx;
        time_t m_time;
};

#endif