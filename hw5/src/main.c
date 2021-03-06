#include "debug.h"
#include "client_registry.h"
#include "transaction.h"
#include "store.h"
#include "server.h"
#include "helpers.h"
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "csapp.h"
#include "protocol.h"

static void terminate(int status);

CLIENT_REGISTRY *client_registry;

int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    char optval;
    int port = 0;
    int listenfd;
    while(optind < argc) {
        if ((optval = getopt (argc, argv, "p:")) != -1) {
            if (optval == 'p' && optarg != NULL) {
                port++;
                listenfd = Open_listenfd(optarg);
            }
        }
    }

    // Perform required initializations of the client_registry,
    // transaction manager, and object store.
    client_registry = creg_init();
    trans_init();
    store_init();

    if (port != 1) {
        fprintf(stderr, "One port number is required.\n");
        terminate(EXIT_FAILURE);
    }

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function xacto_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.
    struct sigaction sighupStruct;
    sighupStruct.sa_handler = SIGHUPHandler;
    sigaction(SIGHUP, &sighupStruct, NULL);

    int* connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA*) &clientaddr, &clientlen);
        pthread_create(&tid, NULL, xacto_client_service, connfdp);
    }

    fprintf(stderr, "You have to finish implementing main() "
        "before the Xacto server will function.\n");

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    trans_fini();
    store_fini();

    debug("Xacto server terminating");
    exit(status);
}

void SIGHUPHandler(int signal) {
    terminate(EXIT_SUCCESS);
}