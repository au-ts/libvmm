#pragma once
#include <stdio.h>

#define DEBUG_SOUND

#if defined(DEBUG_SOUND)
#define LOG_SOUND(...) do{ fprintf(stderr, "UIO(SOUND): "); fprintf(stderr, __VA_ARGS__); }while(0)
#else
#define LOG_SOUND(...) do{}while(0)
#endif

#define LOG_SOUND_ERR(...) do{ fprintf(stderr, "UIO(SOUND)|ERROR: "); fprintf(stderr, __VA_ARGS__); }while(0)
#define LOG_SOUND_WARN(...) do{ fprintf(stderr, "UIO(SOUND)|WARN: "); fprintf(stderr, __VA_ARGS__); }while(0)
