//---------------------- LOGGER.C ----------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <rtai_msg.h>
#include <sys/io.h>

#include "parameters.h"
#include "controller_structs.h"

#define CPUMAP	0x1

static MBX* mbx_log = 0;

static int end = 0;
static void endme(int dummy) { end = 1; if (mbx_log) rt_mbx_delete(mbx_log); }

int main(void) {
	RT_TASK* task_main;
	if (!(task_main = rt_task_init_schmod(rt_get_name(0), 20, 0, 0, SCHED_FIFO, CPUMAP))) {
		printf(ANSI_ERROR_PREFIX " Could not initialize task LOGGER\n");							
		exit(1);
	}
	signal(SIGINT, endme);

	mbx_log = rt_typed_named_mbx_init(MBX_LOG_STRNAME, 12 * sizeof(logmsg_t), FIFO_Q);
	printf(ANSI_OK_PREFIX " Logger ready\n");

	unsigned int alert_number = 0;

	logmsg_t logmsg;
	while (!end)
	{
		int err = rt_mbx_receive(mbx_log, &logmsg, sizeof(logmsg_t));

		if (err == 0)
		{
			printf(ANSI_STYLE_BOLD_YELLOW "ALERT #%d:\n" ANSI_RESET, alert_number);
			for (int i = 0; i < 3; ++i)
			{
				printf("\tStato corrente FILTER %d: ", i);
				if (logmsg.alertcodes[i] == FILTER_OK) printf(ANSI_STYLE_BOLD_GREEN "OK\n" ANSI_RESET);
				else if (logmsg.alertcodes[i] == FILTER_DISAGREE) printf(ANSI_STYLE_BOLD_YELLOW "DISAGREE\n" ANSI_RESET);
				else if (logmsg.alertcodes[i] == FILTER_DEAD) printf(ANSI_STYLE_BOLD_RED "DEAD\n" ANSI_RESET);
				else printf("%d\n", logmsg.alertcodes[i]);

				printf("\tValori attualmente nel BUFFER %d: [", logmsg.nelems[i]);

				if (logmsg.nelems[i] != 0)
				{
					int iter = logmsg.tails[i];
					int iter_end = logmsg.heads[i];

					do {
						printf("%d", logmsg.statusbuffers[i][iter]);
						iter = (iter + 1) % BUF_SIZE;
						if(iter != iter_end) printf(", ");
					} while (iter != iter_end);
				}
				printf("]\n");

				printf("\tValori precedentemente nel BUFFER %d: [", i);
			
				if (logmsg.nelems[i] != BUF_SIZE)
				{	
					int iter = logmsg.heads[i];
					int iter_end = logmsg.tails[i];

					do {
						printf("%d", logmsg.statusbuffers[i][iter]);
						iter = (iter + 1) % BUF_SIZE;
						if(iter != iter_end) printf(", ");
					} while (iter != iter_end);
				}
				printf("]\n");

				printf("\tUltimo AVG: %d\n\n", logmsg.avgs[i]);
			}
			printf("\tSegnale di controllo: %d\n\n",logmsg.controlaction);
			alert_number = alert_number + 1;
		}
		fflush(stdout);
	}

	rt_task_delete(task_main);

	return 0;
}