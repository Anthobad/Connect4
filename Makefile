# Simple Makefile for Connect4

CC := gcc
CFLAGS := -std=c11 -Wall -Werror -g

SRCS := play.c gamelogic.c ui.c bot.c
OBJS := play.o gamelogic.o ui.o bot.o

all: connect4

connect4: $(OBJS)
	$(CC) -o $@ $^

play.o: play.c gamelogic.h ui.h bot.h
	$(CC) $(CFLAGS) -c play.c -o play.o

gamelogic.o: gamelogic.c gamelogic.h
	$(CC) $(CFLAGS) -c gamelogic.c -o gamelogic.o

ui.o: ui.c ui.h gamelogic.h
	$(CC) $(CFLAGS) -c ui.c -o ui.o

bot.o: bot.c bot.h gamelogic.h
	$(CC) $(CFLAGS) -c bot.c -o bot.o

clean:
	rm -f $(OBJS) connect4

help:
	@echo "Makefile targets:"
	@echo "  make                (build connect4)"
	@echo "  make run            (build and run with animation)"
	@echo "  make run-no-anim    (build and run without animation)"
	@echo "  make clean          (remove object files and binary)"
