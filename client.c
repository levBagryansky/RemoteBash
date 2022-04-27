#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLINE   1024
#define PORT      1e4
#define EXIT_CODE -2

struct sockaddr_in serv_addr;
int udp_flag = 0;
int len = sizeof (serv_addr);
int CP2Server();

int BroadcastFirstConnect(){
    printf("Broadcast connecting..\n");
    char buf[MAXLINE] = "BroadcastMess";
    memset(&serv_addr, 0, sizeof(serv_addr));

    int sock_fd;
    if ( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    int a = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &a, sizeof(a));
    //bind(sock_fd, (struct sockaddr*) &serv_addr, sizeof (serv_addr));

    sendto(sock_fd, buf, strlen(buf), MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr);

    // получаем
    socklen_t len = sizeof(serv_addr);
    recvfrom(sock_fd, buf, sizeof(buf), MSG_WAITALL, (struct sockaddr *) &serv_addr, &len);
    printf("Yeah! Get answer from server: ip = %s\n", inet_ntoa(serv_addr.sin_addr));
    close(sock_fd);
    return 0;
}

int ReadAndSend2Server(char *buf, int sock_fd){
    int n = read(STDIN_FILENO, buf, MAXLINE);
    int ret;
    if(udp_flag) {
        ret = sendto(sock_fd, buf, n, MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr);
    }
    else{
        ret = write(sock_fd, buf, n);
    }
    if (!strcmp(buf, "exit\n")) {
        printf("Sender exits\n");
        return EXIT_CODE;
    }
    memset(buf, 0, MAXLINE);
    return ret;
}

int RcvAndWrite(char *buf, int sock_fd){
    int n;
    if(udp_flag){
        n = recvfrom(sock_fd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serv_addr, (socklen_t *)&len);
    } else{
        n = read(sock_fd, buf, MAXLINE);
    }
    if (!strcmp(buf, "exit\n")) {
        printf("Recver exits\n");
        return EXIT_CODE;
    }
    write(STDOUT_FILENO, buf, n);
    memset(buf, 0, MAXLINE);
    return n;
}

int DoCommunication(char* buf){
    int sock_fd;
    if(udp_flag){
        printf("UDP mode\n");
        sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    } else{
        printf("TCP mode\n");
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (sock_fd == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    printf("connecting..\n");
    int len = sizeof (serv_addr);
    if(udp_flag){
        char *str = "I know client datas\n\0";
        sendto(sock_fd, str, 21, MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr);
        int n = recvfrom(sock_fd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serv_addr, (socklen_t *)&len);
        write(STDOUT_FILENO, buf, n);
    } else{ //TCP
        int connect_fd = connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if (connect_fd < 0) {
            printf("connect_fd == %d\n", connect_fd);
            return 1;
        }
        printf("connect_fd = %d\n", connect_fd);
    }
    printf("sock_fd = %d\nConnected, you can start\n", sock_fd);

    int forked = fork();
    if(forked) {
        while (1) {
            if(ReadAndSend2Server(buf, sock_fd) == EXIT_CODE){
                break;
            }
        }
    } else{
        while (1) {
            if(RcvAndWrite(buf, sock_fd) == EXIT_CODE){
                break;
            }
        }
    }

}

int main(int argc, char** argv) {
    char buf[MAXLINE] = {0};

    BroadcastFirstConnect();
    serv_addr.sin_port = htons(PORT);

    if(argc == 2 && (!strcmp(argv[1], "UDP") || !strcmp(argv[1], "udp"))) {
        udp_flag++;
        //DoUDP(buf);
    } else {
        //DoTCP(buf);
    }
    DoCommunication(buf);
}
