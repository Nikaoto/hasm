#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "file.h"

#define READ_BUF_SIZE 256

// Completely read file, all at once
char* load_file(char* file_path, size_t* size)
{
    char buf[READ_BUF_SIZE];

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        printf("Couldn't open %s\n", file_path);
        return 0;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        printf("fstat failed\n");
        return 0;
    }

    size_t file_size = (size_t) st.st_size;

    // Set size pointer if given
    if (size) {
        *size = file_size;
    }

    size_t bytes_read = read(fd, buf, READ_BUF_SIZE);
    if (bytes_read == 0) {
        printf("File %s empty\n", file_path);
        return 0;
    }
    if (bytes_read < 0) {
        printf("Couldn't read %s\n", file_path);
        return 0;
    }

    size_t total_read = 0;
    char* file_string = malloc(file_size + 1);

    do {
        for (size_t i = 0; i < bytes_read; i++) {
            file_string[total_read + i] = buf[i];
        }
        total_read += bytes_read;
        bytes_read = read(fd, buf, READ_BUF_SIZE);
    } while(bytes_read > 0);
    file_string[file_size] = '\0';

    return file_string;
}

// Completely write file, all at once
int write_file(char *buf, char *path, size_t size)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        printf("Couldn't open file '%s' for writing\n", path);
        return 1;
    }

    fwrite(buf, 1, size, fp);

    int err = ferror(fp);
    if (err) {
        printf("I/O error %i when writing to file '%s'\n", err, path);
        return 1;
    }

    fclose(fp);
    return 0;
}
