#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STACK_MEM_SIZE 128
#define HEAP_MEM_SIZE 64

void *log_malloc(size_t bytes);
void log_free(void * ptr, size_t bytes);


int main(int argc, char const *argv[]) {
	// INITIALIZING STACK MEMORY FOR DUMP
	int memory[STACK_MEM_SIZE] = { 0 };
	// STARTING LOGGER
	if (log_start("memory_log.txt", memory, sizeof(int) * STACK_MEM_SIZE) != 0) {
		printf("Couldn't start logger!\n");
		return 1;
	}

	// SETTING LEVEL TO MAX
	log_set_tier(3);

	// DUMPING STACK MEMORY
	log_dump();

	// TESTING LOG_MALLOC AND LOG_FREE
	int *memory_ptr = log_malloc(sizeof(int) * HEAP_MEM_SIZE);
	if (memory_ptr == NULL) {
		printf("Couldn't allocate memory!\n");
		log_stop();
		return 1;
	}
	log_free(memory_ptr, sizeof(int) * HEAP_MEM_SIZE);

	// STOPPING LOGGER
	log_stop();
	return 0;
}

// MY OWN FUNCTIONS MALLOC/FREE
void *log_malloc(size_t bytes) {

	if (is_log_on() == 0) {
		return NULL;
	}

	log_string("Allocated: %hu bytes of memory!", bytes);
	void *memory = (void *)malloc(bytes);
	if (memory == NULL) {
		return NULL;
	}
	return memory;
}

void log_free(void * ptr, size_t bytes) {

	if (ptr != NULL) {
		if (is_log_on() != 0) {
			log_string("Freed: %hu bytes of memory!", bytes);
		}
		free(ptr);
	}
	
}
