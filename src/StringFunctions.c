#include "StringFunctions.h"
void GetNumWord(char *str, char *buf, int n){
    int position = 0;
    while (str[position] == ' '){
        position++;
    }
    for (int i = 0; i < n && position < MAXLINE; ++i) {
        while (str[position] != ' '){
            position++;
        }
        while (str[position] == ' '){
            position++;
        }
    }
    int i_buf = 0;
    while (str[position] != ' ' && str[position] != 0 && str[position] != '\n' && position < MAXLINE){
        buf[i_buf] = str[position];
        i_buf++;
        position++;
    }
}

int GetFileSize(int fd){
    if(fd == -1)
        return -1;

    struct stat buf;
    fstat(fd, &buf);
    off_t file_size = buf.st_size;
    return file_size;
}

int CopyFile(char *path_out, char *path_to){
    int   c;
    FILE *stream_R;
    FILE *stream_W;
    stream_R = fopen (path_out, "r");
    if (stream_R == NULL)
        return -1;
    stream_W = fopen (path_to, "w");   //create and write to file
    if (stream_W == NULL)
    {
        fclose (stream_R);
        return -2;
    }
    while ((c = fgetc(stream_R)) != EOF)
        fputc (c, stream_W);
    fclose (stream_R);
    fclose (stream_W);

    return 0;
}

int CP_CommandDetected(char *buf){
    int i = 0;
    while (CP_Command[i] != 0){
        if(buf[i] != CP_Command[i]){
            return 0;
        }
        i++;
    }
    return 1;
}
