/*
 * BackItUp!
 * CS460 Programming Assignment 2
 * Corey Amoruso, Michael Swiger, Sasha Demyanik
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>


#define MAX_PATH 1024
#define DEBUG 1


/* Constant strings for the backup directory and file suffix. */
const char *bakDir = ".backup";
const char *suffix = ".bak";


/* Type for the mode of operation (backing up or restoring). */
typedef enum {M_BACKUP, M_RESTORE} Mode;


/* Print given debug message if DEBUG is set. */
void printfDbg(const char *format, ...) {
	va_list args;
	va_start(args, format);
	
	if (DEBUG) {
		vprintf(format, args);
	}
	
	va_end(args);
}


/* Returns true if the path is ".", false otherwise.
int isPathSelf(const char *path) {
	if (strcmp(path, ".") == 0) {
		return 1;
	}
	
	return 0;
}


/* Build a backup path for the given file path. Returned path must be freed. */
char *buildBakPath(const char *path, int dir) {
	int bakPathLen = strlen(bakDir) + 1;
	
	/* If the path is '.', we don't need to allocate beyond bakPath */
	if (!isPathSelf(path)) {
		bakPathLen += strlen(path) + 1;
	}
	
	/* If the path is a directory, we don't need to add the suffix. */
	if (!dir) {
		bakPathLen += strlen(suffix);
	}
	
	char *bakPath = malloc(bakPathLen);
	memset(bakPath, '\0', bakPathLen);
	strncpy(bakPath, bakDir, strlen(bakDir));
	
	/* If path isn't '.', append it. */
	if (!isPathSelf(path)) {
		bakPath[strlen(bakDir)] = '/';
		strncat(bakPath, path, strlen(path));
	}
	
	/* If path isn't a directory, append backup suffix. */
	if (!dir) {
		strncat(bakPath, suffix, strlen(suffix));
	}
	
	return bakPath;
}


/* Build a restore path for the given fiel path. Returned path must be freed. */
char *buildResPath(const char *path, int dir) {
	int resPathLen = strlen(path) - strlen(bakDir);
	
	if (!dir) {
		resPathLen -= strlen(suffix);
	}
	
	char *resPath = malloc(resPathLen);
	resPath = memcpy(resPath, &path[strlen(bakDir) + 1], resPathLen);
	resPath[resPathLen - 1] = '\0';
	
	return resPath;
}


/* Build a path for the given file entry name. Returned Path must be freed. */
char *buildEntryPath(const char *path, const char *entry_name) {
	int entryPathLen = strlen(path) + strlen(entry_name) + 2;
	char *entryPath = malloc(entryPathLen);
	memset(entryPath, '\0', entryPathLen);
	strncpy(entryPath, path, strlen(path));
	strncat(entryPath, "/", strlen("/"));
	strncat(entryPath, entry_name, strlen(entry_name));
	return entryPath;
}


/* 
 * Check if the given entry is a valid entry to proceed with. This returns 0
 * (false) if the directory is . (current dir), .. (parent dir), or .backup
 */
int isValidEntry(const struct dirent *entry) {
	const char *name = entry->d_name;
	
	if (!strcmp(name, ".") || !strcmp(name, "..") || !strcmp(name, bakDir)) {
		return 0;
	}
	
	return 1;
}


/* Return the number of regular (non-directory) files in given directory. */
int fileCount(const char *path){
	DIR *dir = opendir(path);
	int count = 0;
	struct dirent *entry;
	struct stat s;

	while ((entry = readdir(dir)) != NULL) {
		if (isValidEntry(entry)) {
			char *entryPath = buildEntryPath(path, entry->d_name);
			
			if (lstat(entryPath, &s) == -1) {
				printf("File counting error in `%s'\n", path);
				perror("lstat");
			}
			
			if (S_ISREG(s.st_mode)) {
				count++;
			}
			
			free(entryPath);
		}
	}
	
	closedir(dir);

	return count;
}


