#include <iostream>
#include <dlfcn.h>
#include <string.h>
#include "baseprotocol/baseprotocol.h"
#include "configparser/configparser.h"
#include "searchlib/searchlib.h"


using namespace std;

int main(int argc, char* argv[])
{

    string path = argv[0];
    path.erase(path.rfind("/")+1,path.size());

    if (argc >= 3){
        cout<<"First protocol: "<<getlowerstr(argv[1])<<endl;
        cout<<"Second protocol: "<<getlowerstr(argv[2])<<endl;

        baseprotocol* protocol_first;
        baseprotocol* protocol_second;
        baseprotocol* (*create)(option_type);

        void * sharedlib = dlopen(getlib(path,argv[1]).c_str(), RTLD_NOW);

        if (!sharedlib) {
            cerr << "Cannot open library: " << dlerror() << '\n';
            return 1;
        }

        create = (baseprotocol* (*)(option_type))dlsym(sharedlib,"create");
        try{
            protocol_first = create(config_parse(path+"config.cfg",argv[1]));
        }
        catch(...){
            cout<<"Cannot create first protocol object"<<endl;
            return 1;
        }

        sharedlib = dlopen(getlib(path,argv[2]).c_str(), RTLD_NOW);

        if (!sharedlib) {
            cerr << "Cannot open library: " << dlerror() << '\n';
            return 1;
        }

        create = (baseprotocol* (*)(option_type))dlsym(sharedlib,"create");
        try{
            protocol_second = create(config_parse(path+"config.cfg",argv[2]));
        }
        catch(...){
            cout<<"Cannot create second protocol object"<<endl;
            return 1;
        }

        protocolerror connect = protocol_first->connect();
        if(connect.error){
            cout<<"First Protocol Connection Error!"<<" "<<connect.text<<" "<<connect.number<<endl;
            return 1;
        }

        connect = protocol_second->connect();
        if(connect.error){
            cout<<"Second Protocol Connection Error!"<<" "<<connect.text<<" "<<connect.number<<endl;
            return 1;
        }

        cout<<"Connection Done!"<<endl;

        protocolquery data;
        protocolerror reconnect;

        while(1 == 1){
            try{

                data = protocol_first->query_out();
                if (data.command != ""){

                    if (data.command.find("error")!= std::string::npos){
                        cout<<"Error "<<argv[1]<<":"<<data.command<<endl;
                        cout<<"Try to reconnect"<<endl;
                        reconnect = protocol_first->reconnect();
                        if (reconnect.error){
                            cout<<"Error "<<argv[1]<<":"<<reconnect.text<<endl;
                        }
                    }

                    data = protocol_second->query_in(&data);

                    if (data.command.find("error")!= std::string::npos){
                        cout<<"Error "<<argv[2]<<":"<<data.command<<endl;
                        cout<<"Try to reconnect"<<endl;
                        reconnect = protocol_second->reconnect();
                        if (reconnect.error){
                            cout<<"Error "<<argv[1]<<":"<<reconnect.text<<endl;
                        }
                    }


                    protocol_first->query_response(&data);
                }

            }
            catch(...){
                delete protocol_first;
                delete protocol_second;

                cout<<"Error in infinity cyclic!!!"<<endl;
                return 1;
            }
        }

    }else{
        cout<<"Not enough arguments!"<<endl;
    }
    return 0;
}
