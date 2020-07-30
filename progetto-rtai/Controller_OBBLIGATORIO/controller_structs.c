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

#include "parameters.h"
#include "controller_structs.h"

int alloc_buffer(buffer_t** buffer) 
{
	if ((*buffer)) {
		printf(ANSI_ERROR_PREFIX " Could not allocate BUFFER (POINTER IS NOT NULL)\n"); 
		return 1;
	}

	if (!(*buffer = rtai_malloc(rt_get_name(0), sizeof(buffer_t)))) {
		printf(ANSI_ERROR_PREFIX " Could not allocate BUFFER\n"); 
		return 2;
	}

	(*buffer)->head = 0;
	(*buffer)->tail = 0;
	(*buffer)->sem_space_avail = 0;
	(*buffer)->sem_measure_avail = 0;

	if (!((*buffer)->sem_space_avail = rt_typed_sem_init(rt_get_name(0), BUF_SIZE, CNT_SEM | PRIO_Q))) {
		printf(ANSI_ERROR_PREFIX " Could not allocate SEM_SPACE_AVAIL\n");
		rt_shm_free(rt_get_name((*buffer))); 
		(*buffer) = 0; return 3;
	}
	if (!((*buffer)->sem_measure_avail = rt_typed_sem_init(rt_get_name(0), 0, CNT_SEM | PRIO_Q))) {
		printf(ANSI_ERROR_PREFIX " Could not initialize SEM_MEASURE_AVAIL\n");
		rt_sem_delete((*buffer)->sem_space_avail);
		rt_shm_free(rt_get_name((*buffer))); 
		(*buffer) = 0; return 4;
	}
	return 0;
}

void dealloc_buffer(buffer_t** buffer)
{
	if ((*buffer))
	{
		if ((*buffer)->sem_space_avail) {
			rt_sem_delete((*buffer)->sem_space_avail);
		}
		if ((*buffer)->sem_measure_avail) {
			rt_sem_delete((*buffer)->sem_measure_avail);
		}
		rt_shm_free(rt_get_name((*buffer)));
		(*buffer) = 0;
	}
}

int alloc_acquire(acquire_t** acquire, int id)
{
	if ((*acquire)) {
		printf(ANSI_ERROR_PREFIX " Could not allocate ACQUIRE (POINTER IS NOT NULL)\n"); 
		return 1;
	}

	if (!(*acquire = rtai_malloc(rt_get_name(0), sizeof(acquire_t)))) {
		printf(ANSI_ERROR_PREFIX " Could not allocate ACQUIRE\n");
		return 2;
	}

	(*acquire)->id = id;
	(*acquire)->sensor = 0;
	(*acquire)->buffer = 0;
	(*acquire)->status = 0;

	return 0;
}

void dealloc_acquire(acquire_t** acquire)
{
	if ((*acquire))
	{
		if ((*acquire)->sensor) {
			rt_shm_free(rt_get_name((*acquire)->sensor));
		}
		rt_shm_free(rt_get_name((*acquire)));
		(*acquire) = 0;
	}
}

int alloc_filter(filter_t** filter)
{
	if ((*filter)) {
		printf(ANSI_ERROR_PREFIX " Could not allocate FILTER (POINTER IS NOT NULL)\n"); 
		return 1;
	}

	if (!(*filter = rtai_malloc(rt_get_name(0), sizeof(filter_t)))) {
		printf(ANSI_ERROR_PREFIX " Could not allocate FILTER\n");
		return 2;
	}

	(*filter)->id = -1;
	(*filter)->buffer = 0;
	(*filter)->status = 0;
	(*filter)->mbx_filtered = 0;

	return 0;
}

void dealloc_filter(filter_t** filter)
{
	if ((*filter))
	{
		rt_shm_free(rt_get_name((*filter)));
		(*filter) = 0;
	}
}

int alloc_controller(controller_t** controller)
{
	if ((*controller)) {
		printf(ANSI_ERROR_PREFIX " Could not allocate CONTROLLER (POINTER IS NOT NULL)\n"); 
		return 1;
	}

	if (!(*controller = rtai_malloc(rt_get_name(0), sizeof(controller_t)))) {
		printf(ANSI_ERROR_PREFIX " Could not allocate CONTROLLER\n");
		return 2;
	}

	(*controller)->reference = 0;
	(*controller)->mbx_filtered = 0;
	(*controller)->mbx_controlaction = 0;
	(*controller)->mbx_ds = 0;

	if (!((*controller)->mbx_filtered = rt_typed_mbx_init(rt_get_name(0), 3 * sizeof(cntrlmsg_t), FIFO_Q))) {
		printf(ANSI_ERROR_PREFIX " Could not initialize MBX_FILTERED\n");
		rt_shm_free(rt_get_name((*controller))); 
		(*controller) = 0; return 3;
	}
	if (!((*controller)->mbx_ds = rt_typed_named_mbx_init(MBX_DS_STRNAME, sizeof(alertmsg_t), FIFO_Q))) {
		printf(ANSI_ERROR_PREFIX " Could not initialize MBX_FILTERED\n");
		rt_shm_free(rt_get_name((*controller))); 
		(*controller) = 0; return 3;
	}

	return 0;
}

void dealloc_controller(controller_t** controller)
{
	if ((*controller))
	{
		if ((*controller)->mbx_filtered) {
			rt_mbx_delete((*controller)->mbx_filtered);
		}
		if ((*controller)->mbx_ds) {
			rt_mbx_delete((*controller)->mbx_ds);
		}
		rt_shm_free(rt_get_name((*controller)));
		(*controller) = 0;
	}
}

