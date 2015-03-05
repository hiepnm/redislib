/*
 * test.c
 *
 *  Created on: Jun 20, 2014
 *      Author: hiep
 */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include "lib/ae.h"
#include "lib/adlist.h"
#include "lib/anet.h"
#include "lib/sds.h"


#define TEST_MAX_CLIENT 10000

typedef struct {
	aeEventLoop* el;
	int sfd;
	int port;
	char bindaddr[16];
	char neterr[ANET_ERR_LEN];
	int tcpkeepalive;
	list* clients;
	int maxclients;
} testserver;
typedef struct {
	int cfd;
	sds requestbuf;
} testclient;

testserver server;

void readRequestFromClient(aeEventLoop* el, int fd, void* privData, int mask) {
	testclient* c = (testclient*)privData;

}
testclient* createClient(int fd) {
	testclient* c = zmalloc(sizeof(testclient));
	if (fd != -1) {
		anetNonBlock(NULL, fd);
		anetEnableTcpNoDelay(NULL, fd);
		if (server.tcpkeepalive)
			anetKeepAlive(NULL, fd, server.tcpkeepalive);
		if (aeCreateFileEvent(server.el, fd, AE_READABLE, readRequestFromClient, c) == AE_ERR) {
			close(fd);
			zfree(c);
			return NULL;
		}
	}
	c->cfd = fd;
	c->requestbuf = sdsempty();
	if (fd != -1)
		listAddNodeTail(server.clients, c);

	return c;
}



void acceptTcpHandler(aeEventLoop* el, int fd, void* privData, int mask) {
	char cip[INET6_ADDRSTRLEN];
	int cport;
	int cfd = anetTcpAccept(server.neterr, fd, cip, sizeof(cip), &cport);
	if (cfd == ANET_ERR) {
		fprintf(stderr, "Accepting client connetion: %s", server.neterr);
		return;
	}
	testclient* c;
	if ((c= createClient(cfd))==NULL){
		fprintf(stderr, "Error registering fd event for the new client: %s (fd=%d)", strerror(errno), fd);
		return;
	}
	if (listLength(server.clients) > server.maxclients) {
		char* err = "-ERR max number of clients reached\r\n";
	}
}
int main(int argc, char **argv) {
	server.maxclients = TEST_MAX_CLIENT;
	server.el = aeCreateEventLoop(server.maxclients + 32+96);
	strcpy(server.bindaddr, "127.0.0.1");
	server.port = 8888;
	server.tcpkeepalive = 0;
	server.clients = listCreate();
	server.sfd = anetTcpServer(server.neterr, server.port, server.bindaddr, 16);
	if (server.sfd == ANET_ERR) {
		fprintf(stderr, "Creating Server TCP listening socket %s:%d\t%s", server.bindaddr ? server.bindaddr : "*", server.port, server.neterr);
		exit(EXIT_FAILURE);
	}
	if (aeCreateFileEvent(server.el, server.sfd, AE_READABLE, acceptTcpHandler, NULL)) {
		fprintf(stderr, "Unrecoverable error creating %d file event", server.sfd);
		exit(EXIT_FAILURE);
	}
	aeMain(server.el);
}















