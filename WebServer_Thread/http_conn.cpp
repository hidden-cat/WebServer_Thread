#include "http_conn.h"

epoll* http_conn::m_ep = nullptr;
char g_html_dir[8] = "./html/";
const int OUTTIME = 5;

// 通过文件名获取文件的类型
const char *get_file_type(const char *name){
  const char* dot;

  // 自右向左查找‘.’字符, 如不存在返回NULL
  dot = strrchr(name, '.');   
  if (dot == NULL)
    return "text/plain; charset=utf-8";
  if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
    return "text/html";
  if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
    return "image/jpeg";
  if (strcmp(dot, ".gif") == 0)
    return "image/gif";
  if (strcmp(dot, ".png") == 0)
    return "image/png";
  if (strcmp(dot, ".css") == 0)
    return "text/css";
  if (strcmp(dot, ".au") == 0)
    return "audio/basic";
  if (strcmp( dot, ".wav" ) == 0)
    return "audio/wav";
  if (strcmp(dot, ".avi") == 0)
    return "video/x-msvideo";
  if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
    return "video/quicktime";
  if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
    return "video/mpeg";
  if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
    return "model/vrml";
  if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
    return "audio/midi";
  if (strcmp(dot, ".mp3") == 0)
    return "audio/mpeg";
  if (strcmp(dot, ".ogg") == 0)
    return "application/ogg";
  if (strcmp(dot, ".pac") == 0)
    return "application/x-ns-proxy-autoconfig";

  return "text/plain; charset=utf-8";
}

http_conn::http_conn() : m_cfd(-1), sql(nullptr), m_idx(0) {

}

http_conn::~http_conn() {

}

void http_conn::init(int fd, sockaddr_in *addr) {
	bzero(&m_addr, sizeof(m_addr));
	m_addr = *addr;
	m_cfd = fd;
	this->init();
}

void http_conn::init(void) {
	m_content_length = 0;
	memset(m_body, '\0', sizeof(m_body));
	memset(m_id_pwd, '\0', sizeof(m_id_pwd));
	m_time = time(NULL);
}

void http_conn::send_head(int no, const char* desp, const char* type, int len)
{
	char buf[256] = {'\0'};
	snprintf(buf, sizeof(buf) - 1, "HTTP/1.1 %d %s\r\n", no, desp);
  	send(m_cfd, buf, strlen(buf), 0);
  	snprintf(buf, sizeof(buf) - 1,"Server: epoll webserver\r\n");
  	send(m_cfd, buf, strlen(buf), 0);
  	snprintf(buf, sizeof(buf) - 1,"Content-Type: %s\r\n", type);
  	send(m_cfd, buf, strlen(buf), 0);
  	snprintf(buf, sizeof(buf) - 1,"Connection: keep-alive\r\n"); //keep-alive
  	send(m_cfd, buf, strlen(buf), 0);
  	snprintf(buf, sizeof(buf) - 1,"content-length: %d\r\n", len);
  	send(m_cfd, buf, strlen(buf), 0);
  	snprintf(buf, sizeof(buf) - 1,"\r\n");
	send(m_cfd, buf, strlen(buf), 0);
}

int http_conn::getfilelen(const char* file) {
	struct stat st;
	stat(file, &st);
  return st.st_size;
}

void http_conn::send_file(char* file) {
	char buf[4096] = {'\0'};
	char sendfile[256] = {0};
	strcpy(sendfile, g_html_dir);
	strcat(sendfile, file);
	int len;
  	int fd = open(sendfile, O_RDONLY);
	if( fd == -1 ) {
		memset(sendfile + strlen(g_html_dir), 0, strlen(sendfile) - strlen(g_html_dir));
		strcat(sendfile, "404.html");
    	fd = open(sendfile, O_RDONLY);
    	len = getfilelen(sendfile);
		this->send_head(404, "Not Found", get_file_type(sendfile), len);
	} else {
    	len = getfilelen(sendfile);
    	this->send_head(200, "OK", get_file_type(sendfile), len);
  	}

	while((len = read(fd, buf,sizeof(buf))) > 0) send(m_cfd, buf, len, 0);
	if(len == -1)
	    LOG_ERROR("send_file read error");
}

//读取全部http消息
bool http_conn::read_http(void) {
	char line[1024] = {'\0'};
	int len;
	m_http.clear();
	m_idx = 0;

	while(1) {
		len = recv(m_cfd, line, sizeof(line) - 1, 0);
		if( len == -1 ) {

			if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}
			//LOG_ERROR("read -1 error");
			return false;

		} else if( len == 0 ) {
			return false;
		}
		m_http.append(line, len);
	}
	return true;
}

int http_conn::get_line(char *buf, int size){
    char c = '\0';
	int old_idx = m_idx;
    for( ; (m_idx < m_idx + size - 1) && (m_http.length() > m_idx); m_idx++ ) {
        c = m_http[m_idx];
        if(c == '\r' && m_http[m_idx + 1] == '\n') {
			m_idx += 2;
        	break;
        }
        buf[m_idx] = c;
    }
	return m_idx - old_idx;
}

