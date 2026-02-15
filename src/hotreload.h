#pragma once

#include "ui/entry.h"

#if HOTRELOAD
#include <atomic>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <errno.h>
#include <sys/stat.h>

#include "imgui_internal.h"

#include "AppContext.h"
#define X(NAME, ARGS, RETURN_TYPE) typedef RETURN_TYPE(*NAME##_t) ARGS;
UI_ENTRY_FUNCTIONS
#undef X
#define X(NAME, args, rettype) static NAME##_t NAME = NULL;
UI_ENTRY_FUNCTIONS
#undef X

#ifndef HOTRELOAD_MODULE_PATH
#define HOTRELOAD_MODULE_PATH "build/Debug/libapp_hotreload.so"
#endif

static bool hotreload(AppContext *ctx) {
	(void)ctx;
	static struct stat statbuf;
	static time_t last_mtime = 0;
	static void *handle = nullptr;

	constexpr const char kAndroidPackagedModuleName[] = "libapp_hotreload.so";
	const bool explicit_path =
		  std::strchr(HOTRELOAD_MODULE_PATH, '/') != nullptr;
	const char *load_path = HOTRELOAD_MODULE_PATH;

	if (explicit_path) {
		if (-1 == stat(HOTRELOAD_MODULE_PATH, &statbuf)) {
#if __ANDROID__
			// On first boot in Android, load from packaged native libs.
			// Once a writable override exists in files/, it will be picked up.
			if (handle != nullptr) {
				return true;
			}
			load_path = kAndroidPackagedModuleName;
#else
			fprintf(stderr, "stat() failed for %s: %s\n", HOTRELOAD_MODULE_PATH,
			        strerror(errno));
			return false;
#endif
		} else {
			if (statbuf.st_size == 0) {
				return true;
			}
			if (handle != nullptr && last_mtime == statbuf.st_mtime) {
				return true;
			}
			last_mtime = statbuf.st_mtime;
		}
	} else if (handle != nullptr) {
		// No path means we can only load once by soname.
		return true;
	}

	printf("loading hotreload module: %s\t", load_path);
	if (handle) {
		dlclose(handle);
	}
	handle = dlopen(load_path, RTLD_NOW);
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
