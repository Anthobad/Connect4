#include "controller.h"

#include "gamelogic.h"
#include "ui.h"
#include "bot.h"
#include "history.h"
#include "input.h"
#include "net.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

static const char *quick_chat_msgs[] = {
	"Nice move!",
	"GG!",
	"Well played!",
	"Get better.",
	"Is this Easy Bot?",
	"Noob noob nobiyi alam rsas wa mahiyi"
};

#define QUICK_CHAT_COUNT (int)(sizeof quick_chat_msgs / sizeof quick_chat_msgs[0])

static void print_quick_chat(char player, int idx) {
	if (idx < 0 || idx >= QUICK_CHAT_COUNT)
		return;
	printf("[Player %c] %s\n", player, quick_chat_msgs[idx]);
}

static int g_allow_chat = 0;

static void switch_player(Board* G) {
	if (G->current == 'A')
		G->current = 'B';
	else
		G->current = 'A';
}

static int do_drop(Board* G, int col0, int use_anim, int anim_ms) {
	if (use_anim) {
		UiOptions opt;
		opt.use_color = 1;
		opt.delay_ms = anim_ms;
		return ui_drop_with_animation(G, col0, G->current, &opt);
	} else {
		return game_drop(G, col0, G->current);
	}
}

static int handle_turn(Board* G, int undo_span, int use_anim, int anim_ms) {
	char line[128];

	for (;;) {
		if (g_allow_chat)
			printf("Player %c, choose (1-%d), 'u' undo, 'r' redo, 't' talk, or 'q' quit: ",
				G->current, COLS);
		else
			printf("Player %c, choose (1-%d), 'u' undo, 'r' redo, or 'q' quit: ",
				G->current, COLS);
		fflush(stdout);

		if (!read_line(line, sizeof line))
			return -1;

		if (g_allow_chat) {
			const char *p = line;
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p == 't' || *p == 'T') {
				printf("Quick chat:\n");
				for (int i = 0; i < QUICK_CHAT_COUNT; i++) {
					printf(" %d) %s\n", i + 1, quick_chat_msgs[i]);
				}
				printf("Choose 1-%d (or 0 to cancel): ", QUICK_CHAT_COUNT);
				fflush(stdout);
				char buf[16];
				if (fgets(buf, sizeof buf, stdin)) {
					int choice = atoi(buf);
					if (choice >= 1 && choice <= QUICK_CHAT_COUNT) {
						int idx = choice - 1;
						print_quick_chat(G->current, idx);
					}
				}
				continue;
			}
		}

		int col0 = -1;
		int a = parse_action(line, &col0);

		if (a == -1)
			return -2;

		if (a == -2) {
			if (history_undo(G, undo_span))
				ui_print_board(G, 1);
			else
				puts("Nothing to undo.");
			return 0;
		}

		if (a == -3) {
			if (history_redo(G, undo_span))
				ui_print_board(G, 1);
			else
				puts("Nothing to redo.");
			return 0;
		}

		if (a == 0) {
			puts("Invalid command. Enter column 1-7, 'u', 'r', or 'q'.");
			continue;
		}

		int row = do_drop(G, col0, use_anim, anim_ms);
		if (row == -1) {
			puts("Column is full. Choose another.");
			return 0;
		}

		history_record_move(row, col0, G->current);

		if (!use_anim)
			ui_print_board(G, 1);

		if (checkWin(G, G->current)) {
			printf("Player %c wins!\n", G->current);
			return 1;
		}
		if (checkDraw(G)) {
			puts("It's a draw! Board is full.");
			return 1;
		}

		switch_player(G);
		return 0;
	}
}

static int play_again_prompt(void) {
	char choice[8];
	while (1) {
		printf("\nPlay again? (y/n): ");
		fflush(stdout);
		if (!fgets(choice, sizeof(choice), stdin))
			return 0;
		if (choice[0] == 'y' || choice[0] == 'Y')
			return 1;
		if (choice[0] == 'n' || choice[0] == 'N')
			return 0;
		puts("Invalid choice. Enter 'y' or 'n'.");
	}
}

