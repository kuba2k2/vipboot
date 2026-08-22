/* Copyright (c) Kuba Szczodrzyński 2026-08-22. */

#include "vipboot.h"

#define EXEC_PATH "/usr/bin/http."

int main(int argc, const char *argv[]) {
	printf("--- HTTP BEFORE: ");
	print_argv(argv);
	printff("\n");

	// build argument array for execv()
	int eargc			  = 1;
	const char *eargv[64] = {EXEC_PATH};

	// set after encountering the first -o flag
	int has_out = 0;

	for (int i = 1; i < argc; i++) {
		// check for -o option
		if (strcmp(argv[i], "-o") == 0) {
			// skip this and the option value if -o was found before
			if (has_out) {
				i += 1;
				continue;
			}
			// otherwise just mark that it was found now
			has_out = 1;
		}

		// store arguments in eargv[]
		eargv[eargc++] = argv[i];
	}

	eargv[eargc] = NULL;
	printf("--- HTTP AFTER : ");
	print_argv(eargv);
	printff("\n");

	return execv(eargv[0], (char **)eargv);
}
