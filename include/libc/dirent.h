#include <headers/file.h>

#define DIRP_BUF_SIZE 1024
struct dirp {
    int   fd;    // file desciptor of directory
    off_t size;  // size of directory
    off_t pos;   // position in directory
    off_t off;   // position in buffer
	int   bsz;   // buffer content size

    char buf[DIRP_BUF_SIZE];
};

struct dirp *opendir(const char *fname);

struct dirent *readdir(struct dirp *dir);

int closedir(struct dirp *dir);