void run_human_vs_human(int use_anim, int anim_ms) {
	char turn[8];
	g_allow_chat = 1;

	printf("Which player goes first, A or B: ");
	if (!fgets(turn, sizeof(turn), stdin)) {
		puts("\nInput ended. Exiting.");
		exit(0);
	}

	while (turn[0] != 'A' && turn[0] != 'B') {
		puts("Invalid letter label, try again.");
		printf("Which player goes first, A or B: ");
		if (!fgets(turn, sizeof(turn), stdin)) {
			puts("\nInput ended. Exiting.");
			exit(0);
		}
	}

	int keep_playing = 1;
	while (keep_playing) {
		Board G;
		initializeBoard(&G, turn[0]);
		history_reset();
		ui_print_board(&G, 1);

		int game_over = 0;
		while (!game_over) {
			int r = handle_turn(&G, 1, use_anim, anim_ms);
			if (r == -2) {
				puts("Goodbye!");
				exit(0);
			}
			if (r == -1) {
				puts("\nInput ended. Exiting.");
				exit(0);
			}
			if (r == 1)
				game_over = 1;
		}

		keep_playing = play_again_prompt();
		if (!keep_playing) {
			puts("Thanks for playing!");
		}
	}
}

void run_vs_bot(int use_anim, int anim_ms, int difficulty) {
	char turn[8];
	g_allow_chat = 0;

	printf("Which player goes first, You(A) or the Bot(B): ");
	if (!fgets(turn, sizeof(turn), stdin)) {
		puts("\nInput ended. Exiting.");
		exit(0);
	}

	while (turn[0] != 'A' && turn[0] != 'B') {
		puts("Invalid letter label, try again.");
		printf("Which player goes first, You(A) or the Bot(B): ");
		if (!fgets(turn, sizeof(turn), stdin)) {
			puts("\nInput ended. Exiting.");
			exit(0);
		}
	}
	zobrist_init();
	tt_init();
	int play_more = 1;
	while (play_more) {
		Board G;
		initializeBoard(&G, turn[0]);
		history_reset();
		ui_print_board(&G, 1);
		//char bot_side = (turn[0] == 'A') ? 'B' : 'A';
		int game_over = 0;
		while (!game_over) {
			if (G.current == 'A') {
				int r = handle_turn(&G, 2, use_anim, anim_ms);
				if (r == -2) {
					puts("Goodbye!");
					exit(0);
				}
				if (r == -1) {
					puts("\nInput ended. Exiting.");
					exit(0);
				}
				if (r == 1) {
					game_over = 1;
					break;
				}
			} else {
				int col0;
				if (difficulty == 1)
					col0 = bot_choose_move(&G);
				else if (difficulty ==2)
					col0 = bot_choose_move_medium(&G);
				else
					col0 = pick_best_move(&G);

				if (col0 == -1) {
					if (checkDraw(&G)) {
						puts("It's a draw! Board is full.");
						game_over = 1;
						break;
					}
				} else {
					int row = do_drop(&G, col0, use_anim, anim_ms);
					if (row != -1) {
						history_record_move(row, col0, G.current);
						if (!use_anim)
							ui_print_board(&G, 1);

						if (checkWin(&G, G.current)) {
							printf("Player %c (bot) wins!\n", G.current);
							game_over = 1;
							break;
						}
						if (checkDraw(&G)) {
							puts("It's a draw! Board is full.");
							game_over = 1;
							break;
						}
						switch_player(&G);
					}
				}
			}
		}
		shutdown_bot();
		play_more = play_again_prompt();
		if (!play_more) {
			puts("Thanks for playing!");
		}
	}
}

static int net_send_action(int sockfd, unsigned char ch) {
	if (net_send_byte(sockfd, ch) != 0) {
		perror("Failed to send action to peer");
		return -1;
	}
	return 0;
}

