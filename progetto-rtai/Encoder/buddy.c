//---------------------- BUDDY.C ----------------------------

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "/usr/realtime/include/rtai_shm.h"
#include "parameters.h"

unsigned int *diff_data;

static int end;
static void endme(int dummy) 
{ 
	end = 1; 
}

int main(void)
{
	signal(SIGINT, endme);
	struct data_str *data;

	diff_data = rtai_malloc(SHMDIFFNAME, 1);

	while (!end)
	{
		int rpm = 120 * (*diff_data) / DIFFMAX;
		printf("RPM: %d\n", rpm);
	}

	rtai_free(SHMDIFFNAME, &data);
	return 0;
}
