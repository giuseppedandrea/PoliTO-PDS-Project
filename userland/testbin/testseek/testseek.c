
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <kern/fcntl.h>
#include <kern/seek.h>

  
int main()
{
    int fd = open("file", O_CREAT | O_RDWR);

	lseek(fd, 500, SEEK_CUR);
	fprintf(fd,"stuff\n\n");
	lseek(fd, 1000, SEEK_END);
	fprintf(fd, "moar stuff\n");

	// This lseek should fail. Sometimes it even returns an error.
	/* lseek(fd, -100, SEEK_SET);
	printf(fd, "shouldn't work\n");
	*/

	close(fd);
}