#include <Modbus/modbus.h>


void modbus::close_sigint(int dummy){
            if (server_socket != -1) {
                close(server_socket);
            }
            modbus_free(ctx);


            exit(dummy);
}

protocolerror modbus::connect(){
    try{
        ctx = modbus_new_tcp(NULL, modbus_tcp_port);

        server_socket = modbus_tcp_listen(ctx, NB_CONNECTION);

        signal(SIGINT, close_sigint);

        /* Clear the reference set of socket */
        FD_ZERO(&refset);
        /* Add the server socket */
        FD_SET(server_socket, &refset);

        /* Keep track of the max file descriptor */
        fdmax = server_socket;
        return protocolerror();
    }catch(...){
        return protocolerror(true,"Connect error!");
    }

}

protocolerror modbus::disconnect(){
    try{
        modbus_close(ctx);
        return protocolerror();
    }catch(...){
        return protocolerror(true,"Disconnect error!");
    }

}

protocolerror modbus::reconnect(){
    try{
        modbus::disconnect();
        modbus::connect();
    }catch(...){
        return protocolerror(true,"Reconnect error!");
    }

}

modbus::modbus(std::string port){
    modbus_tcp_port = std::stoi(port);
}

protocolquery modbus::query_out(){

    protocolquery querydata;

    rdset = refset;

    if (select(fdmax+1, &rdset, NULL, NULL, NULL) == -1) {

        perror("Server select() failure.");
        close_sigint(1);
    }

    master_socket++;
    if(master_socket>fdmax) master_socket = 0;

    if (!FD_ISSET(master_socket, &rdset)) return querydata;

    if (master_socket == server_socket) {
        /* A client is asking a new connection */

        socklen_t addrlen;
        struct sockaddr_in clientaddr;
        int newfd;

        /* Handle new connections */
        addrlen = sizeof(clientaddr);
        memset(&clientaddr, 0, sizeof(clientaddr));

        newfd = accept(server_socket, (struct sockaddr *)&clientaddr, &addrlen);


        if (newfd == -1) {
            perror("Server accept() error");
        } else {
            FD_SET(newfd, &refset);

            if (newfd > fdmax) {
                /* Keep track of the maximum */
                fdmax = newfd;
            }
            printf("New connection from %s:%d on socket %d\n",
                   inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port, newfd);
        }

    }else{
        modbus_set_socket(ctx, master_socket);
        rc = modbus_receive(ctx, query);
        if (rc > 0) {
            exception = 0;
            query_length = rc;
            data_start_index = modbus_get_header_length(ctx) - 1;
            slave = query[data_start_index];
            raw_length = rc - data_start_index - modbus_get_checksum_length(ctx);

            switch (query[7]){
                case 16:{
                    for(int i = 0; i<_8_to_16(query[10],query[11]); i++){
                        querydata.data.insert(std::pair<std::string,std::string>(std::to_string(_8_to_16(query[8],query[9])+i),std::to_string(_8_to_16(query[13+2*i],query[14+2*i]))));
                    }
                    querydata.command = "write";
                    querydata.object = "ModBus";
                    break;
                }
                case 6:{
                        querydata.command = "write";
                        querydata.object = "ModBus";
                        querydata.data = {{std::to_string(_8_to_16(query[8],query[9])),std::to_string(_8_to_16(query[10],query[11]))}};
                        break;
                }
                case 3:{
                }
                case 4:{
                        for(int i = 0; i<_8_to_16(query[10],query[11]); i++){
                            querydata.data.insert(std::pair<std::string,std::string>(std::to_string(_8_to_16(query[8],query[9])+i),""));
                        }
                        querydata.command = "read";
                        querydata.object = "ModBus";
                        break;
                }
                default: std::cout << "No Case! Query: " << query << std::endl; modbus_reply_exception(ctx,query,1); break;
            }

            if (exception != 0) {
                if (exception > MODBUS_ENOBASE && MODBUS_ENOBASE < (MODBUS_ENOBASE + MODBUS_EXCEPTION_MAX)) {
                    exception -= MODBUS_ENOBASE;
                } else {
                    exception = EMBXSFAIL;
                }
                std::cout << "Sending exception: "<< exception << " Query: " << query << std::endl;
                modbus_reply_exception(ctx, query, exception);
            }
        } else if (rc == -1) {
            /* This example server in ended on connection closing or
             * any errors. */
            printf("Connection closed on socket %d\n", master_socket);
            close(master_socket);

            /* Remove from reference set */
            FD_CLR(master_socket, &refset);

            if (master_socket == fdmax) {
                fdmax--;
            }
        }
    }
    return querydata;
}

protocolerror modbus::query_response(protocolquery* response){
    try{
        switch (query[7]){
            case 16:{}
            case 6:{
                modbus_reply_raw_response(ctx, query, query_length, query + data_start_index, 6);
                break;
            }
            case 3:{
            }
            case 4:{
                if((*response).data.size() != _8_to_16(query[10],query[11])){
                    modbus_reply_exception(ctx,query,2);
                    break;
                }

                query_length += ((*response).data.size()*2)+1;
                int start_registr = _8_to_16(query[8],query[9]);
                int registers = _8_to_16(query[10],query[11]);
                query[8] = _8_to_16(query[10],query[11])*2;

                for(auto it = (*response).data.begin(); it != (*response).data.end(); ++it){
                    for(int i = 0; i <registers;i++){
                        if(std::stoi((*it).first) == start_registr+i){
                            query[9+2*i] = _16_to_8_H(std::stoi((*it).second));
                            query[10+2*i] = _16_to_8_L(std::stoi((*it).second));
                        }
                    }
                }
                modbus_reply_raw_response(ctx, query, query_length, query + data_start_index, query_length);
                break;
            }
        }

        return protocolerror();

    }
    catch(...){
        return protocolerror(true);
    }
}

protocolquery modbus::query_in(protocolquery* query){
    return protocolquery();
}



extern "C" baseprotocol* create(option_type options){
    if (options["port"] == "") options["port"] = "502";
    return new modbus(options["port"]);
}
