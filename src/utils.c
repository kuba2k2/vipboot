/* Copyright (c) Kuba Szczodrzyński 2026-08-22. */

#include "vipboot.h"

void hexdump(const void *buf, size_t len) {
	size_t pos = 0;
	while (pos < len) {
		// print hex offset
		printf("%06x ", (unsigned int)pos);
		// calculate current line width
		size_t lineWidth = (len - pos) < 16 ? (len - pos) : 16;
		// print hexadecimal representation
		for (size_t i = 0; i < lineWidth; i++) {
			if (i % 8 == 0) {
				putchar(' ');
			}
			printf("%02x ", ((const uint8_t *)buf)[pos + i]);
		}
		// print ascii representation
		putchar(' ');
		putchar('|');
		for (size_t i = 0; i < lineWidth; i++) {
			uint8_t c = ((const uint8_t *)buf)[pos + i];
			putchar((c >= 0x20 && c <= 0x7f) ? c : '.');
		}
		puts("|");
		pos += lineWidth;
	}
	fflush(stdout);
}

void print_argv(const char *argv[]) {
	for (const char **arg = argv; *arg != NULL; arg++)
		printf("%s ", **arg ? *arg : "\"\"");
}

int execv_fork(const char *path, const char *argv[]) {
	printf("\n    Executing: %s ", path);
	print_argv(argv);
	printff("\n");

	int pid = fork();
	if (pid == -1) {
		return pid;
	}

	if (pid == 0) {
		execv(path, (char **)argv);
		return -1;
	}

	int status;
	pid = wait(&status);

	printff("    PID %d exited with status %d\n", pid, status);
	return status;
}

int ptrace_peek(pid_t pid, uintptr_t addr, uint8_t *buf, size_t len) {
	for (size_t pos = 0; len != 0;) {
		errno	  = 0;
		long word = ptrace(PTRACE_PEEKDATA, pid, (void *)(addr + pos), NULL);
		if (word == -1 && errno != 0)
			return -1;

		size_t n = vb_min(len, sizeof(long));
		memcpy(buf + pos, &word, n);

		pos += n, len -= n;
	}
	return 0;
}

int ptrace_poke(pid_t pid, uintptr_t addr, const uint8_t *buf, size_t len) {
	for (size_t pos = 0; len != 0;) {
		uintptr_t word_addr = (addr + pos) & ~(sizeof(long) - 1);
		size_t offset		= (addr + pos) - word_addr;

		errno	  = 0;
		long word = ptrace(PTRACE_PEEKDATA, pid, word_addr, NULL);
		if (word == -1 && errno != 0)
			return -1;

		size_t n = vb_min(len, sizeof(long) - offset);
		memcpy((uint8_t *)&word + offset, buf + pos, n);

		if (ptrace(PTRACE_POKEDATA, pid, word_addr, word) == -1)
			return -1;

		pos += n, len -= n;
	}
	return 0;
}
