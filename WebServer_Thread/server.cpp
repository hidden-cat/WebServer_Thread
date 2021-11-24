#include "server.h"

server::server(char* ip, unsigned short int port)   :_ep(new epoll()) ,
    _sql_conn(connection_pool::getInstnce("localhost", 3306, "root", "qqq", "my", 3)),
    _tpool(new threadpool<http_conn>(_sql_conn)),
    _users(new http_conn[65535]){

    _listfd = socket(PF_INET, SOCK_STREAM, 0);
    if (_listfd < 1) {
        LOG_ERROR("socket error");
        return;
    }

    int opt = 1;
    setsockopt(_listfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //端口复用

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    int ret = bind(_listfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret != 0) {
        LOG_ERROR("bind error");
        return;
    }

    ret = listen(_listfd, 5);
    if (ret != 0) {
        LOG_ERROR("listen error");
        return;
    }
    _ep->AddEpoll(_listfd, EPOLLIN | EPOLLRDHUP);
    LOG_INFO("listen success");
}

server::~server() {
    if(_listfd){
        _ep->DelEpoll(_listfd);
        close(_listfd);
    }
    delete _ep;
    delete _tpool;
    delete _sql_conn;
    delete[] _users;
}

void server::start(void){
    int i, cfd, len, ret;
    uint32_t events;
    http_conn::m_ep = _ep;
    LOG_INFO("开始wait...");
    while(1) {
        ret = _ep->Wait();
        if(ret < 0 && errno != EINTR) {
            LOG_ERROR("epoll_wait error");
            break;
        }
        for(i = 0; i < ret; i++) {
            cfd = _ep->GetEventFd(i);
            events = _ep->GetEvents(i);
			if( (cfd == _listfd) && (events & EPOLLIN) ) {
			    //客户接入处理。
                struct sockaddr_in client;
	            socklen_t len = sizeof(client);
                cfd = accept(_listfd, (struct sockaddr*)&client, &len);
	            if(cfd < 0) {
		            LOG_ERROR("accept error");
                    continue;
	            }
                {
                std::lock_guard<std::mutex> lock(_mtx);
                _users[cfd].init(cfd, &client);
                _ep->AddEpoll(cfd, EPOLLIN | EPOLLRDHUP);
                _out.push_back(_users + cfd);
                }

            } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                _users[cfd].dis_conn();

            } else if( events & EPOLLIN ) {
                if( _users[cfd].read_http() == false){
		            _users[cfd].dis_conn();
		            continue;
	            }
                LOG_INFO("append success");
                _tpool->append(_users + cfd);

            } else if(events & EPOLLOUT) {
                LOG_INFO("epollout event");

            } else {
                _users[cfd].dis_conn();
                LOG_INFO("epoll error");
            }
      }
    }
}

void server::out_time(void){
    http_conn* p = nullptr;
    std::lock_guard<std::mutex> lock(_mtx);
    for(int i = 0; i < 100 && !_out.empty(); i++){
        p = _out.front();
        if( p->is_outtime() ){
            _out.pop_front();
            p->dis_conn();
        }
    }
}