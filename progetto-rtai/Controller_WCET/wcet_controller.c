#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <rtai_lxrt.h>
#include <rtai_shm.h>
#include <rtai_sem.h>
#include <rtai_msg.h>
#include <rtai_mbx.h>
#include <sys/io.h>
#include <signal.h>

#include "parameters.h"
#include "controller_structs.h"

#define CPUMAP		0x1

#define NUM_MEASURES	300

static int running = 1;
static void endme(int dummy) { running = 0; signal(SIGINT, endme); }

static const unsigned long SEN_SHM_NAMES[3] = {
	SEN_SHM_1,
	SEN_SHM_2,
	SEN_SHM_3
};

static const char* STATUS_SHM_NAMES[3] = {
	STATUS_SHM_1,
	STATUS_SHM_2,
	STATUS_SHM_3
};

void* acquire_loop(void* args)
{
	acquire_t* data = (acquire_t*)args;

	RT_TASK* task_acquire;
	if (!(task_acquire = rt_task_init_schmod(rt_get_name(0), 1, 0, 0, SCHED_FIFO, CPUMAP))) {
		printf(ANSI_ERROR_PREFIX " Could not initialize task ACQUIRE %d\n", data->id);
		pthread_exit(0);
	}
	
	int tmp_head = 0;
	unsigned int temp_buffer[BUF_SIZE];

	RTIME sampl_interval = nano2count(CNTRL_TIME);
	RTIME start_time = rt_get_time() + sampl_interval;
	rt_task_make_periodic(task_acquire, start_time, sampl_interval);
	rt_make_hard_real_time();
	
	RTIME num_periods = 0;
	//while (running)
	for(int i=0; i<(NUM_MEASURES/3);i++)	
	{
		fflush(stdout);
		num_periods = num_periods + 1;

		//rt_sem_wait(data->buffer->sem_space_avail);
		RTIME start = rt_get_time();
		data->buffer->buffer[data->buffer->head] = *(data->sensor);	
		temp_buffer[data->buffer->head] = data->buffer->buffer[data->buffer->head];
		tmp_head = data->buffer->head;
		data->buffer->head = (data->buffer->head + 1) % BUF_SIZE;
		//rt_sem_signal(data->buffer->sem_measure_avail);

		//rt_sem_wait(data->status->sem_mutex);
		data->status->buffer[tmp_head] = temp_buffer[tmp_head];
		data->status->head = (tmp_head + 1) % BUF_SIZE;
		data->status->nelems = data->status->nelems + 1;
		RTIME end = rt_get_time();
		//rt_sem_signal(data->status->sem_mutex);

		RTIME ET = count2nano(end - start);
		rt_printk("ET ACQUIRE %d #%d = %llu\n", data->id, num_periods, ET);

		rt_task_wait_period();
	}
	rt_task_delete(task_acquire);
	pthread_exit(0);
}

