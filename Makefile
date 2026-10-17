CC = gcc
CFLAGS = -Wall -Wextra -g -O2
LDFLAGS = -lpthread

SRC = src/main.c src/proc_reader.c src/tracer.c src/thread_bench.c src/scheduler_sim.c src/netdemo.c
OBJ = $(SRC:.c=.o)
TARGET = aos-analyzer

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
