#include "log.h"

log::log() : m_count(0), m_log_count(1) {
    memset(m_dir_name, '\0', sizeof(m_dir_name));
    memset(m_file_name, '\0', sizeof(m_file_name));
}

log::~log() {
    if(!m_fp) {
        fflush(m_fp);
        fclose(m_fp);
    }
}

bool log::init(const char *file_name, int log_buf_size, int log_max) {
    m_log_max = log_max;
    time_t t = time(nullptr);
    struct tm *sys_tm = localtime(&t);
    m_today = sys_tm->tm_mday;

    const char *p = strrchr(file_name, '/');
    char fname[512] = {'\0'};

    if(!p) {
        //不存在则创建。
        snprintf(fname, 256, "%d_%02d_%02d_%s", sys_tm->tm_year + 1900, \
            sys_tm->tm_mon + 1, m_today, file_name);

        strcpy(m_file_name, fname);
    } else {
        strcpy(m_file_name, p + 1);
        strncpy(m_dir_name, file_name, p - file_name + 1);
        snprintf(fname, sizeof(fname) - 1, "%s_%d_%02d_%02d_%s", m_dir_name, \
            sys_tm->tm_year + 1900, sys_tm->tm_mon + 1, m_today, m_file_name);
    }
    
    m_fp = fopen(fname, "a");
    if(!m_fp) {
        LOG_ERROR("init log open file error");
        return false;
    }
    return true;
}

void log::write_log(int level, const char *msg) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr); //这个比time更精确
    time_t t = now.tv_sec; //tv_sec是秒
    struct tm *sys_tm = localtime(&t);
    char s[16] = {'\0'};
    char msg_buf[256] = {'\0'};
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[erro]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }
    //加锁再写。
    std::lock_guard<std::mutex> lok(g_mtx);
    ++m_count; //写入一行加1
    char tail[64] = {'\0'};
    //先把时间头写好
    snprintf(tail, sizeof(tail) - 1, "[%d年%02d月%02d日%02d时%02d分%02d秒]", sys_tm->tm_year + 1900, \
        sys_tm->tm_mon + 1, sys_tm->tm_mday, sys_tm->tm_hour, sys_tm->tm_min, sys_tm->tm_sec);

    if(m_today != sys_tm->tm_mday || m_count > m_log_max) {
        char new_log[256] = {'\0'};
        fflush(m_fp);
        fclose(m_fp);
        //如果是时间不是今天,则创建今天的日志。如超过了最大行,创建新的。
        if (m_today != sys_tm->tm_mday)
        {
            //新日志时间头
            snprintf(m_file_name, sizeof(m_file_name) - 1, "%d_%02d_%02d", sys_tm->tm_year + 1900, \
        sys_tm->tm_mon + 1, sys_tm->tm_mday);

            snprintf(new_log, sizeof(new_log) - 1, "%s%s", m_dir_name, m_file_name);
            m_today = sys_tm->tm_mday;
        }
        else
        {
            snprintf(new_log, sizeof(new_log) - 1, "%s%s.%d", m_dir_name, \
                m_file_name, m_log_count % m_log_max);
            m_log_count++;
        }
        m_count = 0;
        m_fp = fopen(new_log, "a");
        if(!m_fp) {
            LOG_ERROR("write_log log open file error");
            return;
        }
    }
    //写入日志。
    snprintf(msg_buf, sizeof(msg_buf) - 1, "%s %s %s\n", s, tail, msg);
    fputs(msg_buf, m_fp);
    fflush(m_fp);
}
