#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#if defined __USE_MISC || defined __USE_BSD || defined __USE_XOPEN_EXTENDED
extern int mknod (const char *__path, __mode_t __mode, __dev_t __dev)
__THROW __nonnull ((1));
#endif


const int BUFFER_SIZE = 5000;

ssize_t read_information(const char* file_path, char* destination, int size) {
    int fd;
    if ((fd = open(file_path, O_RDONLY)) < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    ssize_t read_bytes = read(fd, destination, size - 1);
    if (read_bytes == -1) {
        perror("Error reading from file");
        exit(EXIT_FAILURE);
    }
    destination[read_bytes] = '\0'; // Null-terminate the string
    if (close(fd) < 0) {
        perror("Error closing file");
        exit(EXIT_FAILURE);
    }
    return read_bytes;
}

ssize_t write_information(char* file_path,char* buffer, size_t size){
    int fd_write;
    if((fd_write = open(file_path, O_WRONLY | O_CREAT, 0666)) < 0){
        printf("Error opening file for writing\n");
        exit(EXIT_FAILURE);
    }

    ssize_t written = write(fd_write, buffer, size);
    if(written != size){
        printf("Error writing to file\n");
        exit(EXIT_FAILURE);
    }

    if(close(fd_write) < 0){
        printf("Error closing file\n");
        exit(EXIT_FAILURE);
    }
    return written;
}

bool is_vowel(char c) {
    c = toupper(c);
    return (c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U');
}

void replace_vowels_with_hex(char input[], char output[]) {
    for (int i = 0; i < strlen(input); i++) {
        if (is_vowel(input[i])) {
            sprintf(output + strlen(output), "{0x%X}", input[i]); // Заменяем гласные на их ASCII коды в шестнадцатеричной форме
        } else {
            sprintf(output + strlen(output), "%c", input[i]); // Остальные символы оставляем без изменений
        }
    }
}

int main(int argc, char** argv){
    if(argc < 3){
        printf("Not enough arguments to complete task!\n");
        exit(EXIT_FAILURE);
    }

    char* input_file = argv[1];
    char* output_file = argv[2];

    char reader_to_handler[] = "read_to_handler.fifo";
    char handler_to_reader[] = "handler_to_read.fifo";
    mknod(reader_to_handler, S_IFIFO | 0666, 0);
    mknod(handler_to_reader, S_IFIFO | 0666, 0);

    char buffer[BUFFER_SIZE];

    int pid = fork();
    if(pid < 0){
        printf("Can\'t fork child\n");
        exit(EXIT_FAILURE);
    } else if (pid > 0){
        ssize_t read_from_file = read_information(input_file, buffer, BUFFER_SIZE);
        int fd1, fd2;

        if((fd1 = open(reader_to_handler, O_WRONLY)) < 0){
            printf("Can\'t open FIFO for writing\n");
            exit(EXIT_FAILURE);
        }

        size_t written = write(fd1, buffer, read_from_file);
        if(written != read_from_file){
            printf("Can\'t write all string to FIFO\n");
            exit(EXIT_FAILURE);
        }

        if(close(fd1) < 0) {
            printf("reader: Can\'t close writing side of FIFO\n");
            exit(EXIT_FAILURE);
        }

        if((fd2 = open(handler_to_reader, O_RDONLY)) < 0){
            printf("Can\'t open FIFO for reading\n");
            exit(EXIT_FAILURE);
        }

        char info_from_handler[BUFFER_SIZE * 4];
        ssize_t read_from_handler = read(fd2, info_from_handler, BUFFER_SIZE * 4);
        if(read_from_handler < 0){
            printf("Can\'t read string from FIFO\n");
            exit(EXIT_FAILURE);
        }

        ssize_t write_to_file = write_information(output_file, info_from_handler, read_from_handler);
        if(write_to_file < read_from_handler){
            printf("Can\'t write everything to file");
            exit(EXIT_FAILURE);
        }

        if(close(fd2) < 0){
            printf("Can\'t close FIFO");
            exit(EXIT_FAILURE);
        }
    } else {
        int fd1, fd2;
        char info_from_fifo[BUFFER_SIZE];

        if((fd1 = open(reader_to_handler, O_RDONLY)) < 0){
            printf("Can\'t open FIFO for reading\n");
            exit(EXIT_FAILURE);
        }

        ssize_t read_from_fifo = read(fd1,info_from_fifo, BUFFER_SIZE);

        if(read_from_fifo < 0){
            printf("Can\'t read string from FIFO\n");
            exit(EXIT_FAILURE);
        }

        if(close(fd1) < 0) {
            printf("handler: Can\'t close reading side of FIFO\n");
            exit(EXIT_FAILURE);
        }

        char new_string[BUFFER_SIZE * 4];
        replace_vowels_with_hex(info_from_fifo, new_string);
        if((fd2 = open(handler_to_reader, O_WRONLY)) < 0){
            printf("Can\'t open FIFO for writing\n");
            exit(EXIT_FAILURE);
        }

        size_t written = write(fd2, new_string, strlen(new_string));
        if(written != strlen(new_string)){
            printf("Can\'t write all string to FIFO\n");
            exit(EXIT_FAILURE);
        }

        if(close(fd2) < 0) {
            printf("handler: Can\'t close writing side of FIFO\n");
            exit(EXIT_FAILURE);
        }
    }
}
