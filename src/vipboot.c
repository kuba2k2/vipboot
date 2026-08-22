/* Copyright (c) Kuba Szczodrzyński 2026-08-22. */

#include "vipboot.h"

static const patch_t patches[] = {
	{
		/* mov #0x4,r8 -> mov #0x0,r8 */
		.addr = 0x40ca06,
		.len  = 4,
		.old  = "\x04\xe8\x83\x60",
		.new  = "\x00\xe8\x83\x60",
	 },
	{
		/* mov #0x2,r5 -> mov #0x4,r5 */
		.addr = 0x417cfa,
		.len  = 4,
		.old  = "\x02\xe5\x17\xd1",
		.new  = "\x04\xe5\x17\xd1",
	 },
};

int main(int argc, const char *argv[]) {
	int err = 1;

	printf("--- vipboot called with argv: ");
	print_argv(argv);
	printff("\n");

	// find -a, -x, -s, -p options
	const char *http_name	= NULL;
	const char *xml_name	= NULL;
	const char *server_addr = NULL;
	const char *server_port = NULL;
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-a") == 0)
			http_name = argv[++i];
		if (strcmp(argv[i], "-x") == 0)
			xml_name = argv[++i];
		if (strcmp(argv[i], "-s") == 0)
			server_addr = argv[++i];
		if (strcmp(argv[i], "-p") == 0)
			server_port = argv[++i];
	}

	// check if both names are provided
	printff("--- HTTP name set to: %s\n", http_name);
	printff("--- XML name set to: %s\n", xml_name);
	printff("--- Server set to: %s:%s\n", server_addr, server_port);
	if (http_name == NULL) {
		printff("!!! Missing HTTP name\n");
		goto out;
	}
	if (xml_name == NULL) {
		printff("!!! Missing XML name\n");
		goto out;
	}
	if (server_addr == NULL) {
		printff("!!! Missing server address\n");
		goto out;
	}
	if (server_port == NULL) {
		printff("!!! Missing server port\n");
		goto out;
	}

	// STEP 1
	// patch /init in memory
	pid_t pid = 1;
	printf("--- Attaching to PID %d: ", pid);
	if ((err = ptrace(PTRACE_ATTACH, pid, NULL, NULL))) {
		perror("FAILED (ptrace)");
		goto out;
	}
	printff("OK\n");

	// keep this buffer larger than the max patch length
	uint8_t buf[8];
	int applied = 0;
	// go through every available patch
	for (const patch_t *patch = patches; patch < patches + sizeof(patches) / sizeof(*patches); patch++) {
		printf("--- Applying patch at 0x%x: ", patch->addr);
		// read the original bytes
		if ((err = ptrace_peek(pid, patch->addr, buf, patch->len))) {
			perror("FAILED (ptrace_peek)");
			break;
		}
		// check if already applied
		if (memcmp(buf, patch->new, patch->len) == 0) {
			printff("not needed\n");
			applied++;
			continue;
		}
		// check if not applicable
		if (memcmp(buf, patch->old, patch->len) != 0) {
			printff("not applicable\n");
			continue;
		}
		// otherwise apply the patch
		if ((err = ptrace_poke(pid, patch->addr, patch->new, patch->len))) {
			perror("FAILED (ptrace_poke)");
			break;
		}
		// read the data again
		if ((err = ptrace_peek(pid, patch->addr, buf, patch->len))) {
			perror("FAILED (ptrace_peek 2)");
			break;
		}
		// verify the write
		if (memcmp(buf, patch->new, patch->len) != 0) {
			printff("FAILED\n");
			break;
		}
		printff("OK\n");
		applied++;
	}
	ptrace(PTRACE_DETACH, pid, NULL, NULL);
	printff("--- Applied %d patch(es)\n", applied);

	// STEP 2
	printf("--- Renaming %s to %s: ", PATH_HTTP_WRAP, PATH_HTTP_REAL);
	if (access(PATH_HTTP_REAL, F_OK) == 0) {
		printff("not needed\n");
	} else if ((err = rename(PATH_HTTP_WRAP, PATH_HTTP_REAL))) {
		perror("FAILED");
		goto out;
	} else {
		printff("OK\n");
	}

	// STEP 3
	printf("--- Downloading %s to %s: ", http_name, PATH_HTTP_WRAP);
	do {
		if (access(PATH_HTTP_WRAP, F_OK) == 0) {
			printff("not needed\n");
			break;
		}
		const char *eargv[] = {
			PATH_HTTP_REAL,					//
			"-h",			server_addr,	// HttpServer
			"-q",			server_port,	// HttpPort
			"-p",			http_name,		// [server path]
			"-o",			PATH_HTTP_WRAP, // [output path]
			"-b",			xml_name,		// BootcastId
			"-f",			"",				// [firmware version]
			"-s",			"",				// Serial
			"-a",			"",				// MAC
			"-1",			"0.0.0.0",		// DNS1
			NULL,
		};
		if ((err = execv_fork(PATH_HTTP_REAL, eargv))) {
			perror("FAILED");
			goto out;
		}
		printff("OK\n");
	} while (0);
	if ((err = chmod(PATH_HTTP_WRAP, 0777))) {
		perror("chmod");
		goto out;
	}

	// STEP 4
	printf("--- Downloading %s to %s: ", xml_name, PATH_METADATA);
	do {
		if (access(PATH_METADATA, F_OK) == 0) {
			printff("not needed\n");
			break;
		}
		const char *eargv[] = {
			PATH_HTTP_REAL,				   //
			"-h",			server_addr,   // HttpServer
			"-q",			server_port,   // HttpPort
			"-p",			xml_name,	   // [server path]
			"-o",			PATH_METADATA, // [output path]
			"-b",			xml_name,	   // BootcastId
			"-f",			"",			   // [firmware version]
			"-s",			"",			   // Serial
			"-a",			"",			   // MAC
			"-1",			"0.0.0.0",	   // DNS1
			NULL,
		};
		if ((err = execv_fork(PATH_HTTP_REAL, eargv))) {
			perror("FAILED");
			goto out;
		}
		printff("OK\n");
	} while (0);

out:
	printff("--- vipboot finishing with status code %d\n", err);
	// return 1 anyway to fail BootCast method
	return 1;
}
