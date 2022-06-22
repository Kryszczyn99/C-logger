#include "log.h"

// STANDARD LIBRARY
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
// POSIX
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

// ====================== CONSTANTS ======================
#define FILENAME_LEN_MAX 128
#define SIGNAL_STANDARD SIGUSR1
#define SIGNAL_EXIT SIGUSR2

// ====================== GLOBAL VARIABLES ======================
int tier; // 1-MIN, 2-STD, 3-MAX
int status; // 1-ON, 0-OFF
char log_filename[FILENAME_LEN_MAX]; // MAX 128

sem_t     sem_standard;
sem_t     sem_exit;
pthread_t dump_thread_handle;
void*	  buffer_for_dumps;
size_t    buffer_for_dumps_size;


// ====================== SIGNAL HANDLERS ======================
void standard_dump_handler(int signo) {
	sem_post(&sem_standard);
}

void exit_dump_handler(int signo) {
	sem_post(&sem_exit);
	sem_post(&sem_standard);
}
// ====================== ASYNC DUMP THREAD ======================

void* main_dump_thread(void* args) {
	static int dump_num = 0;
	// POST BECAUSE THREAD SHOULD BE RUNNING
	sem_post(&sem_exit);

	while (1) {
		// WAITING FOR ACTION
		sem_wait(&sem_standard);
		// CHECK IF QUEUE IS EMPTY AND THERE WAS SIGNAL TO EXIT
		int val = -1;
		sem_getvalue(&sem_standard, &val);
		if (val == 0)
		{
			if (sem_trywait(&sem_exit) == 0) {
				break;
			}
		}
		// CREATE DUMP FILE (NAME)
		srand(time(NULL));
		char filename[FILENAME_LEN_MAX] = { 0 };
		sprintf(filename, "dump_%d.bin", dump_num + rand());
		// WRITING TO DUMP FILE
		FILE *pFile = fopen(filename, "wb");
		if (!pFile)
		{
			fwrite(buffer_for_dumps, buffer_for_dumps_size, 1, pFile);
			fclose(pFile);
			dump_num++;
		}
	}
}

// ====================== API FUNCTIONS ======================
int log_start(const char *filename, void *buffer, size_t buffer_size) {
	// VALIDATION
	if (status || !filename || !buffer) {
		return 1;
	}
	// VARIABLE
	struct sigaction sa;
	// INITIALIZE GLOBAL VARIABLES
	tier = 1;
	status = 1;
	buffer_for_dumps = buffer;
	buffer_for_dumps_size = buffer_size;
	strcpy(log_filename, filename);
	// STANDARD SEMAPHORE FOR DUMPS
	if (sem_init(&sem_standard, 0, 0) != 0) {
		return 1;
	}
	// EXIT SEMAPHORE FOR DUMPS
	if (sem_init(&sem_exit, 0, 0) != 0) {
		sem_close(&sem_standard);
		return 1;
	}
	// REGISTER SIGNAL FOR DUMPS
	sa.sa_handler = standard_dump_handler;
	sa.sa_flags = 0;
	if (sigaction(SIGNAL_STANDARD, &sa, NULL) != 0) {
		sem_close(&sem_standard);
		sem_close(&sem_exit);
		return 1;
	}
	// REGISTER SIGNAL FOR EXIT
	sa.sa_handler = exit_dump_handler;
	if (sigaction(SIGNAL_EXIT, &sa, NULL) != 0)
	{
		sem_close(&sem_standard);
		sem_close(&sem_exit);
		return 1;
	}
	// STARTING THE MAIN THREAD
	if (pthread_create(&dump_thread_handle, NULL, main_dump_thread, NULL) != 0)
	{
		sem_close(&sem_standard);
		sem_close(&sem_exit);
		return 1;
	}
	// WAITING FOR THREAD TO BE STARTED
	sem_wait(&sem_exit);
}

int log_dump() {
	// CHECK IF PROGRAMME IS RUNNING
	if (!status) {
		return 1;
	}
	// ADD TO QUEUE DUMP ACTION
	union sigval value;
	sigqueue(getpid(), SIGNAL_STANDARD, value);
	return 0;
}
//DO POPRAWY
int log_string(int important_level, const char *format, ...) {
	va_list arg;
	int done = 0;
	FILE* pFile = fopen(log_filename, "a+");
	if (pFile != NULL) {
		time_t rawtime;
		struct tm * timeptr;
		time(&rawtime);
		timeptr = localtime(&rawtime);
		static const char wday_name[][4] = {
			"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
		};
		static const char mon_name[][4] = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
		};
		// STD
		if (tier >= 2) {
			fprintf(pFile, "%.3s %.3s %3d ", wday_name[timeptr->tm_wday], mon_name[timeptr->tm_mon], timeptr->tm_mday);
		}
		// MIN
		fprintf(pFile, "%.2d:%.2d:%.2d", timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);
		// MAX
		if (tier >= 3) {
			fprintf(pFile, " %d", 1900 + timeptr->tm_year);
		}
		fprintf(pFile, "  ");
		va_start(arg, format);
		vfprintf(pFile, format, arg);
		fprintf(pFile, "\n");
		va_end(arg);
		
		fclose(pFile);
	}
	return 0;
}

void log_set_tier(int t) {
	if (t >= 1 && t <= 3) {
		tier = t;
	}
}

void log_stop() {
	// CHECK IF PROGRAMME IS RUNNING
	if (status) {
		// ADD TO QUEUE SIGNAL FOR EXIT
		union sigval value;
		sigqueue(getpid(), SIGNAL_EXIT, value);
		// WAIT TILL THREAD WILL BE FINISHED
		pthread_join(dump_thread_handle, NULL);
		// CLOSE SEMS
		sem_close(&sem_standard);
		sem_close(&sem_exit);
		// RESET GLOBAL VARS
		tier = 0;
		status = 0;
		memset(log_filename, 0, FILENAME_LEN_MAX);
		buffer_for_dumps = NULL;
		buffer_for_dumps_size = 0;
	}
}

int is_log_on() {
	return status;
}