void* filter_loop(void* args)
{
	filter_t* data = (filter_t*)args;

	RT_TASK* task_filter;
	if (!(task_filter = rt_task_init_schmod(rt_get_name(0), 2, 0, 0, SCHED_FIFO, CPUMAP))) {
		printf(ANSI_ERROR_PREFIX " Could not initialize task FILTER %d\n", data->id);
		pthread_exit(0);
	}

	int cnt = BUF_SIZE;
	unsigned int sum = 0;

	cntrlmsg_t message;
	message.id = data->id;

	RTIME sampl_interval = nano2count(CNTRL_TIME);
	RTIME start_time = rt_get_time() + sampl_interval;
	rt_task_make_periodic(task_filter, start_time, sampl_interval);
	rt_make_hard_real_time();

	srand(rt_get_time());

	int tmp_tail;

	unsigned int period_misses = 0;
	RTIME num_periods = 0;	
	//while (running)
	for(int i=0;i<(NUM_MEASURES/3);i++)
	{
		fflush(stdout);

		RTIME start = rt_get_time();

		num_periods = num_periods + 1;
		RTIME end_of_period = start_time + (num_periods * sampl_interval);

		//rt_sem_wait(data->buffer->sem_measure_avail);
		sum += data->buffer->buffer[data->buffer->tail];
		data->buffer->tail = (data->buffer->tail + 1) % BUF_SIZE;
		tmp_tail = data->buffer->tail;
		//rt_sem_signal(data->buffer->sem_space_avail);

		//rt_sem_wait(data->status->sem_mutex);
		data->status->tail = tmp_tail;
		data->status->nelems = data->status->nelems - 1;
		//rt_sem_signal(data->status->sem_mutex);

		cnt--;		
		if (cnt == 0)
		{
			message.avg = sum / BUF_SIZE;

			//rt_sem_wait(data->status->sem_mutex);
			data->status->filter_avg = sum / BUF_SIZE;
			//rt_sem_signal(data->status->sem_mutex);

			// Non simulare i fault durante il calcolo dei WCET (è una simulazione di un fault!)
			
			/*if (period_misses == 0) // Se non stai già simulando dei period_misses, simula un fault con una probabilità pari a FILTER_FAULT_CHANCE %
			{
				unsigned int send_miss_chance = rand() % 100;
				if (send_miss_chance < FILTER_FAULT_CHANCE) {
					period_misses = FILTER_FAULT_DURATION * (1000000000 / count2nano(sampl_interval));
				}
			}*/

			if (period_misses == 0) {
				//rt_mbx_send_until(data->mbx_filtered, &message, sizeof(cntrlmsg_t), end_of_period - (sampl_interval / 4));
			}

			cnt = BUF_SIZE;
			sum = 0;
		}

		/*if (period_misses != 0) {
			period_misses = period_misses - 1;
		}*/

		RTIME end = rt_get_time();
		
		RTIME ET = count2nano(end - start);
		rt_printk("ET FILTER %d #%d = %llu\n", data->id, num_periods, ET);

		rt_task_wait_period();
	}
	rt_task_delete(task_filter);
	pthread_exit(0);
}

void* controller_loop(void* args)
{
	controller_t* data = (controller_t*)args;

	RT_TASK* task_controller;
	if (!(task_controller = rt_task_init_schmod(rt_get_name(0), 3, 0, 0, SCHED_FIFO, CPUMAP))) {
		printf(ANSI_ERROR_PREFIX " Could not initialize task CONTROL\n");
		pthread_exit(0);
	}

	int following_error = 0;
	unsigned int control_action = 0;

	RTIME sampl_interval = BUF_SIZE * nano2count(CNTRL_TIME);
	RTIME start_time = rt_get_time() + sampl_interval;
	rt_task_make_periodic(task_controller, start_time, sampl_interval);
	rt_make_hard_real_time();

	RTIME num_periods = 0;	
	//while (running)
	for(int i=0;i<NUM_MEASURES;i++)
	{
		fflush(stdout);

		RTIME start = rt_get_time();

		num_periods = num_periods + 1;
		RTIME end_of_period = start_time + (num_periods * sampl_interval);

		cntrlmsg_t msg1;
		cntrlmsg_t msg2;
		cntrlmsg_t msg3;

		msg1.id = -1;
		msg2.id = -1;
		msg3.id = -1;
		
		/*int status1;
		int status2;
		int status3;
		int status1 = rt_mbx_receive_until(data->mbx_filtered, &msg1, sizeof(cntrlmsg_t), end_of_period - (sampl_interval / 4));
		int status2 = rt_mbx_receive_until(data->mbx_filtered, &msg2, sizeof(cntrlmsg_t), end_of_period - (sampl_interval / 4));
		int status3 = rt_mbx_receive_until(data->mbx_filtered, &msg3, sizeof(cntrlmsg_t), end_of_period - (sampl_interval / 4));*/
		
		int enter_if = rand() % 9;

		int alert = 0;
		alertmsg_t alertmsg;
		alertmsg.alertcodes[0] = FILTER_DEAD;
		alertmsg.alertcodes[1] = FILTER_DEAD;
		alertmsg.alertcodes[2] = FILTER_DEAD;

		/* ===================== IMPLEMENTAZIONE VOTING ===================== */
		if (enter_if == 0)
		{
			alert = 1;

			following_error = 0;
		}
		else if (enter_if == 1)
		{
			alert = 1;
			
			alertmsg.alertcodes[msg1.id] = FILTER_OK;

			following_error = 0;
		}
		else if (enter_if == 2 || enter_if == 3)
		{
			if (enter_if == 2)
			{
				following_error = *(data->reference) - msg1.avg;
			}
			else
			{
				alert = 1;

				alertmsg.alertcodes[msg1.id] = FILTER_DISAGREE;
				alertmsg.alertcodes[msg2.id] = FILTER_DISAGREE;
				
				following_error = 0;
			}
		}
		else
		{
			if (enter_if == 4)
			{
				following_error = *(data->reference) - msg1.avg;
			}
			else if (enter_if == 5)			
			{
				alert = 1;

				alertmsg.alertcodes[msg1.id] = FILTER_OK;
				alertmsg.alertcodes[msg2.id] = FILTER_OK;
				alertmsg.alertcodes[msg3.id] = FILTER_DISAGREE;

				following_error = *(data->reference) - msg1.avg;

			}
			else if (enter_if == 6)
			{
				alert = 1;

				alertmsg.alertcodes[msg3.id] = FILTER_OK;
				alertmsg.alertcodes[msg2.id] = FILTER_OK;
				alertmsg.alertcodes[msg1.id] = FILTER_DISAGREE;

				following_error = *(data->reference) - msg2.avg;

			}
			else if (enter_if == 7)
			{
				alert = 1;

				alertmsg.alertcodes[msg1.id] = FILTER_OK;
				alertmsg.alertcodes[msg3.id] = FILTER_OK;
				alertmsg.alertcodes[msg2.id] = FILTER_DISAGREE;

				following_error = *(data->reference) - msg1.avg;

			}
			else
			{
				alert = 1;

				alertmsg.alertcodes[0] = FILTER_DISAGREE;
				alertmsg.alertcodes[1] = FILTER_DISAGREE;
				alertmsg.alertcodes[2] = FILTER_DISAGREE;

				following_error = 0;
			}
		}

		/* ===================== CALCOLO DELLA LEGGE DI CONTROLLO ===================== */
		if (following_error > 0) control_action = 1;
		else if (following_error < 0) control_action = 2;
		else control_action = 3;

		if (alert)
		{
			alertmsg.controlaction = control_action;
			//rt_mbx_send_until(data->mbx_ds, &alertmsg, sizeof(alertmsg_t), end_of_period - (sampl_interval / 4));	
		}

		//rt_mbx_send_until(data->mbx_controlaction, &control_action, sizeof(unsigned int), end_of_period - (sampl_interval / 4));

		RTIME end = rt_get_time();
		RTIME ET = count2nano(end - start);
		rt_printk("ET CONTROLLER #%d = %llu\n", num_periods, ET);
		
		rt_task_wait_period();
	}
	rt_task_delete(task_controller);
	pthread_exit(0);
}

