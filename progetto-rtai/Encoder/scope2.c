//---------------------- SCOPE.C ----------------------------

#include <stdio.h>

#include <unistd.h>

#include <sys/types.h>

#include <sys/mman.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <signal.h>

#include "/usr/realtime/include/rtai_shm.h"

#include "parameters.h"

 

static int end;

static void endme(int dummy) { end=1; }

unsigned int *enc_data; 

int main (void)

{

    struct data_str *data;

    signal(SIGINT, endme);

    enc_data = rtai_malloc (SHMNAM,1);

    while (!end) {

        printf("Count: [%d, %d, %d, %d, %d, %d, %d, %d, %d, %d]\n",
	enc_data[0],	
	enc_data[1],
	enc_data[2],
	enc_data[3],
	enc_data[4],
	enc_data[5],
	enc_data[6],
	enc_data[7],
	enc_data[8],
	enc_data[9]
	);
	//sleep(1);

    }

    rtai_free (SHMNAM, &data);

    return 0;

}
