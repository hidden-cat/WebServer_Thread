
#ifndef _SQL_CONNECTION_H_
#define _SQL_CONNECTION_H_

#include <iostream>
#include <list>
#include <mysql/mysql.h>
#include "locker.h"
#include "log.h"

using namespace std;

class connection_pool {

    public:
        connection_pool();
        ~connection_pool();

        //获取一个空闲连接
        MYSQL* getConnection(void);

        //释放当前使用的连接(把使用的连接放回去)
        bool releaseConn(MYSQL*);

        //销毁连接池
        void destroyPool(void);

        //得到当前空闲连接数
        int getFreeConn(void);

        //单例模式 同时初始化
        static connection_pool* getInstnce(string url, unsigned int port, string user, string passWord, \
            string dataBaseName, unsigned int maxConn);

    private:

        //初始化连接池
       connection_pool(string url, unsigned int port, string user, string passWord, \
            string dataBaseName, unsigned int maxConn);

        //mysql链接池
        list<MYSQL*> m_connList;
        //数据库地址
        string m_url;
        unsigned int m_port; //数据库端口号
        string m_user; //数据库用户名
        string m_passWord; //数据库密码
        string m_dataBaseName; //数据库库名
        unsigned int m_maxConn; //最大连接数
        //当前空闲连接数
        unsigned int m_freeConn;
        //当前在使用连接数
        unsigned int m_curConn;
        sem m_sem;
        locker m_lock;
        static connection_pool *m_conn;
};

#endif