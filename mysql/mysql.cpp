#include <mysql/mysql.h>


mysql::mysql(std::string ip, std::string port, std::string user, std::string pass, std::string base,std::string table,std::string columns){
    _ip = ip;
    _port = port;
    _user = user;
    _pass = pass;
    _base = base;
    _table = table;
    _columns = columns;
    _identifier = _columns.substr(0,_columns.find(','));
    _value = _columns.substr(_columns.find(',')+1,_columns.rfind(',')-_identifier.size()-1);
    _update = _columns.substr(_columns.rfind(',')+1,_columns.size());
    _columns = _identifier +","+_value;

}

protocolerror mysql::connect(){
    driver = sql::mysql::get_driver_instance();
    try{
        con = driver->connect("tcp://"+_ip+":"+_port,_user,_pass);
    }
    catch(sql::SQLException &e){
        return protocolerror(true,e.getSQLStateCStr(),e.getErrorCode());
    }
    return protocolerror();
}

protocolerror mysql::reconnect(){
    protocolerror return_error;
    try{
        return_error = disconnect();
    }
    catch(...){
        return_error.error = true;
        return_error.text = "Reconnect error. Disconnect";
    }
    try{
        return_error = connect();
    }
    catch(...){
        return_error.error = true;
        return_error.text = "Reconnect error. Connect";
    }
    return return_error;
}

protocolerror mysql::disconnect(){
    if(!con->isValid()){
        try{
            con->close();
        }
        catch(sql::SQLException &e){
            return protocolerror(true,e.getSQLStateCStr(),e.getErrorCode());
        }
    }
    return protocolerror();
}

sql::SQLException mysql::execute(std::string query){
        try{
            sql::Statement* stmt = con->createStatement();
            stmt->execute(query);
            delete stmt;
        }
        catch(sql::SQLException &e){
            return e;
        }
    return sql::SQLException();
}


std::map <std::string,std::string> mysql::res_to_map(sql::ResultSet *res){
    std::map <std::string,std::string> returning_map;
    try{
        while((*res).next()){
            returning_map.insert(std::pair<std::string,std::string>((*res).getString(1),(*res).getString(2)));
        }
    }
    catch(...){}
    return returning_map;
}

std::map <std::string,std::string> mysql::execute_query(std::string query){
    try{
            sql::Statement* stmt = con->createStatement();
            sql::ResultSet *res = stmt->executeQuery(query);
            std::map <std::string,std::string> return_map = res_to_map(res);
            delete stmt;
            delete res;

            return return_map;
    }
    catch(sql::SQLException &e){
            std::cout << "mysql::execute_query error" << std::endl;
            std::cout << "# ERR: " << e.what() << std::endl;
    }
}

protocolquery mysql::query_in(protocolquery* query){
    protocolquery querydata;
    if((*query).command == "write"){
        try{
            execute(STR_INSERT_INTO(&(query->data)));
            querydata.command = (*query).command;
            querydata.object = (*query).object;
        }
        catch(...){
            querydata.command = "Write error";
        }
    }


    if((*query).command == "read"){
        try{
            std::string where_str;
            for(auto i=(*query).data.begin(); i != (*query).data.end(); ++i){
                where_str += "("+_identifier+"="+(*i).first+")OR";
            }
            where_str.erase(where_str.size()-2);
            querydata.command = (*query).command;
            querydata.object = (*query).object;
            querydata.data = execute_query(STR_SELECT_WHERE(_base,_table,_columns,where_str));
        }
        catch(...){
            querydata.command = "Read error";
        }
    }

    if(querydata.command != "") return querydata;
    querydata.command = "Command missing";

    return querydata;
}

protocolquery mysql::query_out(){
    return protocolquery();
}

protocolerror mysql::query_response(protocolquery* response){
    return protocolerror();
}

std::string mysql::STR_INSERT_INTO(std::map<std::string,std::string> *data){

    std::string return_str;

    return_str = "INSERT INTO "+_base+"."+_table+" ("+_identifier+","+_value+",`"+_update+"`) VALUES ";

    for (auto it = (*data).begin(); it != (*data).end(); ++it)
    {
        return_str += "("+(*it).first+",'"+(*it).second+"',NULL),";
    }

    return_str.erase(return_str.size()-1);


    return return_str+" ON DUPLICATE KEY UPDATE "+_value+" = VALUES ("+_value+"),`"+_update+"`=NULL";
}

std::string mysql::STR_SELECT(std::string base_name, std::string table_name, std::string columns){
    return "SELECT "+columns+" FROM "+base_name+"."+table_name;
}

std::string mysql::STR_SELECT_WHERE(std::string base_name, std::string table_name, std::string columns,std::string where){
    return "SELECT "+columns+" FROM "+base_name+"."+table_name +" WHERE "+ where;
}

std::string mysql::STR_New_Table(std::string base_name, std::string table_name, std::string columns){
    return "CREATE TABLE " + base_name + "." + table_name + " ("+columns+");";
}

std::string mysql::STR_New_Table_ModBus(std::string base_name, std::string table_name){
    return STR_New_Table(base_name,table_name,"`ID` INT NOT NULL AUTO_INCREMENT, `Value` INT NOT NULL DEFAULT '0', `Update` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, PRIMARY KEY (`ID`)");
}

std::string mysql::STR_New_Rows(std::string base_name, std::string table_name, int how){
    std::string return_str = "INSERT INTO `" + base_name + "`.`" + table_name + "` () VALUES ";
    for (int i = 0; i < how; i++)
    {
        return_str += "()";
        if (i < how - 1)
        {
            return_str += ",";
        }
    }
    return_str += ";";

    return return_str;
}

extern "C" baseprotocol* create(option_type options){
    if(options["ip"]=="") options["ip"] = "127.0.0.1";
    if(options["port"]=="") options["port"] = "3306";
    if(options["login"]=="") options["login"] = "root";
    if(options["password"]=="") options["ip"] = "root";
    if(options["base"]=="") options["base"] = "Scada";
    if(options["table"]=="") options["table"] = "Value";
    if(options["columns"]=="") options["columns"] = "ID,Value,Update";

    return new mysql(options["ip"],options["port"],options["login"],options["password"],options["base"],options["table"],options["columns"]);
}
