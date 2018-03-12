#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glob.h>
#include <time.h>

#include "minilogger.h"

#define MAX_PATH_LENGTH 4096	/* TODO: allocate based on given path length */

static char files_directory[MAX_PATH_LENGTH];
static char latest_filepath[MAX_PATH_LENGTH];
static unsigned long desired_data_age; 		
static unsigned long max_file_age;

/* Returns the position after the last / in the given path */
static int get_filename_position(char *path)
{
	int position = 0;

	char c;
	int i = 0;
	while((c = path[i++]) != '\0')
		if(c == '/')
			position = i;

	return position;
}

/* Returns 1 if file was deleted, 0 if not */
static inline int delete_if_old(unsigned long age, char *filepath)
{
	if(age >= (desired_data_age + max_file_age)) {
		unlink(filepath);
		return 1;
	}
	return 0;
}

/* Get the filepath of the file to write data to */
static char *get_latest_filepath()
{
	if(files_directory[0] == '\0')
		return NULL;

	time_t now = time(NULL);

	/* make a wildcard match */
	size_t fd_len = strlen(files_directory) + 1;
	char files_directory_match[fd_len + 1];
	memcpy(files_directory_match, files_directory, fd_len);
	strcat(files_directory_match, "*");

	glob_t pglob;
	int glob_result = glob(files_directory_match, GLOB_ERR, NULL, &pglob);
	if(glob_result != 0) {
		#ifdef MINILOGGER_DEBUG
		printf("glob error (%d)\n", glob_result);
		switch(glob_result) {
			case GLOB_NOSPACE:
				fprintf(stderr, "\tOut of memory\n");
				return NULL;
			case GLOB_ABORTED:
				fprintf(stderr, "\tError reading %s, path doesn't exist or insufficient permission\n", files_directory_match);
				return NULL;
			case GLOB_NOMATCH:
				fprintf(stdout, "\tNo matches found for %s\n\tThis is fine. Creating new log file.\n", files_directory_match);
				goto create_new_file;
				break;
		}
		#endif
		if(glob_result == GLOB_NOMATCH) /* dir is empty, so skip to creating */
			goto create_new_file;

		return NULL;
	}

	if(pglob.gl_pathc > 0) {
		int which = -1;
		unsigned long max = 0;
		unsigned long fname, fage;
		int fname_pos;
		for(int i = 0; i < pglob.gl_pathc; i++) {
			char *path = pglob.gl_pathv[i];
			fname_pos = get_filename_position(path);
			fname = strtoul(path + fname_pos, NULL, 10);
			fage = now - fname;
			if(!delete_if_old(fage, path)) {
				if(fage < max_file_age && fname > max) {
					max = fname;
					which = i;
				}
			}
		}
		if(which > -1) {
			char *path = pglob.gl_pathv[which];
			memcpy(latest_filepath, path, strlen(path) + 1);
		} else {
			goto create_new_file;
		}
	} else {
		create_new_file:;

		int fd_len = strlen(files_directory);
		memcpy(latest_filepath, files_directory, fd_len);
		sprintf(latest_filepath + fd_len, "%lu", now);

		#ifdef MINILOGGER_DEBUG
		printf("Creating new log file: \"%s\"\n", latest_filepath);
		#endif
	}

	globfree(&pglob);
	return latest_filepath;
}

int ml_push_to_file(const void *data, size_t size, size_t nmemb)
{
	char *filepath = get_latest_filepath();
	if(filepath == NULL)
		return -1;

	#ifdef MINILOGGER_DEBUG
	printf("Pushing data to: \"%s\"\n", filepath);
	#endif

	FILE *fp = fopen(filepath, "a");
	if(fp == NULL) {
		#ifdef MINILOGGER_DEBUG
		printf("Failed to open file...\n");
		#endif
		return -1;
	}
	if(fwrite(data, size, nmemb, fp) != nmemb) {
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}

void ml_set_files_directory(const char *path)
{
	/* ensure path ends with '/' */
	int length = strlen(path);
	if(length > 0) {
		memcpy(files_directory, path, length + 1);
		if(path[length - 1] != '/')
			strcat(files_directory, "/");

		#ifdef MINILOGGER_DEBUG
		printf("set files_directory to: \"%s\"\n",files_directory);
		#endif

		return;
	}

	#ifdef MINILOGGER_DEBUG
	printf("set_files_directory: bad directory...\n");
	#endif

	files_directory[0] = '\0';
}

void ml_set_desired_data_age(unsigned long age)
{
	desired_data_age = age;
}

void ml_set_max_file_age(unsigned long age)
{
	max_file_age = age;
}
