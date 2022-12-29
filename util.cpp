#include "util.h"
#include <string>
#include <memory>
#include <direct.h>
namespace RedBase {
	void unix_error(const char* msg) /* unix-style error */
	{
		fprintf(stderr, "%s: %s\n", msg, strerror(errno));
		exit(0);
	}
	void Creat(const char* pathname, int size)
	{
		std::string s = "fsutil file createnew ./";
		s = s + pathname+" " + std::to_string(size);
		//printf("´´½¨");
		system(s.data());
	}
	FILE* Open(const char* pathname,const char* mode)
	{
		FILE* fd;

		if ((fd = fopen(pathname, mode)) == NULL)
			unix_error("Open error");
		return fd;
	}

/*	int Open(const char* pathname, int flags)
	{
		int rc;

		if ((rc = open(pathname, flags)) < 0)
			unix_error("Open error");
		return rc;
	}
	*/

	int Read(FILE* fd, void* buf, size_t count)
	{
		int rc;
		//printf("¶Á2");
		if ((rc = fread(buf,count,1,fd)) < 0)
			unix_error("Read error");
		return rc;
	}

	int Write(FILE* fd, const void* buf, size_t count)
	{
		int rc;

		if ((rc = fwrite(buf,count,1,fd)) < 0)
			unix_error("Write error");
		return rc;
	}

	off_t Lseek(FILE* fildes, off_t offset, int whence)
	{
		off_t rc;

		if ((rc = fseek(fildes,offset,whence)) < 0)
			unix_error("Lseek error");
		return rc;
	}

	void Close(FILE* fd)
	{
		int rc;

		if ((rc = fclose(fd)) < 0)
			unix_error("Close error");
	}

	
	int Unlink(const char* pathname)
	{
		int rc;
		if ((rc = unlink(pathname)) < 0)
			unix_error("Unlink error");
		return 0;
	}
	

	char* Getcwd(char* buf, size_t size)
	{
		char* src = getcwd(buf, size);
		if (src == nullptr) unix_error("Getcwd error");
		return src;
	}
	
	int Chdir(const char* path)
	{
		int rc;
		if ((rc = chdir(path)) < 0) unix_error("Chdir error");
		return 0;
	}
	

}