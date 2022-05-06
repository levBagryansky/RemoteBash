#include "StringFunctions.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define START_PORT      1e4
#define EXIT_CODE -2

#define TRY(cmd) \
  if (cmd < 0) \
  {              \
    perror(#cmd); \
    return(-1); \
  }

//int port;
struct sockaddr_in serv_addr;
int udp_flag = 0;
socklen_t serv_addr_size = sizeof (serv_addr);

int DetermineUserAndIpByStr(char *user_ip_str, char *user, char *ip);
int Send2Server(char *buf, int sock_fd, int n);
int RcvAndWrite(char *buf, int sock_fd);
int CP2Server(char *str, int n, int sock_fd);
int PrintServersByBroadcast();
int BroadcastFirstConnect(int *p_port, char *ip);
int DoCommunication(char* buf, char *login);

// Must be
// ./Client [-t <UDP|TCP>] [<user>@]<IP>
// ./Client —broadcast
int main(int argc, char** argv) {
    char buf[MAXLINE] = {0};
    if(argc < 2 || argc > 3){
        printf("Wring format\n");
        return 0;
    }

    if(!strcmp(argv[1], "--broadcast") || !strcmp(argv[1], "--b")){
        PrintServersByBroadcast();
        return 0;
    }

    char *user_ip_str = argv[1];
    if(!strcmp(argv[1], "-t")){
        if(!strcmp(argv[1], "udp") || !strcmp(argv[1], "UDP")){
            udp_flag++;
        } else if(!strcmp(argv[1], "tcp") || !strcmp(argv[1], "TCP")){
            udp_flag = 0;
        } else{
            printf("Wrong format\n");
            return 0;
        }
        user_ip_str = argv[3];
    }

    char *user_str = (char *) calloc(MAXLINE, sizeof(char));
    char *ip_str = (char *) calloc(MAXLINE, sizeof(char));
    if(DetermineUserAndIpByStr(user_ip_str, user_str, ip_str) == -1){
        free(user_ip_str);
        free(ip_str);
        exit(1);
    }

    if(user_str[0] == 0 || ip_str[0] == 0){
        printf("Wrong user or ip\n");
        free(user_ip_str);
        free(ip_str);
        return 0;
    }

    int port;
    BroadcastFirstConnect(&port, ip_str);
    serv_addr.sin_port = htons(port);
    printf("freeing\n");
    //free(user_str);
    free(ip_str);
    printf("freed");
    if(argc == 2 && (!strcmp(argv[1], "UDP") || !strcmp(argv[1], "udp"))) {
        udp_flag++;
    } else {
    }
    DoCommunication(buf, user_str);
}

int DetermineUserAndIpByStr(char *user_ip_str, char *user, char *ip){
    char *at_sign = strchr(user_ip_str, '@');
    if(at_sign){
        for (int i = 0; i < at_sign - user_ip_str; ++i) {
            user[i] = user_ip_str[i];
        }
        int num_of_at_sign = at_sign  - user_ip_str;
        for (int i = 0; user_ip_str[i + num_of_at_sign + 1] != 0; ++i) {
            ip[i] = user_ip_str[i + num_of_at_sign + 1];
        }
    } else{
        TRY(getlogin_r(user, MAXLINE))
        for (int i = 0; user_ip_str[i] != 0 && i < MAXLINE; ++i) {
            ip[i] = user_ip_str[i];
        }
    }
    printf("user = %s\nip = %s\n", user, ip);
    return 0;
}

int Send2Server(char *buf, int sock_fd, int n){
    int ret;
    if(udp_flag) {
        ret = sendto(sock_fd, buf, n, MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr);
    }
    else{
        ret = write(sock_fd, buf, n);
    }
    return ret;
}

int RcvAndWrite(char *buf, int sock_fd){
    int n;
    if(udp_flag){
        n = recvfrom(sock_fd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serv_addr, (socklen_t *)&serv_addr_size);
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

int CP2Server(char *str, int n, int sock_fd){
    char path_from[MAXLINE];
    GetNumWord(str, path_from, 1);
    int path_from_fd = open(path_from, O_RDONLY);
    if(path_from_fd == -1){
        perror("Open error");
        return -1;
    }

    int file_len = GetFileSize(path_from_fd);
    if(file_len < 0){
        printf("GetFileSize returned %d\n", file_len);
        return file_len;
    }
    char *text = (char *) calloc(file_len, sizeof (char));
    if(text == NULL){
        perror("calloc error");
        return -1;
    }
    int num_of_send = (file_len + MAXLINE - 1) / MAXLINE;
    if(read(path_from_fd, text, file_len) == -1){
        perror("read error");
        return -1;
    }
    Send2Server(str, sock_fd, n);

    if(udp_flag) {
        sendto(sock_fd, &num_of_send, sizeof(int), MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr);
    }
    else{
        write(sock_fd, &num_of_send, sizeof(int));
    }
    char buf[MAXLINE];
    for (int i = 0; i < (file_len - 1) / MAXLINE; ++i) {
        for (int j = 0; j < MAXLINE; ++j) {
            buf[j] = text[MAXLINE * i + j];
        }
        TRY(Send2Server(buf, sock_fd, MAXLINE))
    }
    for (int i = 0; i < (file_len - 1) % MAXLINE + 1; ++i) {
        buf[i] = text[MAXLINE * (file_len / MAXLINE) + i];
    }
    TRY(Send2Server(buf, sock_fd, file_len % MAXLINE))
    close(path_from_fd);

    return 0;
}

int PrintServersByBroadcast(){
    int exit_after_distributed = 1;
    printf("Getting available servers..\n");
    memset(&serv_addr, 0, sizeof(serv_addr));

    int sock_fd;
    if ( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(START_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    int a = 1;
    TRY(setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &a, sizeof(a)))
    //bind(sock_fd, (struct sockaddr*) &serv_addr, sizeof (serv_addr));

    TRY(sendto(sock_fd, &exit_after_distributed, sizeof(int), MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr))

    // получаем
    int port;
    socklen_t len = sizeof(serv_addr);
    TRY(recvfrom(sock_fd, &port, sizeof(port), MSG_WAITALL, (struct sockaddr *) &serv_addr, &len))

    printf("port = %d\n", port);
    printf("See server: ip = %s\n", inet_ntoa(serv_addr.sin_addr));
    close(sock_fd);
    return 0;
}

int BroadcastFirstConnect(int *p_port, char* ip){
    printf("Broadcast connecting..\n");
    memset(&serv_addr, 0, sizeof(serv_addr));

    int sock_fd;
    if ( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(START_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    int a = 1;
    TRY(setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &a, sizeof(a)))
    int exit_after_distributed = 0;
    TRY(sendto(sock_fd, &exit_after_distributed, sizeof(exit_after_distributed), MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr))

    // получаем
    socklen_t len = sizeof(serv_addr);
    while (1) {
        printf("RECEIVING\n");
        TRY(recvfrom(sock_fd, p_port, sizeof(int), MSG_WAITALL, (struct sockaddr *) &serv_addr, &len))
        //printf("ip = %d\nserver_ip = %d\n", htonl(ip), serv_addr.sin_addr.s_addr);
        if(serv_addr.sin_addr.s_addr == inet_addr(ip)){
            break;
        }
        printf("Not equal\n");
    }
    printf("port = %d\n", *p_port);
    printf("Yeah! Get answer from server: ip = %s\n", inet_ntoa(serv_addr.sin_addr));
    close(sock_fd);
    return 0;
}

int DoCommunication(char* buf, char *login){
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
            perror("connect error");
            return 1;
        }
        printf("connect_fd = %d\n", connect_fd);
    }
    printf("sock_fd = %d\nNow We have to be authorized\n", sock_fd);
    Send2Server(login, sock_fd, strlen(login));
    free(login);
    int forked = fork();
    if(forked) {
        while (1) {
            int n = read(STDIN_FILENO, buf, MAXLINE);
            if(CP_CommandDetected(buf)){
                CP2Server(buf, n, sock_fd);
                printf("You can continue\n");
                goto L;
            }
            if(Send2Server(buf, sock_fd, n) == -1){
                printf("Send2Server error\n");
                break;
            }
            if (!strcmp(buf, "exit\n")) {
                printf("Sender exits\n");
                break;
            }
            L:
            memset(buf, 0, MAXLINE);
        }
    } else{
        while (1) {
            if(RcvAndWrite(buf, sock_fd) == EXIT_CODE){
                break;
            }
        }
    }
    return 0;
}
