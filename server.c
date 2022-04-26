#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#define MAXLINE 1024

int udp_flag = 0;
struct sockaddr_in cli_addr;

//Коннектится с юзером
int ConnectWithUser(){
    struct sockaddr_in serv_addr;
    int sock_fd;
    if(udp_flag){
        sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    }
    else {
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (sock_fd == -1) {
        perror("sock_fd == -1, exit");
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_addr.s_addr = inet_addr("192.168.1.153");
    serv_addr.sin_port = htons(1e4);

    bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    printf("binded\n");
    if(udp_flag){
        char buf[MAXLINE] = {0};
        int len = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        int n = recvfrom(sock_fd, (char *) buf, MAXLINE,
                         MSG_WAITALL, (struct sockaddr *) &cli_addr, (socklen_t *) &len);
        write(STDOUT_FILENO, buf, MAXLINE);
        char* str = "Got answer from server\n";
        int sended = sendto(sock_fd, str, 24, MSG_CONFIRM, (const struct sockaddr *) &cli_addr, sizeof cli_addr);
        if(sended == -1){
            perror("sendto error");
        }
        return sock_fd;
    }
    listen(sock_fd, 2);
    printf("listened\n");
    int acc_fd = accept(sock_fd, NULL, NULL);
    printf("accepted\n");

    return acc_fd;
}

//Получает сообщения и посылает их в pipe_get_fds
int GetMessages(int pipe_write_fd, int acc_fd) {
    char buf[1024] = {0};
    int n;
    if (udp_flag) {
        int len;
        while (1){
            int n = recvfrom(acc_fd, (char *) buf, MAXLINE,
                             MSG_WAITALL, (struct sockaddr *) &cli_addr, (socklen_t *) &len);
            write(pipe_write_fd, buf, n);
            write(STDOUT_FILENO, buf, n);
            memset(buf, 0, n);
        }
    } else {
        //TCP
        while (1) {
            n = read(acc_fd, buf, MAXLINE);
            write(pipe_write_fd, buf, n);
            char *command_str = "Command: ";
            //write(STDOUT_FILENO, command_str, 10);
            write(STDOUT_FILENO, buf, n);
            memset(buf, 0, n);
        }
    }
}

//из pipe_send_fds посылвет строки клиенту
int SendMessages(int pipe_read_send_fd, int acc_fd){
    char str[MAXLINE];
    while (1) {
        int n = read(pipe_read_send_fd, str, MAXLINE);
        if(udp_flag){
            int sended = sendto(acc_fd, str, n, MSG_CONFIRM, (const struct sockaddr *) &cli_addr, sizeof cli_addr);
            if(sended == -1){
                perror("sendto error");
            }
        }else {
            write(acc_fd, str, n);
        }
        write(STDOUT_FILENO, str, n);
        memset(str, 0, MAXLINE);
    }
}

int DoBash(int pipe_read_get_fd, int pipe_write_send_fd){
    int clone_stdin = dup(STDIN_FILENO);
    int clone_stdout = dup(STDOUT_FILENO);
    dup2(pipe_read_get_fd, STDIN_FILENO);
    dup2(pipe_write_send_fd, STDOUT_FILENO);
    char *bash_args[] = {"sh", NULL};
    int exected = execvp("sh", bash_args);
    if (exected){
        perror("Execvp error");
    }
    dup2(clone_stdin, STDIN_FILENO);
    dup2(clone_stdout, STDOUT_FILENO);
}

int main(int argc, char **argv){
    if(argc == 2 && (!strcmp(argv[1], "UDP") || !strcmp(argv[1], "udp"))){
        printf("UDP mode\n");
        udp_flag = 1;
    } else{
        printf("TCP mode\n");
    }
    int pipe_get_fds[2];
    int pipe_send_fds[2];
    if(pipe(pipe_get_fds)){
        perror("pipe_get error");
    }
    if(pipe(pipe_send_fds)){
        perror("pipe_send error");
    }

    int acc_fd = ConnectWithUser();
    int forked = fork();
    if(!forked){
        if(acc_fd < 0){
            perror("ConnectWithUser == -1");
            exit(1);
        }
        forked = fork();
        if(!forked) {
            SendMessages(pipe_send_fds[0], acc_fd);
        } else{
            GetMessages(pipe_get_fds[1], acc_fd);
        }

    } else{
        DoBash(pipe_get_fds[0], pipe_send_fds[1]);
    }
}