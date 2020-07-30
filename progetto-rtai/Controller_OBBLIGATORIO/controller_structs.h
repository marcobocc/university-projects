#ifndef H_STRUCT_DEFS
#define H_STRUCT_DEFS

#include <rtai_sem.h>
#include <rtai_shm.h>
#include <rtai_msg.h>
#include <rtai_mbx.h>

#include "parameters.h"

/* Definizione dei codici ANSI per i colori/grassetto in console */
#define ANSI_RESET   				"\x1b[0m"
#define ANSI_COLOR_RED     			"\x1b[91m"
#define ANSI_COLOR_GREEN     		"\x1b[92m"
#define ANSI_COLOR_YELLOW  			"\x1b[93m"

#define ANSI_STYLE_BOLD				"\x1b[1m"
#define ANSI_STYLE_BOLD_RED			ANSI_STYLE_BOLD ANSI_COLOR_RED
#define ANSI_STYLE_BOLD_GREEN		ANSI_STYLE_BOLD ANSI_COLOR_GREEN
#define ANSI_STYLE_BOLD_YELLOW		ANSI_STYLE_BOLD ANSI_COLOR_YELLOW

/* Definizione dei prefissi ANSI per evidenziare un messaggio OK (in verde) o ERRORE (in rosso) */
#define ANSI_OK_PREFIX				ANSI_STYLE_BOLD ANSI_COLOR_GREEN "OK:" ANSI_RESET
#define ANSI_ERROR_PREFIX			ANSI_STYLE_BOLD ANSI_COLOR_RED "ERROR:" ANSI_RESET

typedef struct {
	int buffer[BUF_SIZE];				/* Copia di un buffer condiviso tra ACQUIRE e FILTER */
	int filter_avg;						/* L'ultima media calcolata dal FILTER */
	int head;
	int tail;
	int nelems;							/* Contiene il numero di elementi attualmente presenti nel buffer, utile per discriminare se il buffer è vuoto nel caso la testa sia uguale alla coda */
	SEM* sem_mutex;						/* Semaforo per la gestione della mutua esclusione sullo status */
} status_t;

typedef struct {
	int buffer[BUF_SIZE];
	int head;
	int tail;
	SEM* sem_space_avail;				/* Semafori per la gestione del produttore-consumatore sul buffer */
	SEM* sem_measure_avail;
} buffer_t;

typedef struct {
	int id;								/* Identificativo numerico del sensore su cui l'acquire e il filter stanno operando */
	int* sensor;						/* Puntatore all'area di memoria condivisa per l'acquisizione delle misure del sensore */
	buffer_t* buffer;					/* Puntatore all'area di memoria buffer condivisa tra ACQUIRE e FILTER */
	status_t* status;					/* Puntatore all'area di memoria condivisa contenente la copia del buffer */
} acquire_t;

typedef struct {
	int id;								/* Identificativo numerico del sensore su cui l'acquire e il filter stanno operando */
	buffer_t* buffer;					/* Puntatore all'area di memoria buffer condivisa tra ACQUIRE e FILTER */
	status_t* status;					/* Puntatore all'area di memoria condivisa contenente la copia del buffer */
	MBX* mbx_filtered;					/* Mailbox condivisa tra FILTER e CONTROLLER per l'invio delle medie */
} filter_t;

typedef struct {
	int* reference;						/* Puntatore all'area di memoria condivisa per il segnale di reference */
	MBX* mbx_filtered;					/* Mailbox condivisa tra FILTER e CONTROLLER per la ricezione delle medie */
	MBX* mbx_controlaction;				/* Mailbox condivisa tra ACTUATOR e CONTROLLER per l'invio dell'azione di controllo */
	MBX* mbx_ds;						/* Mailbox (named) condivisa tra DS e CONTROLLER per l'invio dei segnali di alert */
} controller_t;

typedef struct {
	int* plant;							/* Puntatore all'area di memoria condivisa con il plant per attuare l'azione di controllo ricevuta */
	MBX* mbx_controlaction;				/* Mailbox condivisa tra ACTUATOR e CONTROLLER per la ricezione dell'azione di controllo */
} actuator_t;

typedef struct {
	int id;								/* Identificativo numerico del sensore su cui l'acquire e il filter stanno operando */
	unsigned int avg;					/* Media calcolata da un singolo filter */
} cntrlmsg_t;

typedef struct {
	unsigned int controlaction;			/* L'azione di controllo decisa dal controllore sulla base delle medie ricevute */
	int alertcodes[3];					/* Codice di stato per ciascun buffer, come definiti in "parameters.h" [0 = FILTER_OK; 1 = FILTER_DISAGREE; 2 = FILTER_DEAD] */
} alertmsg_t;

typedef struct {
	unsigned int controlaction;			/* L'azione di controllo decisa dal controllore sulla base delle medie ricevute */
	int alertcodes[3];					/* Codice di stato per ciascun buffer, come definiti in "parameters.h" [0 = FILTER_OK; 1 = FILTER_DISAGREE; 2 = FILTER_DEAD] */
	status_t* status[3];				/* Puntatore all'area di memoria condivisa contenente la copia di ciascun buffer */
} statosistema_t;

typedef struct {
	int statusbuffers[3][BUF_SIZE];		/* Contiene una copia di ciascun buffer */
	int heads[3];						/* Contiene una copia del puntatore alla testa di ciascun buffer */
	int tails[3];						/* Contiene una copia del puntatore alla coda di ciascun buffer */
	int nelems[3];						/* Contiene il numero di elementi attualmente presenti in ciascun buffer, utile per discriminare se il buffer è vuoto nel caso la testa sia uguale alla coda */
	unsigned int avgs[3];				/* L'ultima media calcolata dal filter per ciascun buffer */
	unsigned int controlaction;			/* L'azione di controllo decisa dal controllore sulla base delle medie ricevute */
	int alertcodes[3];					/* Codice di stato per ciascun buffer, come definiti in "parameters.h" [0 = FILTER_OK; 1 = FILTER_DISAGREE; 2 = FILTER_DEAD] */
} logmsg_t;

/* Funzioni di utilità per l'allocazione e la deallocazione in RTAI delle strutture */
int alloc_buffer(buffer_t** buffer);
void dealloc_buffer(buffer_t** buffer);

int alloc_acquire(acquire_t** acquire, int id);
void dealloc_acquire(acquire_t** acquire);

int alloc_filter(filter_t** filter);
void dealloc_filter(filter_t** filter);

int alloc_controller(controller_t** controller);
void dealloc_controller(controller_t** controller);

int alloc_actuator(actuator_t** actuator);
void dealloc_actuator(actuator_t** actuator);

/* Funzioni di utilità per collegare le strutture tra di loro */
int attach_acquire_to_sensor(acquire_t* acquire, int* sensor);
int attach_filter_to_acquire(filter_t* filter, buffer_t* buffer, acquire_t* acquire);
int attach_filter_to_controller(filter_t* filter, controller_t* controller);
int attach_controller_to_reference(controller_t* controller, int* reference);
int attach_controller_to_actuator(controller_t* controller, actuator_t* actuator);
int attach_actuator_to_plant(actuator_t* actuator, int* plant);
int attach_filter_to_status(filter_t* filter, status_t* status);
int attach_acquire_to_status(acquire_t* acquire, status_t* status);

#endif