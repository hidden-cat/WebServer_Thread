#ifndef _SERVER_H_

#define _SERVER_H_
#include <iostream>
#include <list>
#include "epoll.h"
#include "sql_connection.h"
#include "pthread.hpp"
#include "class.h"
#include "http_conn.h"

class server {

public:
	server(char* ip, unsigned short int port);
	~server();
	void start(void);
	void out_time(void);

private:
	int _listfd;
	epoll* _ep;
	connection_pool *_sql_conn;
	threadpool<http_conn>* _tpool;
	http_conn* _users;
	list<http_conn*> _out;
	std::mutex _mtx;
};

#endif

