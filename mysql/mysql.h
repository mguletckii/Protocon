#ifndef MYSQL_H
#define MYSQL_H

#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <baseprotocol/baseprotocol.h>
#include <baseprotocol/baseprotocol.cpp>

#include <map>
#include <vector>
#include <dlfcn.h>

#include <thread>
#include <ctime>

class mysql : public baseprotocol{
    private:
        std::string _ip,
                    _port,
                    _user,
                    _pass,
                    _base,
                    _table,
                    _columns,
                    _identifier,
                    _value,
                    _update;

        sql::Driver *driver;
        sql::Connection *con;

        sql::SQLException   execute(std::string query);

        std::map <std::string,std::string> res_to_map(sql::ResultSet *res),
                                            execute_query(std::string query);

        std::string     STR_SELECT(std::string base_name, std::string table_name, std::string columns),
                        STR_SELECT_WHERE(std::string base_name, std::string table_name, std::string columns,std::string where),
                        STR_New_Table(std::string base_name, std::string table_name, std::string columns),
                        STR_New_Table_ModBus(std::string base_name, std::string table_name),
                        STR_New_Rows(std::string base_name, std::string table_name, int how),
                        STR_INSERT_INTO(std::map<std::string,std::string> *data);

    public:
        mysql(std::string ip, std::string port, std::string user, std::string pass, std::string base,std::string table,std::string columns);

        protocolerror       connect(),
                            reconnect(),
                            disconnect(),
                            query_response(protocolquery* response);

        protocolquery   query_out(),
                        query_in(protocolquery* query);

};

extern "C" baseprotocol* create(option_type options);

#endif MYSQL_H
