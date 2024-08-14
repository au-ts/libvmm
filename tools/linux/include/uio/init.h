#define pragma once

#include <stdio.h>
#include <stdbool.h>

/* Uncomment this to enable debug logging */
#define DEBUG_UIO_INIT

#if defined(DEBUG_UIO_INIT)
#define LOG_UIO_INIT(...) do{ printf("UIO_DRIVER_INIT"); printf(": "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_UIO_INIT(...) do{}while(0)
#endif

#define LOG_UIO_INIT_ERR(...) do{ printf("UIO_DRIVER_INIT"); printf("|ERROR: "); printf(__VA_ARGS__); }while(0)

bool load_modules(void);

void init_main(void);

bool insmod(const char *module_path);
