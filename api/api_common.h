#ifndef _API_COMMON_H_
#define _API_COMMON_H_
#include "cache_pool.h"
#include "db_pool.h"
#include "redis_keys.h"
#include "tc_common.h"
#include "dlog.h"
#include "http_conn.h"
#include "json/json.h"
#include <string>

#define HTTP_RESPONSE_HTML_MAX 4096
#define HTTP_RESPONSE_HTML                                                     \
    "HTTP/1.1 200 OK\r\n"                                                      \
    "Connection:close\r\n"                                                     \
    "Content-Length:%d\r\n"                                                    \
    "Content-Type:application/json;charset=utf-8\r\n\r\n%s"
    
// 开启多线程
#define API_MYFILES_MUTIL_THREAD  0
#define API_LOGIN_MUTIL_THREAD  0

static string s_dfs_path_client;
// string s_web_server_ip;
// string s_web_server_port;
static string s_storage_web_server_ip;
static string s_storage_web_server_port;
static string s_shorturl_server_address;
static string s_shorturl_server_access_token;
using std::string;
int ApiInit();
//获取用户文件个数
int CacheSetCount(CacheConn *cache_conn, string key, int64_t count);
int CacheGetCount(CacheConn *cache_conn, string key, int64_t &count);
int CacheIncrCount(CacheConn *cache_conn, string key);
int CacheDecrCount(CacheConn *cache_conn, string key);
int DBGetUserFilesCountByUsername(CDBConn *db_conn, string user_name,
                                  int &count);
int DBGetShareFilesCount(CDBConn *db_conn, int &count);
int DBGetSharePictureCountByUsername(CDBConn *db_conn, string user_name,
                                     int &count);
int RemoveFileFromFastDfs(const char *fileid);
template <typename... Args>
static std::string FormatString(const std::string &format, Args... args);
#endif