void http_conn::handleevent(void){
    char line[1024] = {0};
    char buf[1024] = {0};

    if( get_line(line, sizeof(line)) < 1) {
		LOG_ERROR("get http line error");
		this->dis_conn();
		return;
	}
    char method[8], path[1024], protocol[12];
    sscanf(line, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
    // 判断get请求。
    if(strncasecmp("GET", method, 3) == 0) {
        // 处理get请求。
        this->handleget(path);
        
    } else if(strncasecmp("POST", method, 4) == 0){
        // 处理post请求。
        auto beg = m_http.find("Content-Length:");
		if(beg > 0) {
			m_content_length = atoi(&(m_http[beg + 15]));
            if(m_content_length > 0){
                strncpy(m_body, &(m_http[m_http.length() - m_content_length]), \
					m_content_length);
            }
		}
        this->handlepost(path);

    } else {
      //不支持的请求。
		this->send_file((char*)"501.html");
    }
	m_time = time(NULL);
}

void http_conn::handleget(char* path){
    char* arg = path + 1; // 去掉path中的/ 获取访问文件名
    // 如果没有指定访问的资源, 默认显示资源目录中的内容
	//不用arg是因为/已经被跳过了
    if(strcmp(path, "/") == 0) {    
		this->send_file((char*)"index.html");
		return;
    }
	this->send_file(arg);
}

void http_conn::handlepost(char* path){
    char* arg = path + 1; // 去掉path中的/ 获取访问文件名
	char file[256] = {0};
	strcpy(file, g_html_dir);

    if(m_content_length > 0) {
    	cout << "post arg = " << m_body << "   length = " << m_content_length << endl;
		this->get_uandp(); //解析body获得账号密码。
    } else {
    	cout << "post arg == " << arg << endl;
    }
	if( strcmp(arg, "0") == 0 ) {
		//用户要跳转到注册界面
		this->send_file((char*)"reg.html");

	} else if( strcmp(arg, "1") == 0 ) {
		//用户要跳转到登录界面
		this->send_file((char*)"login.html");

	} else if( strcmp(arg, "reg") == 0 ) {
		//用户注册
		this->reg();

	} else if( strcmp(arg, "login") == 0 ) {
		//用户登录
		this->login();
	}
}

void http_conn::get_uandp(void) {
	bool ifid_pwd = false;
	int i = 0, j = 0;
	char *p = m_body;
	while(*p != '\0') {
        if(*p == '=') {
            p++; //跳过等于号
            ifid_pwd = true;
            continue;
       	}
        if(*p == '&') {
            ifid_pwd = false;
			i++;
            j = 0;
        }
        if(ifid_pwd == true) {
            m_id_pwd[i][j++] = *p;
        }
        p++;
    }
}

void http_conn::login(void) {
	MYSQL_RES* result = NULL;
	MYSQL_ROW row;
	char psql[255] = {'\0'};
	char file[256] = {0};
	int ret;

	strcpy(file, g_html_dir);
	//执行mysql语句查找账号
    snprintf(psql, sizeof(psql) - 1, "select id,pwd from test01 where id='%s' and pwd='%s'", \
		m_id_pwd[0], m_id_pwd[1]);
    ret = mysql_query(sql, psql);
	if(ret != 0) {
		this->send_file((char*)"mysql_conn_err.html");
        return;
	}
    result = mysql_store_result(sql);
	if(!result) {
		this->send_file((char*)"mysql_conn_err.html");
        return;
	}
    int count = mysql_num_rows(result);//获取行数
    if(count < 1) {
        //账号不存在
		this->send_file((char*)"loginerr.html");
        return;
	}
	/*
	//设置在线
	snprintf(psql, sizeof(psql) - 1, "update test01 set zx=1 where id='%s'", m_id_pwd[0]);
	ret = mysql_query(sql, psql);
	if(ret != 0) {
    	this->send_file((char*)"./mysql_conn_err.html");
        return;
	}
	*/
	this->send_file((char*)"logincg.html");
}

void http_conn::reg(void) {
	MYSQL_RES* result = NULL;
	MYSQL_ROW row;
	char psql[256] = {'\0'};
	int ret;
	
	//执行mysql语句查找账号
    snprintf(psql, sizeof(psql) - 1, "select id from test01 where id='%s'", m_id_pwd[0]);
    ret = mysql_query(sql, psql);
	if(ret != 0) {
		this->send_file((char*)"mysql_conn_err.html");
		return;
	}

    result = mysql_store_result(sql);
	if(!result) {
		this->send_file((char*)"mysql_conn_err.html");
		return;
	}

    int count = mysql_num_rows(result);//获取行数
    if(count != 0) {
        //账号已经被注册
		this->send_file((char*)"regerr.html");
		return;

    } else {
        snprintf(psql, sizeof(psql) - 1, "insert into test01(id,pwd,zx) VALUES('%s','%s',0)", m_id_pwd[0], m_id_pwd[1]);
	    if(mysql_query(sql, psql) != 0) {
            this->send_file((char*)"mysql_conn_err.html");
			return;
	    }
		//注册成功
		this->send_file((char*)"regcg.html");
    }
}

bool http_conn::is_outtime(void){
	return ( (m_time + OUTTIME) <=  time(NULL));
}

void http_conn::dis_conn(void) {
	m_ep->DelEpoll(m_cfd);
	close(m_cfd);
	string msg = "cfd " + to_string(m_cfd) + "close";
    LOG_INFO(msg.c_str());
	sql = nullptr;
	m_cfd = -1;
	m_time = 0;
}