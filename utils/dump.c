#include <quicktime/quicktime.h>







int main(int argc, char *argv[])
{
	quicktime_t *file;
	
	if(argc < 2)
	{
		printf("Dump all tables in movie.\n");
		exit(1);
	}

	if(!(file = quicktime_open(argv[1], 1, 0)))
	{
		printf("Open failed\n");
		exit(1);
	}

	quicktime_dump(file);

	quicktime_close(file);
	exit (EXIT_SUCCESS);
}
