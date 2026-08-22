/* Copyright (c) Kuba Szczodrzyński 2026-08-22. */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PATH_HTTP_WRAP "/usr/bin/http"
#define PATH_HTTP_REAL "/usr/bin/http."
#define PATH_METADATA  "/tmp/http_metadata.xml"

#define vb_min(a, b)            \
	({                          \
		__typeof__(a) _a = (a); \
		__typeof__(b) _b = (b); \
		_a < _b ? _a : _b;      \
	})
#define vb_max(a, b)            \
	({                          \
		__typeof__(a) _a = (a); \
		__typeof__(b) _b = (b); \
		_a > _b ? _a : _b;      \
	})

#define printff(...)         \
	do {                     \
		printf(__VA_ARGS__); \
		fflush(stdout);      \
	} while (0)

typedef struct patch_t {
	uintptr_t addr;
	size_t len;
	const uint8_t *old;
	const uint8_t *new;
} patch_t;

// utils.c
void hexdump(const void *buf, size_t len);
void print_argv(const char *argv[]);
int execv_fork(const char *path, const char *argv[]);
int ptrace_peek(pid_t pid, uintptr_t addr, uint8_t *buf, size_t len);
int ptrace_poke(pid_t pid, uintptr_t addr, const uint8_t *buf, size_t len);