static int net_local_turn(Board *G, int use_anim, int anim_ms, int sockfd) {
	char line[128];

	for (;;) {
		printf("Player %c (you), choose (1-%d), 'u' undo, 'r' redo, 't' talk, or 'q' quit: ",
			G->current, COLS);
		fflush(stdout);

		if (!read_line(line, sizeof line)) {
			puts("\nInput ended. Resigning.");
			if (net_send_action(sockfd, 'q') != 0)
				return -1;
			return 1;
		}

		const char *p = line;
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p == 't' || *p == 'T') {
			printf("Quick chat:\n");
			for (int i = 0; i < QUICK_CHAT_COUNT; i++) {
				printf(" %d) %s\n", i + 1, quick_chat_msgs[i]);
			}
			printf("Choose 1-%d (or 0 to cancel): ", QUICK_CHAT_COUNT);
			fflush(stdout);
			char buf[16];
			if (fgets(buf, sizeof buf, stdin)) {
				int choice = atoi(buf);
				if (choice >= 1 && choice <= QUICK_CHAT_COUNT) {
					int idx = choice - 1;
					print_quick_chat(G->current, idx);
					unsigned char code = (unsigned char)('c' + idx);
					if (net_send_action(sockfd, code) != 0)
						return -1;
				}
			}
			continue;
		}

		int col0 = -1;
		int a = parse_action(line, &col0);

		if (a == -1) {
			puts("You resigned. Game over.");
			if (net_send_action(sockfd, 'q') != 0)
				return -1;
			return 1;
		}

		if (a == -2) {
			if (history_undo(G, 1)) {
				ui_print_board(G, 1);
				if (net_send_action(sockfd, 'u') != 0)
					return -1;
				return 0;
			} else {
				puts("Nothing to undo.");
				continue;
			}
		}

		if (a == -3) {
			if (history_redo(G, 1)) {
				ui_print_board(G, 1);
				if (net_send_action(sockfd, 'r') != 0)
					return -1;
				return 0;
			} else {
				puts("Nothing to redo.");
				continue;
			}
		}

		if (a == 0) {
			puts("Invalid command. Enter column 1-7, 'u', 'r', 't', or 'q'.");
			continue;
		}

		if (a == 1) {
			int row = do_drop(G, col0, use_anim, anim_ms);
			if (row == -1) {
				printf("Column %d is full. Choose another column.\n", col0 + 1);
				continue;
			}

			history_record_move(row, col0, G->current);
			if (!use_anim)
				ui_print_board(G, 1);

			if (net_send_action(sockfd, (unsigned char)('1' + col0)) != 0)
				return -1;

			int win = checkWin(G, G->current);
			if (win) {
				printf("Player %c (you) wins!\n", G->current);
				return 1;
			}

			if (checkDraw(G)) {
				puts("It's a draw! Board is full.");
				return 1;
			}

			switch_player(G);
			return 0;
		}
	}
}

static int net_remote_turn(Board *G, int use_anim, int anim_ms, int sockfd) {
	unsigned char ch = 0;
	puts("Waiting for opponent's move...");
	fflush(stdout);

	if (net_recv_byte(sockfd, &ch) != 0) {
		puts("Connection closed by peer.");
		return -1;
	}

	if (ch == 'q' || ch == 'Q') {
		puts("Opponent resigned. Game over.");
		return 1;
	}

	if (ch == 'u' || ch == 'U') {
		if (history_undo(G, 1)) {
			ui_print_board(G, 1);
		} else {
			puts("Opponent requested undo, but nothing to undo.");
		}
		return 0;
	}

	if (ch == 'r' || ch == 'R') {
		if (history_redo(G, 1)) {
			ui_print_board(G, 1);
		} else {
			puts("Opponent requested redo, but nothing to redo.");
		}
		return 0;
	}

	if (ch >= 'c' && ch < 'c' + QUICK_CHAT_COUNT) {
		int idx = (int)(ch - 'c');
		print_quick_chat(G->current, idx);
		return 0;
	}

	if (ch >= '1' && ch <= '7') {
		int col0 = (int)(ch - '1');
		int row = do_drop(G, col0, use_anim, anim_ms);
		if (row == -1) {
			printf("Protocol error: opponent tried to drop in full column %d.\n", col0 + 1);
			return -1;
		}

		history_record_move(row, col0, G->current);
		if (!use_anim)
			ui_print_board(G, 1);

		int win = checkWin(G, G->current);
		if (win) {
			printf("Player %c (opponent) wins!\n", G->current);
			return 1;
		}

		if (checkDraw(G)) {
			puts("It's a draw! Board is full.");
			return 1;
		}

		switch_player(G);
		return 0;
	}

	printf("Received unknown action '%c' from opponent.\n", ch);
	return -1;
}

static void run_network_game_loop(int sockfd, int is_server, int use_anim, int anim_ms, char start_player) {
	Board G;
	initializeBoard(&G, start_player);
	history_reset();

	char my_player = is_server ? 'A' : 'B';
	printf("You are player %c.\n", my_player);
	ui_print_board(&G, 1);

	int finished = 0;
	while (!finished) {
		if (G.current == my_player) {
			int res = net_local_turn(&G, use_anim, anim_ms, sockfd);
			if (res == -1) {
				puts("Network error. Ending game.");
				break;
			}
			if (res == 1)
				finished = 1;
		} else {
			int res = net_remote_turn(&G, use_anim, anim_ms, sockfd);
			if (res == -1) {
				puts("Network error. Ending game.");
				break;
			}
			if (res == 1)
				finished = 1;
		}
	}
}

