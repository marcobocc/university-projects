//------------------- RT_ENCODER.C ---------------------- 

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/io.h>
#include <asm/rtai.h>
#include <rtai_shm.h>
#include <rtai_sched.h>
#include "parameters.h"

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>

MODULE_AUTHOR("Marco Bocchetti, Riccardo Corvi");
MODULE_LICENSE("Licenza");

static int RPM = 120;
module_param(RPM, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(RPM, "Rotations Per Minute");

static RT_TASK enc_task[RES];
static unsigned int *enc_data;

static void enc(int i) 
{	 
    while (1) {
	enc_data[i]++;
	enc_data[i]%=2; 
	rt_task_wait_period();
    }
}

int init_module(void)
{
    RTIME tick_period, start;
    int mul;
    int i;
    unsigned long period;
    
    if (RPM < 5)
    {
        RPM = 5;
        printk("RPM sotto il limite minimo. Riportato a 5.\n");
    }
    else if (RPM > 120)
    {
        RPM = 120;
        printk("RPM sopra il limite massimo. Riportato a 120.\n");
    }
    else
    {
	printk("RPM settato a %d\n", RPM);
    }
    period = TICK_PERIOD_MAX * 120 / RPM;    					// PRIMA: period = TICK_PERIOD_MAX;
   
    for (i = 0; i < RES; i++){
	rt_task_init(&enc_task[i], (void *)enc, i, STACK_SIZE, i+1, 1, 0);
    }
    
    enc_data = rtai_kmalloc(SHMNAM, RES*sizeof(unsigned int));

    tick_period = nano2count(period);
    start = rt_get_time();
    
    mul = 1;
    for (i = 0; i < RES; i++){
	rt_task_make_periodic(&enc_task[i], start+mul*tick_period, mul*tick_period);
	enc_data[i]=0;
	mul*=2;
    }
 
    return 0;
}

void cleanup_module(void)
{
    int i;
    for (i = 0; i < RES; i++){
    	rt_task_delete(&enc_task[i]);
    }

    rtai_kfree(SHMNAM);

    return;
}