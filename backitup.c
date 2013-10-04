#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#define maxSize 512

void copyFiles(){
	DIR *currentDir;
	char ch;
	struct dirent *files;
	
	if( (currentDir = opendir(".")) ){
		while( (files = readdir(currentDir)) != NULL){
			if(strcmp(files->d_name,".") != 0 
				|| strcmp(files->d_name,"..") != 0 
				|| strcmp(files->d_name,".backup") != 0){

				FILE *source, *dest;
				char buffer[maxSize];
				strcpy(buffer,"./.backup/");
 
				source = fopen(files->d_name,"r");
				strcat(buffer,files->d_name);
				strcat(buffer,".bak");
				dest = fopen(buffer,"w");
				while( (ch = fgetc(source)) != EOF){
					fputc(ch,dest);
				}
				fclose(source);
				fclose(dest);
			}
			printf("%s\n", files->d_name);
		}
	}
	closedir(currentDir);
	FILE *source, *dest;
	
}


int main(int argc, char *argv[]){
	mkdir(".backup/",S_IRWXU | S_IRWXG | S_IRWXO);
	
	copyFiles();	
	return 0;	
}
