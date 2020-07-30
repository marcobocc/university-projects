//------------------- RT_SPEED.C ---------------------- 

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/io.h>
#include <asm/rtai.h>
#include <rtai_shm.h>
#include <rtai_sched.h>
#include "parameters.h"

static RT_TASK speed_task;
static unsigned int *enc_data;
static unsigned int *diff_data;

static void speed(int i) 
{
	unsigned int curr_value = 0;
	unsigned int prev_value = 0;

	// Predefinizione del vettore delle potenze per la conversione da binario a decimale
	int pow[RES];
	int k = 0;
	pow[0] = 1;
	for (k = 1; k < RES; ++k)
	{
		pow[k] = pow[k-1]*2;
	}

	// Loop dello speed task 
	while (1) 
	{
		int i = 0;

		prev_value = curr_value % 1024;
		curr_value = 0;

		for (i = 0; i < RES; ++i)
		{
			curr_value += enc_data[i]*pow[i];
		}

		if (curr_value < prev_value)	// Se si va oltre il fine giro
		{
			curr_value += 1024;
		}

		// rt_printk("curr: %d - prev: %d\n", curr_value, prev_value);
		*diff_data = curr_value - prev_value;	
		rt_task_wait_period();
	}
}

int init_module(void)
{
    RTIME tick_period, start;
   
    rt_task_init(&speed_task, (void *)speed, 0, STACK_SIZE, 1, 1, 0);
    
    enc_data = rtai_kmalloc(SHMNAM, RES*sizeof(unsigned int));
    diff_data = rtai_kmalloc(SHMDIFFNAME, sizeof(unsigned int));

    tick_period = nano2count(SAMPLING_PERIOD);
    start = rt_get_time();
    
    *diff_data = 0;
    rt_task_make_periodic(&speed_task, start+tick_period, tick_period);
 
    return 0;
}

void cleanup_module(void)
{
    rt_task_delete(&speed_task);
    rtai_kfree(SHMNAM);
    rtai_kfree(SHMDIFFNAME);

    return;
}