/*Method copyFile*/
void copyFile(const char *srcPath, const char *destPath, Mode mode){
	/* Compare modification times of src and dest, only copy if src is newer. */ 
	struct stat srcStat, destStat;
	lstat(srcPath, &srcStat);
	int destStatErr = lstat(destPath, &destStat); /* exists when returns 0 */
	
	/* If file exists and the source is older than the dest, don't copy. */
	if (!destStatErr && (srcStat.st_mtim.tv_sec > destStat.st_mtim.tv_sec)) {
		printf("`%s' is already up to date\n", destPath);
		return;
	}
	
	/* Print the copy message with warning if overwriting the destination. */
	if (mode == M_BACKUP) {
		printf("Backing up `%s' to `%s'\n", srcPath, destPath);
	} else if (mode == M_RESTORE) {
		printf("Restoring `%s' to `%s'\n", srcPath, destPath);
	}

	if (!destStatErr) {
		printf("WARNING: Overwriting `%s'\n", destPath);
	}

	/* Actually do the file copying. */
	FILE *src = fopen(srcPath, "r");
	FILE *dest = fopen(destPath, "w");
	
	if (!src) {
		printf("Error: Failed to open source file.\n");
	}
	
	if (!dest) {
		printf("Error: Failed to open destination file.\n");
	}
	
	int byte;
	while ((byte = fgetc(src)) != EOF) {
		fputc(byte, dest);
	}

	fclose(src);
	fclose(dest);
}


/* Back up the file at the given path. */
void backup(char *path) {
	char *destPath = buildBakPath(path, 0);
	copyFile(path, destPath, M_BACKUP);
	free(destPath);
	free(path);
}


/* Restore the file at the given path. */
void restore(char *path) {
	char *destPath = buildResPath(path, 0);
	copyFile(path, destPath, M_RESTORE);
	free(destPath);
	free(path);
}


/*
 * Recursively backup/restore the given directory path. Which operation is
 * carried out depends on the value of 'mode'.
 */
void copyDir(const char *path, Mode mode) {
	DIR *dir = opendir(path);
	struct dirent *entry;
	struct stat s;
	pthread_t *threads = NULL;
	int numFiles = 0;
	int ct = 0; /* Head of threads array. */
	
	/* Make sure the directory was successfully opened. */
	if (!dir) {
		printfDbg("Error: Unable to open directory `%s'\n", path);
		return;
	}
	
	/* Make sure the destination directory exists. */
	char *destDir = NULL;
	if (mode == M_BACKUP) {
		destDir = buildBakPath(path, 1);
	} else if (mode == M_RESTORE) {
		destDir = buildResPath(path, 1);
	}
	
	if (destDir) {
		mkdir(destDir, S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH);
		free(destDir);
	}
	
	/* Count the # of regular files, allocate appropriate # of threads. */
	numFiles = fileCount(path);
	threads = malloc(sizeof(pthread_t) * numFiles);
	
	printfDbg("Opened directory `%s' with %d regular files\n", path, numFiles);
	
	/* Copy all regular files in directory, recurse through subdirectories. */
	while ((entry = readdir(dir)) != NULL) {
		if (!isValidEntry(entry)) {
			continue;
		}
		
		/* Build the path of the given file entry. */
		char *entryPath = buildEntryPath(path, entry->d_name);
		
		/* Make sure there aren't any problems with the entry path. */
		if (lstat(entryPath, &s) == -1){
			printf("File copying error for `%s'\n", path);
			perror("lstat");
		}
		
		/*
		 * Perform the correct operation based on mode and file type. Because 
		 * we cannot predict when the threads will terminate, we need to free
		 * the entry path from within the threads. For directories, however, we
		 * can free the path from here.
		 */
		if (S_ISREG(s.st_mode) && mode == M_BACKUP) {
			pthread_create(&threads[ct], NULL, (void*)backup, (void*)entryPath);
			ct++;
		} else if(S_ISREG(s.st_mode) && mode == M_RESTORE) {
			pthread_create(&threads[ct], NULL, (void*)restore, (void*)entryPath);
			ct++;
		} else if (S_ISDIR(s.st_mode)) {
			copyDir(entryPath, mode);
			free(entryPath);
		}
	}
	
	/* Wait for threads to join, then free all threads. */
	for (int i = 0; i < numFiles; i++) {
		pthread_join(threads[i], NULL);
	}
	
	free(threads);
	
	/* Close directory. */
	closedir(dir);
}


/* Main function (entry point). */
int main(int argc, char *argv[]) {
	if (strcmp(argv[argc - 1], "-r") == 0) {
		copyDir(bakDir, M_RESTORE);
	} else {
		copyDir(".", M_BACKUP);
	}
	return 0;
}
