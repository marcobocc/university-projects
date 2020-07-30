//---------------- PARAMETERS.H ----------------------- 

#define TICK_TIME 		100000000
#define CNTRL_TIME 		50000000
#define DS_TIME			CNTRL_TIME / 1000

#define TASK_PRIORITY 		1

#define STACK_SIZE 		10000

#define BUF_SIZE 		10

#define SEN_SHM_1 		121111
#define SEN_SHM_2 		121121
#define SEN_SHM_3 		121131
#define ACT_SHM 		112112
#define REFSENS 		111213

// Fault threshold (in millesimi)
#define FAULT_THD 		990

#define NUM_SENSORS 		2

// Probabilità del filter di avere un fault e mancare l'invio dell'avg al controller (in percentuale)
#define FILTER_FAULT_CHANCE	1

// Tempo durante il quale un filter non riesce ad inviare l'avg al controller in caso di fault
#define FILTER_FAULT_DURATION	10

#define MBX_DS_STRNAME		"MBX_DS"
#define MBX_LOG_STRNAME		"MBX_LG"

#define	STATUS_SHM_1		"STAT_A"
#define	STATUS_SHM_2		"STAT_B"
#define	STATUS_SHM_3		"STAT_C"

#define FILTER_OK		0
#define FILTER_DISAGREE		1
#define FILTER_DEAD		2