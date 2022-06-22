#ifndef __LOG_H_
#define __LOG_H_

#include <stddef.h>

int log_start(const char *log_filename, void *buffer, size_t buffer_size);
int log_dump();
int log_string(const char *format, ...);
void log_set_tier(int t);
void log_stop();
int is_log_on();

#endif