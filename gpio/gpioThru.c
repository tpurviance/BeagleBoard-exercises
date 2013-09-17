/* Copyright (c) 2011, RidgeRun
 * All rights reserved.
 *
From https://www.ridgerun.com/developer/wiki/index.php/Gpio-int-test.c

 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the RidgeRun.
 * 4. Neither the name of the RidgeRun nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY RIDGERUN ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL RIDGERUN BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>	// Defines signal-handling functions (i.e. trap Ctrl-C)
#include "gpio-utils.h"

 /****************************************************************
 * Constants
 ****************************************************************/
 
#define POLL_TIMEOUT (3) //* 1000) /* 3 seconds */
#define MAX_BUF 64

#define GPIO_UP 60
#define GPIO_DOWN 48
#define GPIO_LEFT 3
#define GPIO_RIGHT 49

/****************************************************************
 * Global variables
 ****************************************************************/
int keepgoing = 1;	// Set to 0 when ctrl-c is pressed

/****************************************************************
 * signal_handler
 ****************************************************************/
void signal_handler(int sig);
// Callback called when SIGINT is sent to the process (Ctrl-C)
void signal_handler(int sig)
{
	printf( "Ctrl-C pressed, cleaning up and exiting..\n" );
	keepgoing = 0;
}


/****************************************************************
 * Etchasketch code
 ****************************************************************/

#define ROWS 8
#define COLS 8

int* matrix;
int pos_x = 0;
int pos_y = 0;

typedef enum {UP, DOWN, LEFT, RIGHT} Direction;

void mat_alloc(){
	matrix = (int*) calloc(ROWS*HEIGHT, sizeof(int));
}

void mat_free(){
	free(matrix);
}

void mat_do_move(int newx, int newy) {
	matrix[newx + newy * COLS] = 1;
	pos_x = newx;
	pos_y = newy;	
}

void mat_try_move(Direction dir){
	int dx = 0;
	int dy = 0;
	switch(dir){
		case UP:
			dy = -1;
			break;
		case DOWN:
			dy = 1;
			break;
		case LEFT:
			dx = -1;
			break;
		case RIGHT:
			dx = 1;				
			break;
	}
	int newx = pos_x + dx;
	int newy = pos_y + dy;
	if (newx >= 0 && newx <	COLS &&
		newy >= 0 && newy < ROWS) {
		mat_do_move(newx, newy);
	}
}

void mat_reset() {
	int x, y;
	for (x = 0; x < COLS; x++)
		for (y = 0; y < ROWS; y++)
			matrix[x + y * COLS] = 0;
}

/****************************************************************
 * Main
 ****************************************************************/
int main(int argc, char **argv, char **envp)
{
	struct pollfd fdset[2];
	int nfds = 2;
	int timeout, rc;
	int gpio_fd_up, gpio_fd_down, gpio_fd_left, gpio_fd_right;
	char buf[MAX_BUF];
	unsigned int gpio_up = GPIO_UP;
	unsigned int gpio_down = GPIO_DOWN;
	unsigned int gpio_left = GPIO_LEFT;
	unsigned int gpio_right = GPIO_RIGHT;
	int len;

/*
	if (argc < 2) {
		printf("Usage: gpio-int <gpio-pin>\n\n");
		printf("Waits for a change in the GPIO pin voltage level or input on stdin\n");
		exit(-1);
	}
*/

	// Set the signal callback for Ctrl-C
	signal(SIGINT, signal_handler);

	mat_alloc();


	gpio_export(gpio_up);
	gpio_set_dir(gpio_up, "in");
	gpio_set_edge(gpio_up, "rising");  // Can be rising, falling or both
	gpio_fd_up = gpio_fd_open(gpio_up, O_RDONLY);

	gpio_export(gpio_down);
	gpio_set_dir(gpio_down, "in");
	gpio_set_edge(gpio_down, "rising");  // Can be rising, falling or both
	gpio_fd_down = gpio_fd_open(gpio_down, O_RDONLY);

	gpio_export(gpio_left);
	gpio_set_dir(gpio_left, "in");
	gpio_set_edge(gpio_left, "falling");  // Can be rising, falling or both
	gpio_fd_left = gpio_fd_open(gpio_left, O_RDONLY);

	gpio_export(gpio_right);
	gpio_set_dir(gpio_right, "in");
	gpio_set_edge(gpio_right, "rising");  // Can be rising, falling or both
	gpio_fd_right = gpio_fd_open(gpio_right, O_RDONLY);

	timeout = POLL_TIMEOUT;
	
	unsigned int gpios[] = {GPIO_UP,GPIO_DOWN,GPIO_LEFT,GPIO_RIGHT};
	int cur_gpio = 0;
 
	while (keepgoing) {
		memset((void*)fdset, 0, sizeof(fdset));

		fdset[0].fd = STDIN_FILENO;
		fdset[0].events = POLLIN;
		fdset[1].events = POLLPRI;


		fdset[1].fd = gpios[cur_gpio];
		rc = poll(fdset, nfds, timeout);      

		if (rc < 0) {
			printf("\npoll() failed!\n");
			return -1;
		}
            
		if (fdset[1].revents & POLLPRI) {
			lseek(fdset[1].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[1].fd, buf, MAX_BUF);
			printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n",
				 gpios[cur_gpio], buf[0], len);
		}

		if (fdset[0].revents & POLLIN) {
			(void)read(fdset[0].fd, buf, 1);
			printf("\npoll() stdin read 0x%2.2X\n", (unsigned int) buf[0]);
		}

		cur_gpio = (cur_gpio + 1) % 4;
		fflush(stdout);
	}

	mat_free();
	gpio_fd_close(gpio_fd_up);
	gpio_fd_close(gpio_fd_down);
	gpio_fd_close(gpio_fd_left);
	gpio_fd_close(gpio_fd_right);
	return 0;
}

