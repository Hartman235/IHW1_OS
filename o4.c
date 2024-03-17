#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

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
        printf("Not enough arguments to complete task!");
        exit(EXIT_FAILURE);
    }

    char* input_file = argv[1];
    char* output_file = argv[2];
    char buffer[BUFFER_SIZE];

    int fd_handler_writer[2];
    if(pipe(fd_handler_writer) < 0){
        printf("Can\'t open pipe\n");
        exit(EXIT_FAILURE);
    }

    int fd_reader_handler[2];
    if(pipe(fd_reader_handler) < 0){
        printf("Can\'t open pipe\n");
        exit(EXIT_FAILURE);
    }

    int pid = fork();
    if(pid < 0){
        printf("Can\'t fork child\n");
        exit(EXIT_FAILURE);
    } else if (pid > 0){
        int reader_handler = fork();
        if(reader_handler < 0){
            printf("Can\'t fork child\n");
            exit(EXIT_FAILURE);
        } else if(reader_handler > 0){
            ssize_t read_from_file = read_information(input_file, buffer, BUFFER_SIZE);

            if(close(fd_reader_handler[0]) < 0){
                printf("parent: Can\'t close reading side of pipe\n");
                exit(EXIT_FAILURE);
            }
            size_t written = write(fd_reader_handler[1], buffer, read_from_file);
            if(written != read_from_file){
                printf("Can\'t write all string to pipe\n");
                exit(EXIT_FAILURE);
            }
            if(close(fd_reader_handler[1]) < 0) {
                printf("parent: Can\'t close writing side of pipe\n");
                exit(EXIT_FAILURE);
            }
        } else{
            char info_from_pipe[BUFFER_SIZE];
            if(close(fd_reader_handler[1]) < 0){
                printf("handler: Can\'t close writing side of pipe\n");
                exit(EXIT_FAILURE);
            }
            ssize_t read_from_pipe = read(fd_reader_handler[0],info_from_pipe, BUFFER_SIZE);
            if(read_from_pipe < 0){
                printf("Can\'t read string from pipe\n");
                exit(EXIT_FAILURE);
            }
            char new_string[BUFFER_SIZE * 4];
            close(fd_reader_handler[0]);
            replace_vowels_with_hex(info_from_pipe, new_string);

            if(close(fd_handler_writer[0]) < 0){
                printf("handler: Can\'t close reading side of pipe\n");
                exit(EXIT_FAILURE);
            }
            size_t written = write(fd_handler_writer[1], new_string, strlen(new_string));
            if(written != strlen(new_string)){
                printf("Can\'t write all string to pipe\n");
                exit(EXIT_FAILURE);
            }
            if(close(fd_handler_writer[1]) < 0) {
                printf("handler: Can\'t close writing side of pipe\n");
                exit(EXIT_FAILURE);
            }
        }
    } else {
        char info_from_pipe[BUFFER_SIZE];
        if(close(fd_handler_writer[1]) < 0){
            printf("Writer to file: Can\'t close writing side of pipe\n");
            exit(EXIT_FAILURE);
        }
        ssize_t read_from_pipe = read(fd_handler_writer[0],info_from_pipe, BUFFER_SIZE);
        if(read_from_pipe < 0){
            printf("Can\'t read string from pipe\n");
            exit(EXIT_FAILURE);
        }
        size_t written_to_file = write_information(output_file, info_from_pipe ,read_from_pipe);
        if(written_to_file != read_from_pipe){
            printf("Can't write all string to file!\n");
            exit(EXIT_FAILURE);
        }
        if(close(fd_handler_writer[0]) < 0){
            printf("Writer to file: Can\'t close reading side of pipe\n");
            exit(EXIT_FAILURE);
        }
    }
}
