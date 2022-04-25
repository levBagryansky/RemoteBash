#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLINE 1024
#define PORT 1e4

int main(int argc, char** argv) {
    char buf[1024] = {0};

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd == -1){
        printf("sock_fd == -1\n");
        exit(1);
    }
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(1e4);
    serv_addr.sin_addr.s_addr = inet_addr("192.168.1.153");
    int connect_fd = connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if(connect_fd < 0){
        printf("connect_fd == %d\n", connect_fd);
        exit(1);
    }
    printf("connect_fd == %d\n", connect_fd);
    int n;
    while (1){
        n = read(STDIN_FILENO, buf, 256);
        write(sock_fd, buf, n);
    }
}