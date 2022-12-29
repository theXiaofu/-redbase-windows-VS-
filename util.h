#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

namespace RedBase {
	void Creat(const char* pathname, int size);
	FILE* Open(const char* pathname,const char* mode);
	int Read(FILE* fd, void* buf, size_t count);
	int Write(FILE* fd, const void* buf, size_t count);
	off_t Lseek(FILE* fildes, off_t offset, int whence);
	void Close(FILE* fd);
	int Unlink(const char* pathname);
	char* Getcwd(char* buf, size_t size);
	int Chdir(const char* path);
}

#endif /* UTIL_H */