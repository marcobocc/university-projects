prefix := $(shell rtai-config --prefix)

ifeq ($(prefix),)
$(error Please add <rtai-install>/bin to your PATH variable)
endif

CC = $(shell rtai-config --cc)
LXRT_CFLAGS = $(shell rtai-config --lxrt-cflags)
LXRT_LDFLAGS = $(shell rtai-config --lxrt-ldflags)

.PHONY: all
all: plant reference ds logger wcet_controller

plant: plant.c
	$(CC) $(LXRT_CFLAGS) -o $@ $< $(LXRT_LDFLAGS)

wcet_controller: wcet_controller.c controller_structs.c controller_structs.h parameters.h
	$(CC) $(LXRT_CFLAGS) -o wcet_controller wcet_controller.c controller_structs.c controller_structs.h parameters.h $(LXRT_LDFLAGS)

ds: ds.c ds.c controller_structs.c controller_structs.h parameters.h
	$(CC) $(LXRT_CFLAGS) -o ds ds.c controller_structs.c controller_structs.h parameters.h $(LXRT_LDFLAGS)

reference: reference.c parameters.h
	$(CC) $(LXRT_CFLAGS) -o reference reference.c parameters.h

logger: logger.c parameters.h
	$(CC) $(LXRT_CFLAGS) -o logger logger.c parameters.h

.PHONY: clean
clean:
	rm -f *.o *~ plant reference ds logger wcet_controller