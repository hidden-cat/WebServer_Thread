#include "log.h"
#include "server.h"

static server* g_ser;
const int OUTTIME = 5;

void close_process(int msg) {
    if(g_ser){
        delete g_ser;
    }
    g_ser = NULL;
    cout << "close" << endl;
    exit(0);
}

void out_time(int sig){
    g_ser->out_time();
    signal(SIGALRM,out_time);
    alarm(OUTTIME);
}

int main(int argc, char *argv[]) {

    //屏蔽信号
    for(int i = 0; i < 50; i++) {
        signal(i, SIG_IGN);
    }
    //退出信号不能屏蔽 重新监视起来
    signal(SIGTERM, close_process);
    signal(SIGINT,close_process);
    signal(SIGALRM,out_time);
    char ip[16] = {'\0'};
    unsigned short int port = 0;
    int ret;
    log* l = log::get_instance();
    if( !l->init("log", 200, 500) ) {
        cout << "log create error" << endl;
    }
    //解析参数
    while ((ret = getopt(argc, argv, "i:p:")) != -1) {
        
        switch(ret) {

            case 'i' :
                strcpy(ip, optarg);
                break;
            case 'p' :
                port = (unsigned short int)atoi(optarg);
                break;
        }
    }
    cout << ip << "   " << port << endl;
    char workDir[128];
    getcwd(workDir, 128);
    chdir(workDir); //请修改成main.cpp所在目录，html目录的../目录
    g_ser = new server(ip, port);
    alarm(OUTTIME);
    g_ser->start();
    close_process(15);
    return 0;
}