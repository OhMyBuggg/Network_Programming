#pragma once
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#define writeline writeline_r

/* Public interface */
static void writeline_r(int fd, char c);


/* Internal implementation methods & data */
#define WL_MAX 2000
static pthread_key_t _wl_key;
static pthread_once_t _wl_once = PTHREAD_ONCE_INIT;

typedef struct {
	size_t count;
	char buffer[WL_MAX];
} _WLData;

static void _wl_destructor(void *);
static void _wl_first();

void writeline_r(int fd, char c) {
	_WLData *wl_data;

	pthread_once(&_wl_once, _wl_first);
	if((wl_data = pthread_getspecific(_wl_key)) == NULL) {
		wl_data = malloc(sizeof(_WLData));
		wl_data->count = 0;
		pthread_setspecific(_wl_key, wl_data);
	}

	wl_data->buffer[wl_data->count++] = c;
	if(c == '\n' || wl_data->count == WL_MAX) {
		ssize_t sum = 0;
		do {
			sum += write(fd, wl_data->buffer + sum, wl_data->count - sum);
		} while(sum < wl_data->count);

		wl_data->count = 0;
	}
}

void _wl_destructor(void *ptr) {
	free(ptr);
}

void _wl_first() {
	pthread_key_create(&_wl_key, _wl_destructor);
}
