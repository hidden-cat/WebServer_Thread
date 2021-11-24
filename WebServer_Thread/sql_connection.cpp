#include "sql_connection.h"

connection_pool::connection_pool() : m_freeConn(0), m_maxConn(0){

}

connection_pool::~connection_pool() {
    this->destroyPool();
}

connection_pool* connection_pool::m_conn = NULL;


//单例模式 同时初始化
connection_pool* connection_pool::getInstnce(string url, unsigned int port, string user, string passWord, \
            string dataBaseName, unsigned int maxConn) {

    if(m_conn == NULL) {
        m_conn = new connection_pool(url, port, user, passWord, dataBaseName, maxConn);
    }
    return m_conn;
}

//初始化连接池
connection_pool::connection_pool(string url, unsigned int port, string user, string passWord, \
            string dataBaseName, unsigned int maxConn) {

                this->m_url = url;
                this->m_port = port;
                this->m_user = user;
                this->m_passWord = passWord;
                this->m_dataBaseName = dataBaseName;

                MYSQL *con = NULL;
                this->m_lock.lock(); //加锁
                for(int i = 0; i < maxConn; ++i) {
                    con = mysql_init(con);
                    if(con == NULL) {
                        string errmsg = "mysql_init Error: ";
                        errmsg += mysql_error(con);
                        LOG_ERROR(errmsg.c_str());
                        exit(0);
                    }

                    con = mysql_real_connect(con, url.c_str(), user.c_str(), \
                        passWord.c_str(), dataBaseName.c_str(), port, NULL, 0);

                    if(con == NULL) {
                        string errmsg = "mysql_connection Error: ";
                        errmsg += mysql_error(con);
                        LOG_ERROR(errmsg.c_str());
                        exit(0);
                    }
                    mysql_set_character_set(con, "utf8"); //设置字符集
                    this->m_connList.push_back(con);
                    con = NULL;
                    ++this->m_freeConn;
                }

                this->m_sem = sem(maxConn);
                this->m_maxConn = maxConn;
                this->m_lock.unlock(); //解锁
}

//获取一个空闲连接
MYSQL* connection_pool::getConnection(void) {
    MYSQL *ret = NULL;

    if(this->m_connList.size() == 0) {
        //没有空闲连接 直接返回NULL
        return NULL;
    }

    this->m_sem.wait(); //等待其他正在被使用的连接释放
    this->m_lock.lock();

    ret = this->m_connList.front();
    this->m_connList.pop_front();
    --this->m_freeConn;
    ++this->m_curConn;
    this->m_lock.unlock();
    return ret;
}

//释放当前使用的连接(把使用的连接放回去)
bool connection_pool::releaseConn(MYSQL* con) {
    this->m_lock.lock();

    if(con == NULL) {
        this->m_lock.unlock();
        return false;
    }
    this->m_connList.push_back(con);
    ++this->m_freeConn;
    --this->m_curConn;
    this->m_lock.unlock();
    this->m_sem.post();
    return true;
}

//得到当前空闲连接数
int connection_pool::getFreeConn(void) {
    return this->m_freeConn;
}

//销毁连接池
void connection_pool::destroyPool(void) {

    this->m_lock.lock();
    for(auto i = this->m_connList.begin(); i != this->m_connList.end(); ++i) {
        mysql_close((MYSQL*)*i);
    }
    this->m_curConn = 0;
    this->m_freeConn = 0;
    this->m_connList.clear();
    this->m_lock.unlock();
}