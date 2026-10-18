#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 1024

/* This function runs in the background as the Server */
void run_server() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char *hello = "Hello from server";

    /* 1. socket() */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed"); exit(EXIT_FAILURE);
    }

    /* Allow immediate reuse of the port */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    /* 2. bind() */
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed"); exit(EXIT_FAILURE);
    }
    
    /* 3. listen() */
    if (listen(server_fd, 3) < 0) {
        perror("listen"); exit(EXIT_FAILURE);
    }

    /* 4. accept() - waits for client */
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept"); exit(EXIT_FAILURE);
    }

    /* 5. recv() */
    read(new_socket, buffer, BUFFER_SIZE);
    
    /* 6. send() */
    send(new_socket, hello, strlen(hello), 0);
    
    close(new_socket);
    close(server_fd);
    exit(0); /* Exit child process */
}

/* This function runs as the Client */
void run_client() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";
    char buffer[BUFFER_SIZE] = {0};

    /* 1. socket() */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error"); exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    /* 2. connect() */
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed"); exit(EXIT_FAILURE);
    }

    /* 3. send() */
    send(sock, hello, strlen(hello), 0);
    
    /* 4. recv() */
    read(sock, buffer, BUFFER_SIZE);
    
    close(sock);
}

int run_netdemo(void) {
    printf("=== Module D: Distributed Systems & Network Syscalls ===\n\n");

    /* Fork the server into the background */
    pid_t server_pid = fork();
    if (server_pid == 0) {
        run_server();
    }

    /* Give the server 1 second to start listening */
    sleep(1);

    /* Run the client */
    printf("Client connecting to localhost...\n");
    run_client();
    
    /* Wait for server to finish */
    waitpid(server_pid, NULL, 0);
    printf("Network exchange complete.\n\n");

    /* --- SECURITY ANALYSIS REPORT --- */
    printf("=== SECURITY ANALYSIS (Distributed Systems) ===\n");
    printf("1. No Authentication: The server accepted the connection via accept()\n");
    printf("   without verifying who the client is.\n");
    printf("2. Plaintext Data: The send() and recv() syscalls transmitted data in\n");
    printf("   raw bytes. Anyone sniffing the network can read the message.\n");
    printf("3. No Encryption: There is no TLS/SSL layer. A real distributed system\n");
    printf("   must use secure sockets.\n");
    printf("4. Privilege: The server ran as user 'eman'. If it ran as 'root', a\n");
    printf("   vulnerability would mean full system compromise.\n");
    printf("5. Race Conditions: If two clients connected at the exact same time,\n");
    printf("   the single-threaded server would drop one connection.\n");

    return 0;
}
