#include "api_sharefiles.h"
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api_common.h"
#include <sys/time.h>
template <typename... Args>
static std::string FormatString(const std::string &format, Args... args) {
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...) +
                1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(),
                       buf.get() + size - 1); // We don't want the '\0' inside
}
int getShareFilesCount(CDBConn *db_conn, CacheConn *c_conn,int& count){
	int ret = 0;
	int64_t file_count = 0;

	string str_sql;
	if(CacheGetCount(c_conn, FILE_PUBLIC_COUNT, file_count) < 0){
		LogWarn("CacheGetCount failed\n");
		ret = -1;
	}

	if(file_count == 0){
		if(DBGetShareFilesCount(db_conn, count) < 0){
			LogError("CacheGetCount failed\n");
			ret = -1;
		}

		file_count = (int64_t)count;
		if(CacheSetCount(c_conn, FILE_PUBLIC_COUNT, count)<0){
			LogError("CacheSetCount failed\n");
			return -1;
		}

		ret = 0;
	}
	count = file_count;
	return ret;
}

int encodeSharefilesJson(int ret, int total, string &str_json){
	Json::Value root;
	root["code"] = ret;
	if(ret == 0){
		root["total"]=total;
	}
	Json::FastWriter writer;
	str_json = writer.write(root);
    return 0;
}
int decodeShareFileslistJson(string &str_json, int &start, int &count) {
    bool res;
    Json::Value root;
    Json::Reader jsonReader;
    res = jsonReader.parse(str_json, root);
    if (!res) {
        LogError("parse reg json failed ");
        return -1;
    }

    if (root["start"].isNull()) {
        LogError("start null\n");
        return -1;
    }
    start = root["start"].asInt();

    if (root["count"].isNull()) {
        LogError("count null\n");
        return -1;
    }
    count = root["count"].asInt();

    return 0;
}


int handleGetSharefilesCount(int &count){
	CDBManager *manager = CDBManager::getInstance();
	CDBConn *db_conn = manager->GetDBConn("tuchuang_slave");
	CacheManager *c_manager = CacheManager::getInstance();
	CacheConn *c_conn = c_manager->GetCacheConn("token");
	AUTO_REL_DBCONN(manager, db_conn);
	AUTO_REL_CACHECONN(c_manager, c_conn);

	int ret = 0;
	ret = getShareFilesCount(db_conn, c_conn, count);
	return ret;
}

int handleGetShareFilelist(int start,int count,string & str_json){
	int ret = 0;
	string str_sql;
	int total = 0;
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);
    CacheManager *cache_manager = CacheManager::getInstance();
    CacheConn *cache_conn = cache_manager->GetCacheConn("token");
    AUTO_REL_CACHECONN(cache_manager, cache_conn);

    CResultSet *result_set = NULL;
    int file_count = 0;
    Json::Value root, files;

    ret = getShareFilesCount(db_conn, cache_conn, total); // 获取文件总数量
    if (ret < 0) {
        LogError("getShareFilesCount err");
        ret = -1;
        goto END;
    } else {
        if (total == 0) {
            ret = 0;
            goto END;
        }
    }

    // sql语句
    str_sql = FormatString(
        "select share_file_list.*, file_info.url, file_info.size, file_info.type from file_info, \
        share_file_list where file_info.md5 = share_file_list.md5 limit %d, %d",
        start, count);
    LogInfo("执行: {}", str_sql);
    result_set = db_conn->ExecuteQuery(str_sql.c_str());
    if (result_set) {
        // 遍历所有的内容
        // 获取大小
        file_count = 0;
        while (result_set->Next()) {
            Json::Value file;
            file["user"] = result_set->GetString("user");
            file["md5"] = result_set->GetString("md5");
            file["file_name"] = result_set->GetString("file_name");
            file["share_status"] = result_set->GetInt("share_status");
            file["pv"] = result_set->GetInt("pv");
            file["create_time"] = result_set->GetString("create_time");
            file["url"] = result_set->GetString("url");
            file["size"] = result_set->GetInt("size");
            file["type"] = result_set->GetString("type");
            files[file_count] = file;
            file_count++;
        }
        if (file_count > 0)
            root["files"] = files;
        ret = 0;
        delete result_set;
    } else {
        ret = -1;
    }
END:
    if (ret == 0) {
        root["code"] = 0;
        root["total"] = total;
        root["count"] = file_count;
    } else {
        root["code"] = 1;
    }
    str_json = root.toStyledString();
	return ret;
}

