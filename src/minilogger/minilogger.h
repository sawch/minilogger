#ifndef MINILOGGER_H
#define MINILOGGER_H

int ml_push_to_file(const void *data, size_t size, size_t nmemb);
void ml_set_files_directory(const char *path);
void ml_set_desired_data_age(unsigned long age);
void ml_set_max_file_age(unsigned long age);

#endif