int alloc_actuator(actuator_t** actuator)
{
	if ((*actuator)) {
		printf(ANSI_ERROR_PREFIX " Could not allocate ACTUATOR (POINTER IS NOT NULL)\n"); 
		return 1;
	}

	if (!(*actuator = rtai_malloc(rt_get_name(0), sizeof(actuator_t)))) {
		printf(ANSI_ERROR_PREFIX " Could not allocate ACTUATOR\n");
		return 2;
	}

	(*actuator)->plant = 0;
	(*actuator)->mbx_controlaction = 0;

	if (!((*actuator)->mbx_controlaction = rt_typed_mbx_init(rt_get_name(0), sizeof(unsigned int), FIFO_Q))) {
		printf(ANSI_ERROR_PREFIX " Could not initialize MBX_CONTROLACTION\n");
		rt_shm_free(rt_get_name((*actuator))); 
		(*actuator) = 0; return 3;
	}

	return 0;
}

void dealloc_actuator(actuator_t** actuator)
{
	if ((*actuator))
	{
		if ((*actuator)->mbx_controlaction) {
			rt_mbx_delete((*actuator)->mbx_controlaction);
		}
		rt_shm_free(rt_get_name((*actuator)));
		(*actuator) = 0;
	}
}

int attach_acquire_to_sensor(acquire_t* acquire, int* sensor)
{
	if (!acquire) {
		printf(ANSI_ERROR_PREFIX " Could not attach ACQUIRE to SENSOR (ACQUIRE IS NULL)\n");
		return 1;
	}

	if (!sensor) {
		printf(ANSI_ERROR_PREFIX " Could not attach ACQUIRE to SENSOR (SENSOR IS NULL)\n");
		return 2;
	}

	acquire->sensor = sensor;
	return 0;
}

int attach_filter_to_acquire(filter_t* filter, buffer_t* buffer, acquire_t* acquire)
{
	if (!filter) {
		printf(ANSI_ERROR_PREFIX " Could not attach FILTER to ACQUIRE (FILTER IS NULL)\n");
		return 1;
	}

	if (!buffer) {
		printf(ANSI_ERROR_PREFIX " Could not attach FILTER to ACQUIRE (BUFFER IS NULL)\n");
		return 2;
	}

	if (!acquire) {
		printf(ANSI_ERROR_PREFIX " Could not attach FILTER to ACQUIRE (ACQUIRE IS NULL)\n");
		return 3;
	}

	filter->id = acquire->id;
	filter->buffer = buffer;
	acquire->buffer = buffer;
	return 0;
}

int attach_filter_to_controller(filter_t* filter, controller_t* controller)
{
	if (!filter) {
		printf(ANSI_ERROR_PREFIX " Could not attach FILTER to CONTROLLER (FILTER IS NULL)\n");
		return 1;
	}

	if (!controller) {
		printf(ANSI_ERROR_PREFIX " Could not attach FILTER to CONTROLLER (CONTROLLER IS NULL)\n");
		return 2;
	}

	filter->mbx_filtered = controller->mbx_filtered;
	return 0;
}

int attach_controller_to_reference(controller_t* controller, int* reference)
{
	if (!controller) {
		printf(ANSI_ERROR_PREFIX " Could not attach CONTROLLER to REFERENCE (CONTROLLER IS NULL)\n");
		return 1;
	}

	if (!reference) {
		printf(ANSI_ERROR_PREFIX " Could not attach CONTROLLER to REFERENCE (REFERENCE IS NULL)\n");
		return 2;
	}

	controller->reference = reference;
	return 0;
}

int attach_controller_to_actuator(controller_t* controller, actuator_t* actuator)
{
	if (!controller) {
		printf(ANSI_ERROR_PREFIX " Could not attach CONTROLLER to ACTUATOR (CONTROLLER IS NULL)\n");
		return 1;
	}

	if (!actuator) {
		printf(ANSI_ERROR_PREFIX " Could not attach CONTROLLER to ACTUATOR (ACTUATOR IS NULL)\n");
		return 2;
	}

	controller->mbx_controlaction = actuator->mbx_controlaction;
	return 0;
}

int attach_actuator_to_plant(actuator_t* actuator, int* plant)
{
	if (!actuator) {
		printf(ANSI_ERROR_PREFIX " Could not attach ACTUATOR to PLANT (ACTUATOR IS NULL)\n");
		return 1;
	}

	if (!plant) {
		printf(ANSI_ERROR_PREFIX " Could not attach ACTUATOR to PLANT (PLANT IS NULL)\n");
		return 2;
	}

	actuator->plant = plant;
	return 0;
}

int attach_filter_to_status(filter_t* filter, status_t* status)
{
	if (!filter) {
		printf(ANSI_ERROR_PREFIX " Could not attach FILTER to STATUS (FILTER IS NULL)\n");
		return 1;
	}

	if (!status) {
		printf(ANSI_ERROR_PREFIX " Could not attach FILTER to STATUS (STATUS IS NULL)\n");
		return 2;
	}

	filter->status = status;
	return 0;
}

int attach_acquire_to_status(acquire_t* acquire, status_t* status)
{
	if (!acquire) {
		printf(ANSI_ERROR_PREFIX " Could not attach ACQUIRE to STATUS (ACQUIRE IS NULL)\n");
		return 1;
	}

	if (!status) {
		printf(ANSI_ERROR_PREFIX " Could not attach FILTER to STATUS (STATUS IS NULL)\n");
		return 2;
	}

	acquire->status = status;
	return 0;
}