//------------------- PLANT.C ---------------------- 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <rtai_lxrt.h>
#include <rtai_shm.h>
#include <sys/io.h>
#include <signal.h>
#include "parameters.h"
#define CPUMAP 0x1

//emulates the plant to be controlled

static RT_TASK *main_Task;
static RT_TASK *loop_Task;
static int keep_on_running = 1;

static pthread_t main_thread;
static RTIME expected;
static RTIME sampl_interv;

static void endme(int dummy) {keep_on_running = 0;}

int* sensor1;
int* sensor2;
int* sensor3;
int* actuator;

static void * main_loop(void * par) {
	
	if (!(loop_Task = rt_task_init_schmod(nam2num("PLANT"), 2, 0, 0, SCHED_FIFO, CPUMAP))) {
		printf("CANNOT INIT PERIODIC TASK\n");
		exit(1);
	}

	unsigned int iseed = (unsigned int)rt_get_time();
  	srand (iseed);

	(*sensor1) = 100; (*sensor2) = 100; (*sensor3) = 100; (*actuator) = 0;
	int sens_temp = 100; 
	expected = rt_get_time() + sampl_interv;
	rt_task_make_periodic(loop_Task, expected, sampl_interv);
	rt_make_hard_real_time();

	int count = 1;
	int noise = 0;
	int fault1 = 0; int count_fault1 = 0;
	int fault2 = 0; int count_fault2 = 0;
	int fault3 = 0; int count_fault3 = 0;
	int when_to_decrease = 1 + (int)( 10.0 * rand() / ( RAND_MAX + 1.0 ));
	while (keep_on_running)
	{
		if (count%when_to_decrease==0) {
			if (sens_temp>0) sens_temp--; // the sensed data smoothly decreases...
			when_to_decrease = 1 + (int)( 10.0 * rand() / ( RAND_MAX + 1.0 ));
			count = when_to_decrease;
		}
		// add noise between -1 and 1	
		noise = -1 + (int)(3.0 * rand() / ( RAND_MAX + 1.0 ));
		if (sens_temp>0) sens_temp += noise;
		
		// reaction to control
		if (count%2 == 0) {
			if ((*actuator) == 1) 
				sens_temp++;
			else if (((*actuator) == -1) && (sens_temp>0)) 
				sens_temp--;
		}
		count++;
		
		fault1 = 1 + (int)( 1000.0 * rand() / ( RAND_MAX + 1.0 ));
//printf("fault1=%d\n",fault1);
		if (fault1 > FAULT_THD && count_fault1==0) { 
			(*sensor1) = 1 + (int)( 200.0 * rand() / ( RAND_MAX + 1.0 ));
			count_fault1 = 8 + (int)( 8.0 * rand() / ( RAND_MAX + 1.0 ));
		}
		else if (count_fault1 > 0) count_fault1--;
		else (*sensor1) = sens_temp;

		fault2 = 1 + (int)( 1000.0 * rand() / ( RAND_MAX + 1.0 ));
//printf("fault1=%d\n",fault1);
		if (fault2 > FAULT_THD && count_fault2==0) { 
			(*sensor2) = 1 + (int)( 200.0 * rand() / ( RAND_MAX + 1.0 ));
			count_fault2 = 8 + (int)( 8.0 * rand() / ( RAND_MAX + 1.0 ));
		}
		else if (count_fault2 > 0) count_fault2--;
		else (*sensor2) = sens_temp;
		

		fault3 = 1 + (int)( 1000.0 * rand() / ( RAND_MAX + 1.0 ));
//printf("fault1=%d\n",fault1);
		if (fault3 > FAULT_THD && count_fault3==0) { 
			(*sensor3) = 1 + (int)( 200.0 * rand() / ( RAND_MAX + 1.0 ));
			count_fault3 = 8 + (int)( 8.0 * rand() / ( RAND_MAX + 1.0 ));
		}
		else if (count_fault3 > 0) count_fault3--;
		else (*sensor3) = sens_temp;
		
		
		

		rt_task_wait_period();
	}
	rt_task_delete(loop_Task);
	return 0;
}

int main(void)
{
	printf("The plant STARTED!\n");
 	signal(SIGINT, endme);

	if (!(main_Task = rt_task_init_schmod(nam2num("MNTSK"), 0, 0, 0, SCHED_FIFO, 0xF))) {
		printf("CANNOT INIT MAIN TASK\n");
		exit(1);
	}

	//attach to data shared with the controller
	sensor1 = rtai_malloc(SEN_SHM_1, sizeof(int));
	sensor2 = rtai_malloc(SEN_SHM_2, sizeof(int));
	sensor3 = rtai_malloc(SEN_SHM_3, sizeof(int));
	actuator = rtai_malloc(ACT_SHM, sizeof(int));

	sampl_interv = nano2count(TICK_TIME);
	pthread_create(&main_thread, NULL, main_loop, NULL);
	while (keep_on_running) {
		printf("Values: %d\t%d\t%d\n",(*sensor1),(*sensor2),(*sensor3));
		rt_sleep(1000000000);
	}

	rt_shm_free(SEN_SHM_1);
	rt_shm_free(SEN_SHM_2);
	rt_shm_free(SEN_SHM_3);
	rt_shm_free(ACT_SHM);
	rt_task_delete(main_Task);
 	printf("Simple RT Task (user mode) STOPPED\n");
	return 0;
}