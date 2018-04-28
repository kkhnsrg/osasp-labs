/*5. Подсчитать суммарный размер файлов в заданном каталоге (аргумент 1 командной строки) и для каждого его подкаталога отдельно. Вывести на консоль и в файл (аргумент 2 командной строки) название подкаталога, количество файлов в нём, суммарный размер файлов, имя файла с наибольшим размером. */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<dirent.h>
#include<errno.h>
#include<limits.h>//realpath
#include<libgen.h>//basename

#define ERR_LOG_PATH "/tmp/err.log"

char *AppName;

int ProcessFolder( FILE *outfile, char *curPath){
	DIR *curDir;
	curDir = opendir(curPath);
	if(curDir == NULL) {
		fprintf(stderr,"%s: %s: %s\n", AppName, curPath, strerror(errno));
		errno = 0;
		return -1;
	}
	struct dirent *dent;
	struct stat buf;
	
	long int sum = 0;
	int count = 0;
	int maxSize = -1;
	char *maxFile = alloca(strlen(curPath));
	if(maxFile==NULL){
		fprintf(stderr,"%s: %s.", AppName, strerror(errno));
		return -1;
	}
	maxFile[0] = 0;
	errno = 0;
	while(dent = readdir(curDir)){
		char *file = alloca(strlen(curPath) + strlen(dent->d_name) + 1);
		if(file==NULL){
			fprintf(stderr,"%s: %s.", AppName, strerror(errno));
			return -1;
		}
		if(strcmp(".", dent->d_name) && strcmp("..", dent->d_name)){
			strcpy(file, curPath);
			strcat(file, "/");
 			strcat(file, dent->d_name);
			if (lstat(file,&buf) == -1) {
				fprintf(stderr,"%s: %s: %s\n", AppName, curPath, strerror(errno));
				return -1;
			}
			if(!S_ISDIR(buf.st_mode)){
				if(buf.st_size > maxSize){
					maxFile = alloca(strlen(basename(file)) + 1);
					if(maxFile==NULL){
						fprintf(stderr,"%s: %s.", AppName, strerror(errno));
						return -1;
					}
					maxSize = buf.st_size;
					strcpy(maxFile, basename(file));
				}
				sum+=buf.st_size;
				count++;
			}
			if(S_ISDIR(buf.st_mode)){
				ProcessFolder(outfile, file);
			}
		}
	}

	if(errno!=0){
		fprintf(stderr,"%s: %s: %s\n", AppName, curPath, strerror(errno));
		return -1;
	}

	
	if (maxFile == NULL) {
		maxFile=("\0");
	}
	if (curPath[0] == curPath[1]){
		curPath++;
	}
	printf("%s %d %ld %s\n", curPath, count, sum, maxFile);
	fprintf(outfile,"%s %d %ld %s\n", curPath, count, sum, maxFile);
	if(closedir(curDir)==-1){
		fprintf(stderr,"%s: %s: %s\n", AppName, curPath, strerror(errno));
		return -1;
	}
	return 0;
}

int main(int argc, char **argv){
	AppName = alloca(strlen(basename(argv[0])));
	strcpy(AppName, basename(argv[0]));
	if(argc<3){
		fprintf(stderr, "%s: Too few arguments.\n", AppName);
		return -1;
	}
	FILE *file;
	if((file = fopen(argv[2],"w")) == NULL){
		fprintf(stderr,"%s: %s: %s\n", AppName, argv[2], strerror(errno));
		return -1;
	}

	if(realpath(argv[1], NULL) == NULL) {
		fprintf(stderr,"%s: %s: %s\n", AppName, argv[1], strerror(errno));
		return -1;
	}

	ProcessFolder(file, realpath(argv[1], NULL));

	if(fclose(file)!=0){
		fprintf(stderr,"%s: %s: %s\n", AppName, argv[2], strerror(errno));
		return -1;
	}
	return 0;
}
