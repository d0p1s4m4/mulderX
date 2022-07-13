/*
 * Copyright (c) 2022 d0p1 <contact@d0p1.eu>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <mulderX/client.h>
#include <mulderX/backend.h>

static int
open_socket(void)
{
	int fd;
	uint16_t port;
	struct sockaddr_in addr;

	port = 6000;
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		return (-1);
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	do
	{
		addr.sin_port = htons(port);
		port++;
	}
	while (bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) != 0);

	printf("listen on port: %d\n", port - 1);

	listen(fd, 10);
	return (fd);
}

static int
register_client_socket(Client *client, fd_set *fds, int max_fd)
{
	Client *tmp;

	if (client == NULL)
	{
		return (max_fd);
	}

	for (tmp = client; tmp != NULL; tmp = tmp->next)
	{
		if (tmp->socket > 0)
		{
			FD_SET(tmp->socket, fds);
			if (tmp->socket > max_fd)
			{
				max_fd = tmp->socket;
			}
		}
	}

	return (max_fd);
}

static void
client_loop(fd_set *fds, Client *client)
{
	Client *tmp;
	uint32_t buff;
	uint8_t c;

	for (tmp = client; tmp != NULL; tmp = tmp->next)
	{
		if (FD_ISSET(tmp->socket, fds))
		{
			if (tmp->state == CLIENT_UNITIALIZED)
			{
				read(tmp->socket, &c, sizeof(c));
				printf("'%c'\n", c);
			}
			else {
				read(tmp->socket, &buff, sizeof(buff));
				printf("%X\n", buff);
			}
		}
	}

}

static void *
server_main_loop(void *ptr)
{
	fd_set fds;
	int max_fd;
	Client *client_list;
	int tmp_sock;
	int master;

	master = *((int *)ptr);
	client_list = NULL;
	for (;;)
	{
		FD_ZERO(&fds);
		FD_SET(master, &fds);
		max_fd = register_client_socket(client_list, &fds, master);		

		if (select(max_fd + 1, &fds, NULL, NULL, NULL) < 0)
		{
			perror("select");
		}

		if (FD_ISSET(master, &fds))
		{
			tmp_sock = accept(master, NULL, NULL);
			if (tmp_sock < 0)
			{
				perror("accept");
			}
			client_push(&client_list, client_new(tmp_sock));
		}

		client_loop(&fds, client_list);
	}

	return (NULL);
}

int
main(void)
{
	int fd;
	pthread_t server_thread;

	if (backend_initialize() < 0)
	{
		return (EXIT_FAILURE);
	}

	fd = open_socket();
	if (fd < 0)
	{
		return (EXIT_FAILURE);
	}

	pthread_create(&server_thread, NULL, server_main_loop, (void *)&fd);

	backend_loop();

	pthread_join(server_thread, NULL);

	return (EXIT_SUCCESS);
}
