#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>  // socket, setsockopt, accept, send, recv
#include <sys/socket.h>

#include <signal.h>
#include <pthread.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "unitooth.h"

pthread_t server_thread;
pthread_t rcv_handler_theread;

static int serv_sock; 		// server socket
static int client_connected = 0;// boolean to signify that client is connected
static int client = -1;		// client fd
static callbackStr onData = NULL;
static char recv_buf[1024];

int serverR()
{
	return pthread_create(&server_thread,
			NULL,
			server_daemonR,
		       	NULL);
}

void *server_daemonR()
{
	char command[64];

	// Turn on discoverable
	sprintf(command, "echo -e 'discoverable on\nquit' | bluetoothctl");
	system(command);

	struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
	//char buf[1024] = { 0 };
	socklen_t opt = sizeof(rem_addr);

	//allocate socket
	serv_sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_RFCOMM);

	// bind socket to port 1 of the first available 
	// bluetooth adapter
	loc_addr.rc_family = AF_BLUETOOTH;
	loc_addr.rc_bdaddr = *BDADDR_ANY;
	loc_addr.rc_channel = (uint8_t) 1;

	bind(serv_sock, (struct sockaddr *)&loc_addr, sizeof(loc_addr));

	// put socket into listening mode
	listen(serv_sock, 1);

	// accept one connection
	client = accept(serv_sock, (struct sockaddr *)&rem_addr, &opt);
	client_connected = 1;	
	pthread_create(&rcv_handler_theread,
			NULL,
			recv_handleR,
			NULL);
	return NULL;
}

int sendR (char *msg)
{
	if(!client_connected)
	{
		return -1;
	}
	return write(client, msg, strlen(msg));

}

void *recv_handleR()
{
	int checkVal = -1;
	while(1)
	{
		checkVal = read(client, recv_buf ,sizeof(recv_buf));
		if(checkVal < 0)
			break;
		onData(recv_buf); 
	}
	return NULL;	
}

void set_callbackR (callbackStr callback)
{
	onData = callback;
}

