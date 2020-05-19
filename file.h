#ifndef FILE_H
#define FILE_H

char *load_file(char* file_path, size_t* size);
int write_file(char* buf, char *path, size_t size);

#endif // FILE_H
