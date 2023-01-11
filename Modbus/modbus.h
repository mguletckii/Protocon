//#ifndef MODBUS_H
//#define MODBUS_H

#include <baseprotocol/baseprotocol.h>
#include <baseprotocol/baseprotocol.cpp>

#include <modbus/modbus.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include <map>
#include <vector>
#include <dlfcn.h>
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define _8_to_16(a,b)    (int16_t)((a<<8)|(b))
#define _16_to_8_L(a)    (a&0xFF)
#define _16_to_8_H(a)    (a>>8)

#define NB_CONNECTION    5

modbus_t *ctx = NULL;
int server_socket = -1;

class modbus : public baseprotocol{

    private:
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        int master_socket;
        int rc;
        fd_set refset;
        fd_set rdset;
        /* Maximum file descriptor number */
        int fdmax;
        int query_length;
        uint8_t response[MODBUS_RTU_MAX_ADU_LENGTH];
        int data_start_index;
        int raw_length;
        int exception;
        int slave;
        int modbus_tcp_port;

        static void close_sigint(int dummy);

    public:
        modbus(std::string port);
        protocolerror       connect(),
                            reconnect(),
                            disconnect(),
                            query_response(protocolquery* response);

        protocolquery   query_out(),
                        query_in(protocolquery* query);
};

extern "C" baseprotocol* create(option_type options);

//#endif
