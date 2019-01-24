#ifndef MysqlConnector_H 
#define MysqlConnector_H
#include <iostream>
#include <string>
#include <limits.h>
#include <unistd.h>
#include <iomanip> 
#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include "LogBase.h"

using namespace std;

class MysqlConnector
{

public:
    MysqlConnector();
    ~MysqlConnector();
    bool initDB(string host,string user,string pwd,string db_name);
    bool exeSQL(string sql);
    bool exeQuery(string sql, uint8_t *sealed_data, uint32_t *sealed_data_len);
private:
    MYSQL *mysql;
    MYSQL_RES *g_query_result;
    MYSQL_ROW g_query_row;
};

#endif
