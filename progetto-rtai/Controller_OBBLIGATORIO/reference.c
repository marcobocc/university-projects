//---------------------- REFERENCE.C ----------------------------

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <rtai_shm.h>
#include "parameters.h"
 
int main (int argc, char ** argv)
{
	if (argc != 2) {	
		printf("Usage: reference <val>\n");
		return -1;
	}

	int reference = atoi(argv[1]);

	printf("Reference set to: %d\n", reference);

	int *ref_sensor;

	ref_sensor = rtai_malloc(REFSENS, sizeof(int));		/* NB: Il reference deve essere avviato DOPO il controller */
	(*ref_sensor) = reference;

	rtai_free(REFSENS, NULL);

	return 0;
}