void* actuator_loop(void* args)
{
	actuator_t* data = (actuator_t*)args;

	RT_TASK* task_actuator;
	if (!(task_actuator = rt_task_init_schmod(rt_get_name(0), 4, 0, 0, SCHED_FIFO, CPUMAP))) {
		printf(ANSI_ERROR_PREFIX " Could not initialize task ACTUATOR\n");
		pthread_exit(0);
	}

	unsigned int control_action = 0;
	int cntr = 0;

	RTIME sampl_interval = BUF_SIZE * nano2count(CNTRL_TIME);
	RTIME start_time = rt_get_time() + sampl_interval;
	rt_task_make_periodic(task_actuator, start_time, sampl_interval);
	rt_make_hard_real_time();

	RTIME num_periods = 0;	
	//while (running)
	for(int i=0;i<NUM_MEASURES;i++)	
	{
		fflush(stdout);

		RTIME start = rt_get_time();
		
		num_periods = num_periods + 1;
		RTIME end_of_period = start_time + (num_periods * sampl_interval);
		
		int control_action=rand()%4;
		//rt_mbx_receive_until(data->mbx_controlaction, &control_action, sizeof(unsigned int), end_of_period - (sampl_interval / 4));

		switch (control_action) {
			case 1: cntr = 1; break;
			case 2:	cntr = -1; break;
			case 3:	cntr = 0; break;
			default: cntr = 0;
		}

		//*(data->plant) = cntr;

		RTIME end = rt_get_time();

		RTIME ET = count2nano(end - start);
		rt_printk("ET ACTUATOR #%d = %llu\n", num_periods, ET);

		rt_task_wait_period();
	}
	rt_task_delete(task_actuator);
	pthread_exit(0);
}