void handleGetRankingFilelist(int start, int count, string &str_json){
	int ret = 0;
	char sql_cmd[SQL_MAX_LEN] = {0};
	int total = 0;
	char filename[1024] = {0};
	int sql_num;
	int redis_num;
	int score;
	int end;
	RVALUES value = NULL;
	Json::Value root;
	Json::Value files;
	int file_count = 0;

	CDBManager *db_manager = CDBManager::getInstance();
	CDBConn *db_conn = db_manager->GetDBConn("tuchuang_slave");
	AUTO_REL_DBCONN(db_manager, db_conn);
	CResultSet *pCResultSet = NULL;

	CacheManager *cache_manager = CacheManager::getInstance();
	CacheConn *cache_conn = cache_manager->GetCacheConn("token");
	AUTO_REL_CACHECONN(cache_manager, cache_conn);

	ret = getShareFilesCount(db_conn, cache_conn, total);
	
	if(ret != 0){
		LogError("操作失败{}",sql_cmd);
		ret = -1;
		goto END;
	}

	sql_num = total;

	redis_num = cache_conn->ZsetZcard(FILE_PUBLIC_ZSET);
	if(redis_num == -1){
		LogError("ZsetZcard failed\n");
		ret = -1;
		goto END;
	}

	LogInfo("sql_num:{}, redis_num:{}", sql_num, redis_num);

	if(redis_num != sql_num){
		cache_conn->Del(FILE_PUBLIC_ZSET);
		cache_conn->Del(FILE_NAME_HASH);

		strcpy(sql_cmd, "select md5, file_name, pv from share_file_list order by pv desc");
		LogInfo("执行：{}",sql_cmd);

		pCResultSet = db_conn->ExecuteQuery(sql_cmd);
		if(!pCResultSet){
			LogError("操作失败{}", sql_cmd);
			ret = -1;
			goto END;
		}

		while(pCResultSet->Next()){
			char field[1024] = {0};
			string md5 = pCResultSet->GetString("md5");
			string filename = pCResultSet->GetString("file_name");
			int pv = pCResultSet->GetInt("pv");
			sprintf(field, "%s%s", md5.c_str(), filename.c_str());
			cache_conn->ZsetAdd(FILE_PUBLIC_ZSET, pv, field);

			cache_conn->Hset(FILE_NAME_HASH, field, filename);
		}
	}

	value = (RVALUES)calloc(count, VALUES_ID_SIZE);
	if(value == NULL){
		ret = -1;
		goto END;
	}

	file_count = 0;
	end = start + count -1;
	ret = cache_conn->ZsetZrevrange(FILE_PUBLIC_ZSET, start, end, value, file_count);
	if(ret != 0){
		LogError("ZsetZrevrange 操作失败");
        ret = -1;
        goto END;
	}

	for(int i =0;i<file_count;++i){
		Json::Value file;
		ret = cache_conn->Hget(FILE_NAME_HASH, value[i], filename);
		if(ret!=0){
			LogError("hget failed\n");
			ret = -1;
			goto END;
		}
		file["filename"] = filename;
		score = cache_conn->ZsetGetScore(FILE_PUBLIC_ZSET, value[i]);
		if(score == -1){
			LogError("ZsetGetScore failed\n");
			ret = -1;
			goto END;
		}
		file["pv"] = score;
		files[i] = file;
		
	}
	if(file_count > 0){
		root["files"] = files;
	}
END:
	if(ret == 0){
		root["code"] = 0;
		root["total"] = total;
		root["count"] = file_count;
	}else{
		root["code"] = 1;
	}
	str_json = root.toStyledString();
}
int ApiSharefiles(string & url, string & post_data, string & str_json){
	char cmd[20];
	string user_name;
	string token;
	int start = 0;
	int count = 0;
	LogInfo("post_data:{}", post_data);

	QueryParseKeyValue(url.c_str(), "cmd", cmd, NULL);
	LogInfo("cmd={}",cmd);
	if(strcmp(cmd, "count") == 0){
		if(handleGetSharefilesCount(count) < 0){
			encodeSharefilesJson(1,0,str_json);
		}else{
			encodeSharefilesJson(0,count,str_json);
		}
		return 0;
	}else{
		if(decodeShareFileslistJson(post_data, start, count) < 0){
			encodeSharefilesJson(1,0,str_json);
			return 0;
		}
		if(strcmp(cmd, "normal") == 0){
			handleGetShareFilelist(start, count, str_json);
		}else if(strcmp(cmd, "pvdesc") == 0){
			handleGetRankingFilelist(start, count, str_json);
		}else{
			encodeSharefilesJson(1,0,str_json);
		}
	}
	return 0;
}
