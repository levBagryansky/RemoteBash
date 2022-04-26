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
    char buf[MAXLINE] = {0};

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("192.168.1.153");

    if(argc == 2 && (!strcmp(argv[1], "UDP") || !strcmp(argv[1], "udp"))) {
        printf("UDP mode\n");
        int sock_fd;
        // Creating socket file descriptor
        if ( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            perror("socket creation failed");
            exit(EXIT_FAILURE);
        }

        int n, len;
        while (1){
            n = read(STDIN_FILENO, buf, MAXLINE);
            printf("buf = %s", buf);
            if(!strcmp(buf, "-quit\n")){
                break;
            }
            sendto(sock_fd, buf, n, MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr);
            memset(buf, 0, sizeof(buf));
            n = recvfrom(sock_fd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serv_addr, &len);
            write(STDOUT_FILENO, buf, n);
            memset(buf, 0, sizeof(buf));
        }


    } else {
        printf("TCP mode\n");
        int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd == -1) {
            printf("sock_fd == -1\n");
            exit(1);
        }
        printf("connecting..\n");
        int connect_fd = connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if (connect_fd < 0) {
            printf("connect_fd == %d\n", connect_fd);
            return 1;
        }

        printf("connect_fd == %d\n", connect_fd);
        int n;
        while (1) {
            n = read(STDIN_FILENO, buf, MAXLINE);
            write(sock_fd, buf, n);
            memset(buf, 0, MAXLINE);
            n = read(sock_fd, buf, MAXLINE);
            printf("\n");
            write(STDOUT_FILENO, buf, n);
            memset(buf, 0, MAXLINE);
        }
    }
}
