#ifndef _EOPLL_H_
#define _EOPLL_H_

#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <cctype>
#include <error.h>
#include <cassert>
#include<signal.h>
#include <vector>
#include "log.h"

using namespace std;

class epoll {

public:
	epoll();
	~epoll();
	bool AddEpoll(int fd, uint32_t ListenEvents);
	void DelEpoll(int fd);
	int Wait(void);
	int GetEventFd(int i);
	uint32_t GetEvents(int i);


private:
	int _efd;
	//vector<struct epoll_event> _evs;
	epoll_event _evs[500];
};

//设置非独占模式
void setnolock(int fd);

#endif