// Check the ip address of the host
char* getIp() {
     char *ip = malloc(INET_ADDRSTRLEN);
     if (!ip) return NULL;

     int sock = socket(AF_INET, SOCK_DGRAM, 0);
     if (sock < 0) return NULL;

     struct sockaddr_in serv;
     memset(&serv, 0, sizeof(serv));
     serv.sin_family = AF_INET;
     serv.sin_addr.s_addr = inet_addr("8.8.8.8");
     serv.sin_port = htons(80);

     connect(sock, (const struct sockaddr*)&serv, sizeof(serv));

     struct sockaddr_in name;
     socklen_t namelen = sizeof(name);
     getsockname(sock, (struct sockaddr*)&name, &namelen);

     inet_ntop(AF_INET, &name.sin_addr, ip, INET_ADDRSTRLEN);

     close(sock);
     return ip;
}

void run_human_online(int use_anim, int anim_ms) {
	char line[128];
	int is_server = 0;
	int port = 4444;
	int sockfd = -1;
	char start_player = 'A';

	g_allow_chat = 0;

	for (;;) {
		printf("Online mode. Do you want to host (h) or join (j)? ");
		fflush(stdout);
		if (!fgets(line, sizeof line, stdin)) {
			puts("\nInput ended. Exiting online mode.");
			return;
		}
		if (line[0] == 'h' || line[0] == 'H') {
			is_server = 1;
			break;
		}
		if (line[0] == 'j' || line[0] == 'J') {
			is_server = 0;
			break;
		}
		puts("Invalid choice. Please enter 'h' to host or 'j' to join.");
	}

	if (is_server) {
		printf("Enter port to listen on (default 4444): ");
		fflush(stdout);
		if (fgets(line, sizeof line, stdin) && line[0] != '\n') {
			int p = atoi(line);
			if (p > 0 && p < 65536)
				port = p;
		}

		printf("Hosting game on ip: %s and port:  %d. Waiting for a friend to connect...\n",getIp(), port);
		sockfd = net_open_server(port);
		if (sockfd < 0) {
			puts("Failed to open server socket.");
			return;
		}
		puts("Friend connected!");

		for (;;) {
			printf("Who plays first, player A or player B? ");
			fflush(stdout);
			if (!fgets(line, sizeof line, stdin)) {
				puts("\nInput ended. Exiting online mode.");
				close(sockfd);
				return;
			}
			if (line[0] == 'A' || line[0] == 'a') {
				start_player = 'A';
				break;
			}
			if (line[0] == 'B' || line[0] == 'b') {
				start_player = 'B';
				break;
			}
			puts("Invalid choice. Please enter 'A' or 'B'.");
		}

		if (net_send_action(sockfd, (unsigned char)start_player) != 0) {
			puts("Failed to send starting player to client.");
			close(sockfd);
			return;
		}
		printf("Player %c will start.\n", start_player);
	} else {
		char ip[64];

		printf("Enter server IP address: ");
		fflush(stdout);
		if (!fgets(ip, sizeof ip, stdin)) {
			puts("\nInput ended. Exiting online mode.");
			return;
		}
		for (char *p = ip; *p; ++p) {
			if (*p == '\n' || *p == '\r') {
				*p = '\0';
				break;
			}
		}
		if (ip[0] == '\0') {
			puts("No IP address entered. Exiting online mode.");
			return;
		}

		printf("Enter server port (default 4444): ");
		fflush(stdout);
		if (fgets(line, sizeof line, stdin) && line[0] != '\n') {
			int p = atoi(line);
			if (p > 0 && p < 65536)
				port = p;
		}

		printf("Connecting to %s:%d...\n", ip, port);
		sockfd = net_open_client(ip, port);
		if (sockfd < 0) {
			puts("Failed to connect to server.");
			return;
		}
		puts("Connected to server!");

		unsigned char sp = 0;
		if (net_recv_byte(sockfd, &sp) != 0) {
			puts("Failed to receive starting player from server.");
			close(sockfd);
			return;
		}
		if (sp == 'A' || sp == 'B')
			start_player = (char)sp;
		else
			start_player = 'A';
		printf("Player %c will start.\n", start_player);
	}

	run_network_game_loop(sockfd, is_server, use_anim, anim_ms, start_player);
	close(sockfd);
}
