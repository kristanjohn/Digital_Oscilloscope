/*
 * os_enet.c
 *
 *  Created on: 27 May 2016
 *      Author: Kristan Edwards
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <stdio.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/GPIO.h>

/* NDK BSD support */
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

/* Example/Board Header file */
#include "Board.h"

/* Define protypes */
extern Void tcpWorker(int clientfd);

#define NUMTCPWORKERS 2


/*
 *  ======== tcpHandler ========
 *  Creates new Task to handle new TCP connections.
 */
Void tcpHandler(UArg arg0, UArg arg1)
{
    int                     status;
    int                     clientfd;
    int                     server;
    char                    port[8];
    struct addrinfo        *results = NULL;
    struct addrinfo        *currAddr = NULL;
    struct addrinfo         hints;
    struct sockaddr_storage clientAddr;
    socklen_t               addrlen = sizeof(clientAddr);
    int                     optval;
    int                     optlen = sizeof(optval);

    /* Initialize local variables */
    server = -1;
    memset(&hints, 0, sizeof(hints));
    memset(&clientAddr, 0, addrlen);

    /*
     *  Use getaddrinfo for generic socket set up code. The family (AF_INET or
     *  AF_INET6) is passed in via arg1 and dictates whether the socket is an
     *  IPv4 or IPv6 socket.
     */
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = (int)arg1;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    sprintf(port, "%d", arg0);
    status = getaddrinfo(NULL, port, &hints, &results);
    if (status != 0) {
        System_printf("tcpHandler: Error: getaddrinfo failed (%d).\n", status);
        return;
    }

    /*
     *  Cycle through the list of address structs returned from getaddrinfo,
     *  trying each until we succeed:
     */
    for (currAddr = results; currAddr != NULL; currAddr = currAddr->ai_next) {
        server = socket(currAddr->ai_family, currAddr->ai_socktype,
                currAddr->ai_protocol);
        if (server == -1) {
            System_printf("Error: socket not created.\n");
            goto nextaddr;
        }

        status = bind(server, currAddr->ai_addr, currAddr->ai_addrlen);
        if (status == -1) {
            System_printf("Error: bind failed.\n");
            goto nextaddr;
        }

        status = listen(server, NUMTCPWORKERS);
        if (status == -1) {
            System_printf("Error: listen failed.\n");
            goto nextaddr;
        }

        optval = 1;
        if (setsockopt(server, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen)
                == -1) {
            System_printf("Error: setsockopt failed\n");
            goto nextaddr;
        }

        /* We have a valid socket set up */
        break;

nextaddr:
        if (server > 0) {
            close(server);
        }
        server = -1;
    }

    freeaddrinfo(results);

    if (server == -1) {
        System_printf("Error: could not create a valid socket\n");
        return;
    }

    while ((clientfd =
            accept(server, (struct sockaddr *)&clientAddr, &addrlen)) != -1) {

        System_printf("PC Connected.\n");
        tcpWorker(clientfd); // Won't ever leave this function

    }

    System_printf("Error: accept failed.\n");

    if (server > 0) {
        close(server);
    }
}

