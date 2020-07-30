#ifndef H_STRUCT_DEFS
#define H_STRUCT_DEFS

#include <rtai_sem.h>
#include <rtai_shm.h>
#include <rtai_msg.h>
#include <rtai_mbx.h>

#include "parameters.h"

#define ANSI_RESET   			"\x1b[0m"
#define ANSI_COLOR_RED     		"\x1b[91m"
#define ANSI_COLOR_GREEN     		"\x1b[92m"
#define ANSI_COLOR_YELLOW  		"\x1b[93m"

#define ANSI_STYLE_BOLD			"\x1b[1m"
#define ANSI_STYLE_BOLD_RED		ANSI_STYLE_BOLD ANSI_COLOR_RED
#define ANSI_STYLE_BOLD_GREEN		ANSI_STYLE_BOLD ANSI_COLOR_GREEN
#define ANSI_STYLE_BOLD_YELLOW		ANSI_STYLE_BOLD ANSI_COLOR_YELLOW

#define ANSI_OK_PREFIX			ANSI_STYLE_BOLD ANSI_COLOR_GREEN "OK:" ANSI_RESET
#define ANSI_ERROR_PREFIX		ANSI_STYLE_BOLD ANSI_COLOR_RED "ERROR:" ANSI_RESET

typedef struct {
	int buffer[BUF_SIZE];
	int filter_avg;
	int head;
	int tail;
	int nelems;
	SEM* sem_mutex;
} status_t;

typedef struct {
	int buffer[BUF_SIZE];
	int head;
	int tail;
	SEM* sem_space_avail;
	SEM* sem_measure_avail;
} buffer_t;

typedef struct {
	int id;
	int* sensor;
	buffer_t* buffer;
	status_t* status;
} acquire_t;

typedef struct {
	int id;
	buffer_t* buffer;
	status_t* status;
	MBX* mbx_filtered;	
} filter_t;

typedef struct {
	int* reference;
	MBX* mbx_filtered;
	MBX* mbx_controlaction;
	MBX* mbx_ds;
} controller_t;

typedef struct {
	int* plant;
	MBX* mbx_controlaction;
} actuator_t;

typedef struct {
	int id;
	unsigned int avg;
} cntrlmsg_t;

typedef struct{
	unsigned int controlaction;
	int alertcodes[3];
} alertmsg_t;

typedef struct {
	unsigned int controlaction;
	int alertcodes[3];
	status_t* status[3];
} statosistema_t;

typedef struct{
	int statusbuffers[3][BUF_SIZE];
	int heads[3];
	int tails[3];
	int nelems[3];
	unsigned int avgs[3];
	unsigned int controlaction;
	int alertcodes[3];
} logmsg_t;

typedef struct{
	int id;
	int* sensor;
	buffer_t* buffer;
	status_t* status;
	MBX* mbx_filtered;	
} fuso_t;

int alloc_buffer(buffer_t** buffer);
void dealloc_buffer(buffer_t** buffer);

int alloc_acquire(acquire_t** acquire, int id);
void dealloc_acquire(acquire_t** acquire);

int alloc_fuso(fuso_t** fuso, int id);
void dealloc_fuso(fuso_t** fuso);

int alloc_filter(filter_t** filter);
void dealloc_filter(filter_t** filter);

int alloc_controller(controller_t** controller);
void dealloc_controller(controller_t** controller);

int alloc_actuator(actuator_t** actuator);
void dealloc_actuator(actuator_t** actuator);

int attach_acquire_to_sensor(acquire_t* acquire, int* sensor);
int attach_fuso_to_sensor(fuso_t* fuso, int* sensor);
int attach_filter_to_acquire(filter_t* filter, buffer_t* buffer, acquire_t* acquire);
int attach_fuso_to_buffer(fuso_t* fuso, buffer_t* buffer);
int attach_filter_to_controller(filter_t* filter, controller_t* controller);
int attach_fuso_to_controller(fuso_t* fuso, controller_t* controller);
int attach_controller_to_reference(controller_t* controller, int* reference);
int attach_controller_to_actuator(controller_t* controller, actuator_t* actuator);
int attach_actuator_to_plant(actuator_t* actuator, int* plant);

int attach_filter_to_status(filter_t* filter, status_t* status);
int attach_acquire_to_status(acquire_t* acquire, status_t* status);
int attach_fuso_to_status(fuso_t* fuso, status_t* status);

#endif