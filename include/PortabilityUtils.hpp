#pragma once

// #########################
//  Sleep functions
//  ########################

#ifdef _WIN32
#include <windows.h>

// Sleep for seconds
#define SLEEP(seconds) Sleep((seconds) * 1000)

// Sleep for microseconds
#define USLEEP(usec) Sleep((usec) / 1000)  // Windows Sleep() has millisecond precision
#else
#include <unistd.h>

// Sleep for seconds
#define SLEEP(seconds) sleep(seconds)

// Sleep for microseconds
#define USLEEP(usec) usleep(usec)
#endif

