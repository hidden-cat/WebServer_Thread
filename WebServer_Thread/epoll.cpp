#include "epoll.h"


epoll::epoll() : _efd(epoll_create(5)){
	
}

epoll::~epoll() {
	close(_efd);
}

bool epoll::AddEpoll(int fd, uint32_t ListenEvents) {

	setnolock(fd);
	
	/*把设置好的新客户加入监听列表*/
	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = ListenEvents;
	if(epoll_ctl(_efd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		LOG_ERROR("epoll_ctl error");
		return false;
	}
	string msg = "add fd " + to_string(fd);
	LOG_INFO(msg.c_str());
	return true;
}

void epoll::DelEpoll(int fd) {
	epoll_ctl(_efd, EPOLL_CTL_DEL, fd, 0);
}

int epoll::Wait(void){
	//return epoll_wait(_efd, &_evs[0], static_cast<int>(_evs.size()), -1); //等待监听事件触发
	return epoll_wait(_efd, _evs, 500, -1); //等待监听事件触发
}

int epoll::GetEventFd(int i){
	if(i > 500 || i < 0){
		return 0;
	}
	return _evs[i].data.fd;
}

uint32_t epoll::GetEvents(int i){
	if(i > 500 || i < 0){
		return 0;
	}
	return _evs[i].events;
}

//设置非独占模式
void setnolock(int fd) {
	int flag = fcntl(fd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flag);
}
