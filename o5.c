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
        printf("Not enough arguments to complete task!");
        exit(EXIT_FAILURE);
    }

    // файлы для чтения и записи
    char* input_file = argv[1];
    char* output_file = argv[2];
    char reader_to_converter_pipe[] = "mypipe.fifo";
    char converter_to_writer_pipe[] = "mysecondpipe.fifo";
    char buffer[BUFFER_SIZE];

    mknod(reader_to_converter_pipe, S_IFIFO | 0666, 0);
    mknod(converter_to_writer_pipe, S_IFIFO | 0666, 0);

    int pid = fork();
    if(pid < 0){
        printf("Can\'t fork child\n");
        exit(EXIT_FAILURE);
    } else if (pid > 0){ // предположим, что родительский процесс будет читать информацию из файла и передавать её
        int reader_handler = fork();
        if(reader_handler < 0){
            printf("Can\'t fork child\n");
            exit(EXIT_FAILURE);
        } else if(reader_handler > 0){ // родительский процесс, читаем информацию и пишем в канал
            int fd_reader;
            ssize_t read_from_file = read_information(input_file, buffer, BUFFER_SIZE);

            if((fd_reader = open(reader_to_converter_pipe, O_WRONLY)) < 0){
                printf("reader: Can\'t open FIFO for writing\n");
                exit(EXIT_FAILURE);
            }
            size_t written = write(fd_reader, buffer, read_from_file);
            if(written != read_from_file){
                printf("reader: Can\'t write all string to FIFO\n");
                exit(EXIT_FAILURE);
            }
            if(close(fd_reader) < 0) {
                printf("reader: Can\'t close FIFO\n");
                exit(EXIT_FAILURE);
            }
        } else{
            int fd_converter;
            char info_from_pipe[BUFFER_SIZE];
            if((fd_converter = open(reader_to_converter_pipe, O_RDONLY)) < 0){
                printf("converter: Can\'t open FIFO\n");
                exit(EXIT_FAILURE);
            }
            ssize_t read_from_pipe = read(fd_converter,info_from_pipe, BUFFER_SIZE);
            if(read_from_pipe < 0){
                printf("Can\'t read string from pipe\n");
                exit(EXIT_FAILURE);
            }
            if(close(fd_converter) < 0) {
                printf("handler: Can\'t close FIFO\n");
                exit(EXIT_FAILURE);
            }
            char new_string[BUFFER_SIZE * 4];
            replace_vowels_with_hex(info_from_pipe, new_string);
            if((fd_converter = open(converter_to_writer_pipe, O_WRONLY)) < 0){
                printf("converter: Can\'t open FIFO\n");
                exit(EXIT_FAILURE);
            }
            size_t written = write(fd_converter, new_string, strlen(new_string));
            if(written != strlen(new_string)){
                printf("converter: Can\'t write all string to FIFO\n");
                exit(EXIT_FAILURE);
            }
            if(close(fd_converter) < 0) {
                printf("handler: Can\'t close FIFO\n");
                exit(EXIT_FAILURE);
            }
        }
    } else {
        int fd_writer;
        char info_from_pipe[BUFFER_SIZE];
        if((fd_writer = open(converter_to_writer_pipe, O_RDONLY)) < 0){
            printf("writer: Can\'t open FIFO for reading\n");
            exit(EXIT_FAILURE);
        }
        ssize_t read_from_pipe = read(fd_writer,info_from_pipe, BUFFER_SIZE);
        if(read_from_pipe < 0){
            printf("Can\'t read string from pipe\n");
            exit(EXIT_FAILURE);
        }
        size_t written_to_file = write_information(output_file, info_from_pipe ,read_from_pipe);
        if(written_to_file != read_from_pipe){
            printf("Can't write all string to file!\n");
            exit(EXIT_FAILURE);
        }
        if(close(fd_writer) < 0){
            printf("Writer to file: Can\'t close reading side of pipe\n");
            exit(EXIT_FAILURE);
        }
    }
}