int main(int argc, char** argv)
{
	RT_TASK* task_main;	
	if (!(task_main = rt_task_init_schmod(rt_get_name(0), 0, 0, 0, SCHED_FIFO, CPUMAP))) {
		printf(ANSI_ERROR_PREFIX " Could not initialize task MAIN\n");
		exit(1);
	}

	signal(SIGINT, endme);

	int continue_allocation = 1;

	/* ================================ ALLOCAZIONE ACTUATOR =================================== */

	int* plant = 0;
	actuator_t* actuator = 0;

	if (continue_allocation && 
		alloc_actuator(&actuator) != 0)
	{
		printf(ANSI_ERROR_PREFIX " Allocation failed for ACTUATOR\n");
		continue_allocation = 0;
	}

	if (continue_allocation && 
		!(plant = rtai_malloc(ACT_SHM, sizeof(int))))
	{
		printf(ANSI_ERROR_PREFIX " Allocation failed for ACTUATOR\n");
		continue_allocation = 0;
	}

	if (continue_allocation && 
		attach_actuator_to_plant(actuator, plant) != 0)
	{
		printf(ANSI_ERROR_PREFIX " Attach failed for ACTUATOR to PLANT\n");
		continue_allocation = 0;
	}

	/* =============================== ALLOCAZIONE CONTROLLER ================================== */

	int* reference = 0;
	controller_t* controller = 0;

	if (continue_allocation && 
		alloc_controller(&controller) != 0)
	{
		printf(ANSI_ERROR_PREFIX " Allocation failed for CONTROLLER\n");
		continue_allocation = 0;
	}

	if (continue_allocation && 
		!(reference = rtai_malloc(REFSENS, sizeof(int))))
	{
		printf(ANSI_ERROR_PREFIX " Allocation failed for REFERENCE\n");
		continue_allocation = 0;
	}

	if (continue_allocation && 
		attach_controller_to_reference(controller, reference) != 0)
	{
		printf(ANSI_ERROR_PREFIX " Attach failed for CONTROLLER to REFERENCE\n");
		continue_allocation = 0;
	}

	if (continue_allocation && 
		attach_controller_to_actuator(controller, actuator) != 0)
	{
		printf(ANSI_ERROR_PREFIX " Attach failed for CONTROLLER to ACTUATOR\n");
		continue_allocation = 0;
	}

	/* ============================ ALLOCAZIONE ACQUIRE E FILTER =============================== */

	buffer_t* buffer[NUM_SENSORS];
	acquire_t* acquire[NUM_SENSORS];
	filter_t* filter[NUM_SENSORS];
	status_t* status[NUM_SENSORS];
	int* sensors[NUM_SENSORS];

	for (int i = 0; i < NUM_SENSORS; ++i)
	{
		buffer[i] = 0;
		if (continue_allocation && 
			alloc_buffer(&buffer[i]) != 0)
		{
			printf(ANSI_ERROR_PREFIX " Allocation failed for BUFFER %d\n", i);
			continue_allocation = 0;
		}

		acquire[i] = 0;
		if (continue_allocation && 
			alloc_acquire(&acquire[i], i) != 0)
		{
			printf(ANSI_ERROR_PREFIX " Allocation failed for ACQUIRE %d\n", i);
			continue_allocation = 0;
		}

		filter[i] = 0;
		if (continue_allocation && 
			alloc_filter(&filter[i]) != 0)
		{
			printf(ANSI_ERROR_PREFIX " Allocation failed for FILTER %d\n", i);
			continue_allocation = 0;
		}

		if (continue_allocation && 
			!(sensors[i] = rtai_malloc(SEN_SHM_NAMES[i], sizeof(int))))
		{
			printf(ANSI_ERROR_PREFIX " Allocation failed for SENSOR %d\n", i);
			continue_allocation = 0;
		}


		if (continue_allocation && 
			attach_acquire_to_sensor(acquire[i], sensors[i]) != 0)
		{
			printf(ANSI_ERROR_PREFIX " Attach failed for ACQUIRE to SENSOR\n");
			continue_allocation = 0;
		}

		if (continue_allocation && 
			attach_filter_to_acquire(filter[i], buffer[i], acquire[i]) != 0)
		{
			printf(ANSI_ERROR_PREFIX " Attach failed for FILTER to ACQUIRE\n");
			continue_allocation = 0;
		}

		if (continue_allocation && 
			attach_filter_to_controller(filter[i], controller) != 0)
		{
			printf(ANSI_ERROR_PREFIX " Attach failed for FILTER to CONTROLLER\n");
			continue_allocation = 0;
		}

		status[i] = 0;
		if (continue_allocation &&
			!(status[i] = rtai_malloc(nam2num(STATUS_SHM_NAMES[i]), sizeof(status_t))))
		{
			printf(ANSI_ERROR_PREFIX " Allocation failed for STATUS\n");
			continue_allocation = 0;
		}
		status[i]->nelems = 0;

		if (continue_allocation &&
			!(status[i]->sem_mutex = rt_typed_sem_init(rt_get_name(0), 1, CNT_SEM | PRIO_Q)))
		{
			printf(ANSI_ERROR_PREFIX " Allocation failed for SEM_MUTEX in STATUS\n");
			continue_allocation = 0;
		}
		
		if (continue_allocation && 
			attach_filter_to_status(filter[i], status[i]) != 0)
		{
			printf(ANSI_ERROR_PREFIX " Attach failed for FILTER to STATUS\n");
			continue_allocation = 0;
		}

		if (continue_allocation && 
			attach_acquire_to_status(acquire[i], status[i]) != 0)
		{
			printf(ANSI_ERROR_PREFIX " Attach failed for ACQUIRE to STATUS\n");
			continue_allocation = 0;
		}
	}

	/* ================================= CREAZIONE THREAD ====================================== */

	if (continue_allocation) 
	{
		printf(ANSI_OK_PREFIX " All resources have been successfully allocated\n");
		printf(ANSI_OK_PREFIX " Controller ready\n");

		pthread_attr_t attribs;
		pthread_attr_init(&attribs); 
		pthread_attr_setdetachstate(&attribs, PTHREAD_CREATE_JOINABLE);

		pthread_t acquire_threads[NUM_SENSORS];
		pthread_t filter_threads[NUM_SENSORS];

		pthread_t controller_thread;
		pthread_t actuator_thread;

		sigset_t set;
		sigemptyset(&set);
		sigaddset(&set, SIGINT);

		/* Blocca SIGINT per i thread figli */
		pthread_sigmask(SIG_BLOCK, &set, NULL);

		for (int i = 0; i < NUM_SENSORS; ++i)
		{
			pthread_create(&(acquire_threads[i]), &attribs, acquire_loop, (void*)(acquire[i]));
			pthread_create(&(filter_threads[i]), &attribs, filter_loop, (void*)(filter[i]));
		}

		pthread_create(&controller_thread, &attribs, controller_loop, (void*)(controller));
		pthread_create(&actuator_thread, &attribs, actuator_loop, (void*)(actuator));

		/* Sblocca SIGINT per il main */
		pthread_sigmask(SIG_UNBLOCK, &set, NULL);

		rt_spv_RMS(CPUMAP);


	/* =============================== FINE INIZIALIZZAZIONE =================================== */



	/* =================================== ATTESA THREAD ======================================= */

		pthread_join(actuator_thread, NULL);
		pthread_join(controller_thread, NULL);

		for (int i = 0; i < NUM_SENSORS; ++i)
		{
			//rt_sem_signal(buffer[i]->sem_measure_avail);
			pthread_join(filter_threads[i], NULL);

			//rt_sem_signal(buffer[i]->sem_space_avail);
			pthread_join(acquire_threads[i], NULL);
		}

		printf("\n" ANSI_OK_PREFIX " All threads have joined\n");
		pthread_attr_destroy(&attribs);
	}

	/* ============================== DEALLOCAZIONE RISORSE ==================================== */

	for (int i = 0; i < NUM_SENSORS; ++i)
	{
		dealloc_acquire(&acquire[i]);
		dealloc_filter(&filter[i]);
		dealloc_buffer(&buffer[i]);

		if (sensors[i]) {
			rt_shm_free(rt_get_name(sensors[i]));
		}
		if (status[i]) {
			if (status[i]->sem_mutex) {
				rt_sem_delete(status[i]->sem_mutex);
			}
			rt_shm_free(rt_get_name(status[i]));
		}
	}

	dealloc_controller(&controller);
	if (reference) {
		rt_shm_free(rt_get_name(reference));
	}

	dealloc_actuator(&actuator);
	if (plant) {
		rt_shm_free(rt_get_name(plant));
	}

	rt_sleep(nano2count(500000000));
	printf(ANSI_OK_PREFIX " All resources have been successfully deallocated\n");

	rt_task_delete(task_main);

	exit(0);
}