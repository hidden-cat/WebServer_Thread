ALL:ser
ser:
	g++ -o ser main.cpp class.h epoll.h epoll.cpp locker.h log.h log.cpp pthread.hpp server.h server.cpp http_conn.h http_conn.cpp sql_connection.h  sql_connection.cpp -lmysqlclient -lpthread


clean:
	rm -rf ./ser
PHONY: clean ALL