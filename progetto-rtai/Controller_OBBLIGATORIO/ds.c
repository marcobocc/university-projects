#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <rtai_lxrt.h>
#include <rtai_shm.h>
#include <rtai_sem.h>
#include <rtai_msg.h>
#include <sys/io.h>
#include <signal.h>

#include "controller_structs.h"
#include "parameters.h"

#define CPUMAP	0x1

static int running = 1;
static void endme(int dummy) { running = 0; }

static MBX* mbx_log;

static const char* STATUS_SHM_NAMES[3] = {
	STATUS_SHM_1,
	STATUS_SHM_2,
	STATUS_SHM_3
};

void* diag(void* args)
{
	statosistema_t* statosistema = (statosistema_t*)args;

	RT_TASK* task_diag;
	if (!(task_diag = rt_task_init_schmod(rt_get_name(0), 1, 0, 0, SCHED_FIFO, CPUMAP))) {	// TODO: la priorità qui?
		printf(ANSI_ERROR_PREFIX " Could not initialize task DIAG \n");
		pthread_exit(0);
	}

	rt_make_hard_real_time();
	rt_task_resume(task_diag);

	logmsg_t logmsg;
	
	for(int i = 0; i < NUM_SENSORS; ++i)
	{
		rt_sem_wait(statosistema->status[i]->sem_mutex);

		for(int j = 0; j < BUF_SIZE; ++j)
		{
			logmsg.statusbuffers[i][j] = statosistema->status[i]->buffer[j];
		}
		logmsg.avgs[i] = statosistema->status[i]->filter_avg;
		logmsg.heads[i] = statosistema->status[i]->head;
		logmsg.tails[i] = statosistema->status[i]->tail;
		logmsg.nelems[i] = statosistema->status[i]->nelems;

		rt_sem_signal(statosistema->status[i]->sem_mutex);

		logmsg.alertcodes[i] = statosistema->alertcodes[i];
	}
	logmsg.controlaction = statosistema->controlaction;
	rt_mbx_ovrwr_send(mbx_log, &logmsg, sizeof(logmsg_t));

	rt_task_delete(task_diag);	
	pthread_exit((void*)0);
}

int main(int argc, char** argv)
{	
	RT_TASK* task_main;
	
	if (!(task_main = rt_task_init_schmod(rt_get_name(0), 1, 0, 0, SCHED_FIFO, CPUMAP))) { 		// TODO:la priorità qui massima 1?????
		printf(ANSI_ERROR_PREFIX " Could not initialize task DS\n");							
		exit(1);
	}

	printf(ANSI_OK_PREFIX " DS ready\n");
	signal(SIGINT, endme);

	pthread_t diag_thread;
	pthread_attr_t attribs;
	pthread_attr_init(&attribs);
	pthread_attr_setdetachstate(&attribs, PTHREAD_CREATE_JOINABLE);

	RTIME period = nano2count(DS_TIME); //TODO define macro periodo time DS
	RTIME start_time = rt_get_time() + period;

	rt_task_make_periodic(task_main, start_time, period);
	rt_make_hard_real_time();

	rt_spv_RMS(CPUMAP);

	/* ============================= ALLOCAZIONE RISORSE DS =============================== */

	MBX* mailbox = rt_typed_named_mbx_init(MBX_DS_STRNAME, 12 * sizeof(alertmsg_t), FIFO_Q);
	mbx_log = rt_typed_named_mbx_init(MBX_LOG_STRNAME, 12 * sizeof(logmsg_t), FIFO_Q);

	statosistema_t* sys_state = rtai_malloc(rt_get_name(0), sizeof(statosistema_t));
	for (int i = 0; i < 3; ++i) {
		sys_state->status[i] = rtai_malloc(nam2num(STATUS_SHM_NAMES[i]), sizeof(status_t));
	}

	/* ==================================== LOOP DS ======================================= */

	RTIME num_periods = 0;
	while (running)
	{
		num_periods = num_periods + 1;
		RTIME end_of_period = start_time + (num_periods * period);

		alertmsg_t alertmsg;
		int alert = rt_mbx_receive_until(mailbox, &alertmsg, sizeof(alertmsg_t), end_of_period - (period/4));

		if (alert == 0)
		{
			sys_state->controlaction = alertmsg.controlaction;
			sys_state->alertcodes[0] = alertmsg.alertcodes[0];
			sys_state->alertcodes[1] = alertmsg.alertcodes[1];
			sys_state->alertcodes[2] = alertmsg.alertcodes[2];

			pthread_create(&diag_thread, &attribs, diag, (void*)(sys_state));
			pthread_join(diag_thread, NULL);
		}
		rt_task_wait_period(); // TODO: se aspetta sulla join come si garantisce l'hard real time ?? bisogna dimensionare bene i period no?
	}

	/* ============================= DEALLOCAZIONE RISORSE DS ============================= */

	for (int i = 0; i < NUM_SENSORS; ++i) {
		rt_shm_free(rt_get_name(sys_state->status[i]));
	}
	rt_shm_free(rt_get_name(sys_state));
	rt_mbx_delete(mailbox);
	rt_mbx_delete(mbx_log);

	rt_task_delete(task_main);
	pthread_attr_destroy(&attribs);

	exit(0);
}