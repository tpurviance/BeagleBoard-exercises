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
#include "i2c-dev.h"

 /****************************************************************
 * Constants
 ****************************************************************/
 
#define POLL_TIMEOUT (1000) //* 1000) /* 3 seconds */
#define MAX_BUF 64

#define GPIO_MOTOR_A 30
#define GPIO_MOTOR_B 60

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
 * Main
 ****************************************************************/
int main(int argc, char **argv, char **envp)
{
	struct pollfd fdset[2];
	int nfds = 2;
	int timeout, rc;
	int gpio_fd[2];
	char buf[MAX_BUF];
	int gpio_fd_motor_a, gpio_fd_motor_b;
	unsigned int gpio_motor_a = GPIO_MOTOR_A;
	unsigned int gpio_motor_b = GPIO_MOTOR_B;
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


	gpio_export(gpio_motor_a);
	gpio_set_dir(gpio_motor_a, "out");
	gpio_fd_motor_a = gpio_fd_open(gpio_motor_a, O_WRONLY);

	gpio_export(gpio_motor_b);
	gpio_set_dir(gpio_motor_b, "out");
	gpio_fd_motor_b = gpio_fd_open(gpio_motor_b, O_WRONLY);


	timeout = POLL_TIMEOUT;
	
	//unsigned int gpios[] = {GPIO_BUTTONS,GPIO_SCREEN};
	//unsigned int gpiofds[] = {gpio_fd_buttons, gpio_fd_screen};

	int cur_gpio = 0;
	int motor_a = 0;
	int motor_b = 0;
 
	while (keepgoing) {
/**
		memset((void*)fdset, 0, sizeof(fdset));

		fdset[0].fd = STDIN_FILENO;
		fdset[0].events = POLLIN;
		fdset[1].events = POLLPRI;


		fdset[1].fd = gpiofds[cur_gpio];
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
			switch (cur_gpio) {
				case 0:
					mat_try_move(UP);
					break;
				case 1:
					mat_try_move(DOWN);
					break;

			}
		}
**/		
		if (cur_gpio == 0)
			motor_a = !motor_a;
		else
			motor_b = !motor_b;

		gpio_set_value(gpio_motor_a, motor_a);
		gpio_set_value(gpio_motor_b, motor_b);

		usleep(50000);
		cur_gpio = (cur_gpio + 1) % 2;
		fflush(stdout);
	}

	gpio_fd_close(gpio_fd_motor_a);
	gpio_fd_close(gpio_fd_motor_b);
	return 0;
}

