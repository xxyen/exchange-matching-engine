#include "server.hpp"

int Server::connect2Client(){
    // std::cout<<socket_fd<<std::endl;
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd;
    // std::cout<<socket_fd<<std::endl;
    // printf("before accept\n");
    // std::cout<<socket_fd<<std::endl;
    client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    // std::cout<<client_connection_fd;
    if (client_connection_fd == -1) {
        std::cerr << "Error: cannot accept connection on socket" << std::endl;
        return -1;
    } //if

    return client_connection_fd;
}

 int Server::buildServer(){
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;
    
    // printf("before");

    int status;
    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        std::cerr << "Error: cannot get address info for host" << std::endl;
        std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
        return EXIT_FAILURE;
    } //if
    // printf("getaddrinfo\n");

    socket_fd = socket(host_info_list->ai_family, 
                host_info_list->ai_socktype, 
                host_info_list->ai_protocol);
    // std::cout<<socket_fd<<std::endl;
    if (socket_fd == -1) {
        std::cerr << "Error: cannot create socket" << std::endl;
        std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
        return EXIT_FAILURE;
    } //if
    // printf("socket\n");

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        std::cerr << "Error: cannot bind socket" << std::endl;
        std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
        return EXIT_FAILURE;
    } //if

    status = listen(socket_fd, 512);
    if (status == -1) {
        std::cerr << "Error: cannot listen on socket" << std::endl; 
        std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
        return EXIT_FAILURE;
    } //if
    // printf("listen\n");
    return EXIT_SUCCESS;
}

void Server::run(){
    int ready = buildServer();
    // cout<<"starting..."<<endl;
    if(ready == EXIT_FAILURE){
        perror("Cannot connect\n");
        exit(EXIT_FAILURE);
    }

    while (true) {
        int client_fd = connect2Client();
        char request[512];
        recv(client_fd, request, 512, 0);
        // parse XML TBD: 需要判断收到的长度啥的

        string response = xxxx;
        const char *response_xml = response.c_str();
        send(client_fd, response_xml, strlen(response_xml),0);

        freeaddrinfo(host_info_list);
        close(socket_fd);
    }
}