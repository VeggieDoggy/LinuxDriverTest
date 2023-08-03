
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <signal.h>


/*
 * ./sr04_app /dev/sr04
 *
 */
int main(int argc, char **argv)
{
	int fd;
	int ns;

	int i;
	
	/* 1. 判断参数 */
	if (argc != 2) 
	{
		printf("Usage: %s <dev>\n", argv[0]);
		return -1;
	}


	/* 2. 打开文件 */
//	fd = open(argv[1], O_RDWR | O_NONBLOCK);
	fd = open(argv[1], O_RDWR);
	if (fd == -1)
	{
		printf("can not open file %s\n", argv[1]);
		return -1;
	}


	while (1)
	{
		if (read(fd, &ns, 4) == 4) {
			printf("get ns: %d us\n", ns);
            printf("get distance: %d cm\n", ns/58000); // mm
            printf("get distance: %d inch\n", ns/148000); // inch
        } else
			printf("get distance: -1\n");
	}
	
	close(fd);
	
	return 0;
}


