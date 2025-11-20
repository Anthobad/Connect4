#include "ui.h"
#include "controller.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char** argv) {
	int use_anim = 1;
	int anim_ms = 110;

	if (argc > 1 && strcmp(argv[1], "--no-anim") == 0)
		use_anim = 0;

	srand((unsigned)time(NULL));

	while (1) {
		int selection = ui_main_menu();

		if (selection == 0) {
			puts("Goodbye!");
			return 0;
		}

		if (selection == 1) {
			run_human_vs_human(use_anim, anim_ms);
		} else if (selection == 2) {
			ui_clear_screen();
			puts("Online multiplayer is not implemented yet.");
			puts("(Here we will later show network-related options.)");
			ui_wait_for_enter();
		} else if (selection == 3) {
			int diff = ui_bot_menu();
			if (diff == 0) {
				continue;
			} else if (diff == 1 || diff == 2 || diff == 3) {
				run_vs_bot(use_anim, anim_ms, diff);
			}
		} else if (selection == 4) {
			ui_show_about();
			ui_wait_for_enter();
		}
	}
}
