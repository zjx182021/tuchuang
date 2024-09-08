#ifndef _API_LOGIN_H_
#define _API_LOGIN_H_
#include "api_common.h"
#include "string"
using namespace std;
#if API_LOGIN_MUTIL_THREAD 
int ApiUserLogin(u_int32_t conn_uuid, std::string &url, std::string &post_data);
#else
int ApiUserLogin(string &url, string &post_data, string &str_json);
#endif
#endif // ! _API_LOGIN_H_
