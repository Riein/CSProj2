#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#define maxSize 512
#define DEBUG 1

/*Method copyFile*/
void copyFile(struct dirent  *files){
	char ch;
	FILE *source, *dest;
	char buffer[maxSize];
	int bytes = 0;

	printf("[Thread %u] Backing up %s\n",(unsigned int)pthread_self(),files->d_name);

	strcpy(buffer,"./.backup/");
 
	source = fopen(files->d_name,"r");
	strcat(buffer,files->d_name);
	strcat(buffer,".bak");
	dest = fopen(buffer,"w");
	
	struct stat s, d;
	lstat(files->d_name,&s);
	lstat(buffer,&d);
	if(dest == NULL || s.st_mtim.tv_sec < d.st_mtim.tv_sec){
		while( (ch = fgetc(source)) != EOF){
			fputc(ch,dest);
			bytes++;
		}
	}

	fclose(source);
	fclose(dest);
	
	printf("Copied %d bytes from %s to %s.bak\n", bytes,files->d_name,files->d_name);
		
}
//------------------------------ ----------------------------------------------------
/*Method fileCount: returns the number of regular files in the current directory*/
int fileCount(DIR *currentDir){
	int count = 0;
	struct dirent *files;
	struct stat s;
	char* path;

	if(DEBUG){
		printf("Counting # of files\n");
	}

	//loop through directory count the number of regular files
	while( (files = readdir(currentDir)) != NULL){
		if(strcmp(files->d_name,".") != 0 
			|| strcmp(files->d_name,"..") != 0 
			|| strcmp(files->d_name,".backup") != 0){

			if(stat(files->d_name,&s) == -1){
				printf("Stat Error\n");
			}			
			if(S_ISREG(s.st_mode)){
				count++;
			}
		}
	}
	return count;
}

//-----------------------------------------------------------------------------------
/*Method main*/
int main(int argc, char *argv[]){
	struct dirent *files;    				//holds directory entry structure
	DIR *currentDir;					//holds current directory
	int numFiles ;						//number of files in current directory
	struct stat s;						
	int i = 0;						//used to count number of threads created
	

	mkdir(".backup/",S_IRWXU | S_IRWXG | S_IRWXO);		//makes direcotry 

	//debug test statmenet
	if(DEBUG){
		printf("Opened directory\n");
	}

	//opens the current directory and calls fileCount to count the number of regular files in the directory
	if((currentDir = opendir(".")) ){
		numFiles = fileCount(currentDir);
		if(DEBUG){
			printf("number of regular files in current directory: %d\n",numFiles);
		}
		closedir(currentDir);				//closes open directory
	}

	pthread_t thread[numFiles]; 				//creates an array of threads

	if((currentDir = opendir("."))){			//opens directory for copying

		//loops through the directory creating a thread for each regular file
		while( (files = readdir(currentDir)) != NULL){
			if(strcmp(files->d_name,".") != 0 
				|| strcmp(files->d_name,"..") != 0 
				|| strcmp(files->d_name,".backup") != 0){
		
				if(stat(files->d_name,&s) == -1){
					printf("Stat Error\n");
				}

				//if the file is a regualr file			
				if(S_ISREG(s.st_mode)){
					pthread_create(&thread[i],NULL,(void *)copyFile,(void *)files);
					i++;
				}
				//else if(S_ISDIR(s.st_mode)){
				// Handle the directory
				//}
			}
		}
	}

	//join the threads
	for(int c = 0; c < i; c++){
		pthread_join(thread[c],NULL);
	}

	closedir(currentDir);		//close open directory
	return 0;	
}
