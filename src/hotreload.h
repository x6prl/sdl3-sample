#pragma once

#include "ui/entry.h"

#if HOTRELOAD
#include <cstdio>
#include <dlfcn.h>
#include <sys/stat.h>

#define X(NAME, ARGS, RETURN_TYPE) typedef RETURN_TYPE(*NAME##_t) ARGS;
UI_ENTRY_FUNCTIONS
#undef X
#define X(NAME, args, rettype) static NAME##_t NAME = NULL;
UI_ENTRY_FUNCTIONS
#undef X

#ifndef HOTRELOAD_MODULE_PATH
#define HOTRELOAD_MODULE_PATH "build/Debug/libapp_hotreload.so"
#endif

static bool hotreload(const char *path) {
	static struct stat statbuf;
	static time_t last_mtime = 0;
	static void *handle = nullptr;

	if (0 == stat(path, &statbuf)) {
		if (statbuf.st_size == 0) {
			return true;
		}
		if (handle != nullptr && last_mtime == statbuf.st_mtime) {
			return true;
		}
		last_mtime = statbuf.st_mtime;
	} else if (handle != nullptr) {
		// Missing path after at least one successful load is not fatal.
		return true;
	}

	printf("loading hotreload module: %s\t", path);
	if (handle) {
		dlclose(handle);
	}
	handle = dlopen(path, RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "dlopen() failed: %s\n", dlerror());
		return false;
	}

#define X(NAME, ARGS, RETURN_TYPE)                                             \
	NAME = (NAME##_t)(dlsym(handle, #NAME));                                   \
	if (!NAME)                                                                 \
		fprintf(stderr, "dlsym() failed: %s\n", dlerror());
	UI_ENTRY_FUNCTIONS
#undef X

	printf("DONE\n");
	return true;
}

#endif // HOTRELOAD
