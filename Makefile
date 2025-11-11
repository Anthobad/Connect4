# Simple Makefile for Connect4

CC := gcc
CFLAGS := -std=c11 -Wall -Werror -g

SRCS := play.c gamelogic.c ui.c bot.c history.c input.c controller.c
OBJS := play.o gamelogic.o ui.o bot.o history.o input.o controller.o

all: connect4

run: connect4
	./connect4

run-no-anim: connect4
	./connect4 --no-anim

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

history.o: history.c history.h gamelogic.h
	$(CC) $(CFLAGS) -c history.c -o history.o

input.o: input.c input.h
	$(CC) $(CFLAGS) -c input.c -o input.o

controller.o: controller.c controller.h gamelogic.h ui.h bot.h history.h input.h
	$(CC) $(CFLAGS) -c controller.c -o controller.o


clean:
	rm -f $(OBJS) connect4

test:
	@echo "=========================================="
	@echo "Testing Connect4 with optimization levels"
	@echo "=========================================="
	@for opt in 0 1 2 3; do \
		make clean >/dev/null; \
		echo ""; \
		echo " > Compiling with -O$$opt"; \
		echo "------------------------------------------"; \
		$(CC) $(CFLAGS) -O$$opt -o connect4_O$$opt $(SRCS); \
		if [ $$? -ne 0 ]; then \
			echo "Compilation failed for -O$$opt"; \
			exit 1; \
		fi; \
		echo "Compilation succeeded for -O$$opt"; \
		rm connect4_O$$opt; \
	done
	@echo ""
valgrind:
	@echo "=========================================="
	@echo "Running Valgrind Memory Check (-O3 build)"
	@echo "=========================================="
	@$(CC) $(CFLAGS) -O3 -o connect4 $(SRCS)
	@valgrind --leak-check=full --error-exitcode=1 ./connect4 || echo "Memory leaks detected!"
	@make clean >/dev/null

help:
	@echo "Makefile targets:"
	@echo "  make                (build connect4)"
	@echo "  make run            (build and run with animation)"
	@echo "  make run-no-anim    (build and run without animation)"
	@echo "  make clean          (remove object files and binary)"
	@echo "  make test           (compile with various optimization levels)"
	@echo "  make valgrind       (run Valgrind memory check on -O3 build)"