#include <iostream>
#include <cstring>
#include "MysqlConnector.h"

using namespace util;

MysqlConnector::MysqlConnector()
{
    mysql=mysql_init(NULL);
    if(mysql==NULL) {
        Log("Initial mysql failed!%s", mysql_error(mysql), log::error);
        return;
    }
}

MysqlConnector::~MysqlConnector()
{
    if(mysql!=NULL) {
        mysql_close(mysql);
    }
}


bool MysqlConnector::initDB(string host, string user, string passwd, string db_name)
{
    mysql = mysql_real_connect(mysql, host.c_str(), user.c_str(), passwd.c_str(), db_name.c_str(), 0, NULL, 0);  
    if(mysql == NULL) {  
        Log("Connect mysql failed!%s", mysql_error(mysql), log::error);
        return false;
    }  
    return true;  
}

bool MysqlConnector::exeQuery(string sql, uint8_t *sealed_data, uint32_t *sealed_data_len) {
    if (mysql_query(mysql,sql.c_str())) {
        Log("Query failed!%s", mysql_error(mysql), log::error);
        return false;
    } else {
        g_query_result = mysql_store_result(mysql);
        if (g_query_result) {
           int num_fields = mysql_num_fields(g_query_result);
           int num_rows = mysql_num_rows(g_query_result);
           g_query_row = mysql_fetch_row(g_query_result);
           if (g_query_row < 0 || num_fields != 2) {
                Log("Get sealed data failed!%s", mysql_error(mysql), log::error);
                return false;
           }
           unsigned long *fieldLength = mysql_fetch_lengths(g_query_result);
           uint32_t sealed_data_len = fieldLength[1];
           memcpy(sealed_data, (uint8_t*)g_query_row[1], sealed_data_len);
        } else {
            if (mysql_field_count(mysql) == 0)   //代表执行的是update,insert,delete类的非查询语句
            {
                // (it was not a SELECT)
                int num_rows = mysql_affected_rows(mysql);  //返回update,insert,delete影响的行数
            } else {
                Log("Get g_query_result error!%s", mysql_error(mysql), log::error);
                return false;
            }
        }
    }

    return true;
}

bool MysqlConnector::exeSQL(string sql)
{
    if (mysql_query(mysql,sql.c_str())) {
        Log("Query failed!%s", mysql_error(mysql), log::error);
        return false;
    } else {
        g_query_result = mysql_store_result(mysql);
        if (g_query_result) {
           int num_fields = mysql_num_fields(g_query_result);
           int num_rows = mysql_num_rows(g_query_result);
           for(int i=0;i<num_rows;i++) {
                g_query_row = mysql_fetch_row(g_query_result);
                if (g_query_row < 0) break;

                for(int j=0;j<num_fields;j++) {
                    cout<<g_query_row[j]<<"\t\t";
                }
                cout<<endl;
           }

        } else {
            if (mysql_field_count(mysql) == 0)   //代表执行的是update,insert,delete类的非查询语句
            {
                // (it was not a SELECT)
                int num_rows = mysql_affected_rows(mysql);  //返回update,insert,delete影响的行数
            } else {
                Log("Get g_query_result error!%s", mysql_error(mysql), log::error);
                return false;
            }
        }
    }

    return true;
}
