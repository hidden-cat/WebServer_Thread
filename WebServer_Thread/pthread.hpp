#include "sql_connection.h"
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include "locker.h"
using namespace std;

template<typename T>
class threadpool {
    public:
        threadpool(connection_pool* connpool, int thread_number = 2, int max_request = 500);
        ~threadpool();

        //把任务加入任务队列
        bool append(T* p);

    private:
        static void* worker(void* arg);
        void run(void);
    private:
        int m_thread_number; //线程池中的线程数
        int m_max_requests; //请求队列中允许的最大数
        vector<thread> m_threads; //线程池id数组
        list<T*> m_workqueue; //请求队列
        locker m_lock; //互斥锁
        sem m_sem; //是否有任务
        bool m_stop; //是否结束线程
        connection_pool* m_connPool; //数据库
};

template<typename T>
threadpool<T>::threadpool(connection_pool* connpool, int thread_number, int max_request) : \
    m_thread_number(thread_number),m_max_requests(max_request), m_stop(false), \
    m_connPool(connpool){

        //如果传入的要创建的线程数量或者最大线程数等于0则输出错误
        if(this->m_thread_number == 0 || this->m_max_requests == 0) {
            throw std::exception();
        }

        //创建线程
        for(int i = 0; i < thread_number; ++i) {

            m_threads.emplace_back(thread(&threadpool::worker, (void*)this));
            m_threads[i].detach();
        }
}

template<typename T>
threadpool<T>::~threadpool() {
    this->m_stop = true;
}

//把任务加入任务队列
template<typename T>
bool threadpool<T>::append(T* p) {
    this->m_lock.lock(); //互斥访问任务队列。
    //if(T* == nullptr) return false;
    if(this->m_workqueue.size() >= m_max_requests) {
        //任务数量到达最大，不再接受任务。
        p->send_file((char*)"./503.html");
        this->m_lock.unlock();
        return false;
    }
    this->m_workqueue.push_back(p);
    this->m_lock.unlock();
    this->m_sem.post(); //sem信号量post方法等于把它加1 sem信号量wait方法会减1
    return true;
}

template<typename T>
void* threadpool<T>::worker(void* arg) {
    //参数是this指针 没有this不能操作一些private的成员变量或函数
    threadpool *pool = (threadpool*) arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run() {
    T* p;
    while(!this->m_stop) {
        //wait是把任务量减1，如果任务数是0则等待，sem和任务数是相同。
        this->m_sem.wait();
        this->m_lock.lock(); //互斥访问任务队列。
        if(this->m_workqueue.empty()) {
            //任务列表为空。
            this->m_lock.unlock();
            continue;
        }
        //从任务队列取出待处理事件数据。
        p = this->m_workqueue.front();
        this->m_workqueue.pop_front(); //把已取出待处理事件指针抛弃。
        this->m_lock.unlock();

        p->sql = this->m_connPool->getConnection(); //取出一个空闲的mysql连接。
        if(p->sql == nullptr) {
            LOG_ERROR("get mysql connect error");
        }
        p->handleevent();
        this->m_connPool->releaseConn(p->sql);
        p->sql = nullptr;
    }
}