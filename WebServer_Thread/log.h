#include <iostream>
#include <cstdio>
#include <cstring>
#include <sys/time.h>
#include <mutex>

#ifndef _LOG_H_
#define _LOG_H_

using std::cout;
static std::mutex g_mtx; //互斥锁

class log {
    public:
        log();
        ~log();
        bool init(const char *file_name, int log_buf_size, int log_max); //初始化
        void write_log(int level, const char *msg); //写日志
    static log *get_instance()
    {
        std::lock_guard<std::mutex> lok(g_mtx);
        static log instance;
        return &instance;
    }
    
    private:
        int m_count;   //当前日志行数
        int m_log_max; //最大行数
        int m_log_count; //如果日志文件大于1，日志文件后缀
        int m_today;    //因为按天分类,记录当前时间是那一天
        FILE *m_fp; //log文件指针
        char m_dir_name[128]; //log文件目录
        char m_file_name[128]; //log文件名
};

#define LOG_DEBUG(format) log::get_instance()->write_log(0, format)
#define LOG_INFO(format) log::get_instance()->write_log(1, format)
#define LOG_WARN(format) log::get_instance()->write_log(2, format)
#define LOG_ERROR(format) log::get_instance()->write_log(3, format)

#endif
