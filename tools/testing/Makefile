CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS =

SRCS = minigame_tester.c mgt_platform.c mgt_state.c mgt_game_sim.c mgt_sync.c \
       mgt_bus.c mgt_read_bridge.c mgt_host.c mgt_game_bridge.c mg_registry.c \
       mg_test_cube.c

OBJS = $(SRCS:.c=.o)

ifeq ($(OS),Windows_NT)
  EXE = minigame_tester.exe
else
  EXE = minigame_tester
endif

.PHONY: all clean run embed-test

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

embed-test: mgt_embed_test.c mgt_state.c mgt_game_sim.c
	$(CC) $(CFLAGS) -o mgt_embed_test mgt_embed_test.c mgt_state.c mgt_game_sim.c
	./mgt_embed_test

%.o: %.c mgt.h mgt_state.h mgt_host.h mgt_platform.h
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(EXE)
	./$(EXE)

clean:
	rm -f $(OBJS) $(EXE) minigame_tester.exe mgt_embed_test mgt_embed_test.exe
