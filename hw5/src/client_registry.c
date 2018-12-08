#include "client_registry.h"
#include "csapp.h"

/*
 * A client registry keeps track of the file descriptors for clients
 * that are currently connected.  Each time a client connects,
 * its file descriptor is added to the registry.  When the thread servicing
 * a client is about to terminate, it removes the file descriptor from
 * the registry.  The client registry also provides a function for shutting
 * down all client connections and a function that can be called by a thread
 * that wishes to wait for the client count to drop to zero.  Such a function
 * is useful, for example, in order to achieve clean termination:
 * when termination is desired, the "main" thread will shut down all client
 * connections and then wait for the set of registered file descriptors to
 * become empty before exiting the program.
 */
typedef struct client_registry {
    int* fdArray;                   // Array of all registered file descriptors
    int fdArraySize;                // Number of registered file descriptors
    int firstFd;                    // First registered file descriptor in array
    int lastFd;                     // Last registered file descriptor in array
    sem_t mutex;                    // Semaphore to provide access to array
    sem_t elements;                 // Semaphore to count available file descriptors
} CLIENT_REGISTRY;

/*
 * Initialize a new client registry.
 *
 * @return  the newly initialized client registry.
 */
CLIENT_REGISTRY *creg_init() {
    CLIENT_REGISTRY* client_registry = malloc(sizeof(CLIENT_REGISTRY));
    client_registry->fdArray = malloc(sizeof(int));
    client_registry->fdArraySize = 0;
    client_registry->firstFd = 0;
    client_registry->lastFd = 0;
    sem_init(&client_registry->mutex, 0, 1);
    sem_init(&client_registry->elements, 0, 0);
    return client_registry;
}
/*
 * Finalize a client registry.
 *
 * @param cr  The client registry to be finalized, which must not
 * be referenced again.
 */
void creg_fini(CLIENT_REGISTRY *cr) {
    free(cr->fdArray);
    free(cr);
}

/*
 * Register a client file descriptor.
 *
 * @param cr  The client registry.
 * @param fd  The file descriptor to be registered.
 */
void creg_register(CLIENT_REGISTRY *cr, int fd) {
    P(&cr->mutex);
    if (cr->fdArraySize == 0) {
        cr->fdArray[0] = fd;
        cr->fdArraySize = 1;
        cr->firstFd = fd;
    }
    else {
        cr->fdArraySize += 1;
        cr->fdArray = realloc(cr, sizeof(int) * cr->fdArraySize);
        cr->fdArray[cr->fdArraySize - 1] = fd;
    }
    cr->lastFd = fd;
    V(&cr->mutex);
    V(&cr->elements);
}

/*
 * Unregister a client file descriptor, alerting anybody waiting
 * for the registered set to become empty.
 *
 * @param cr  The client registry.
 * @param fd  The file descriptor to be unregistered.
 */
void creg_unregister(CLIENT_REGISTRY *cr, int fd) {
    P(&cr->elements);
    P(&cr->mutex);
    int i = 0;
    for (i = 0; i < cr->fdArraySize; i++) {
        if (cr->fdArray[i] == fd) {
            cr->fdArraySize -= 1;
            if (cr->fdArraySize == 0) {
                cr->fdArray[0] = 0;
                cr->firstFd = 0;
                cr->lastFd = 0;
            }
            else  {
                memmove(cr + i, cr + i + 1, sizeof(int) * (cr->fdArraySize - i));
                cr = realloc(cr, sizeof(int) * cr->fdArraySize);
                cr->firstFd = cr->fdArray[0];
                cr->lastFd = cr->fdArray[cr->fdArraySize - 1];
            }
            break;
        }
    }
    V(&cr->mutex);
}

/*
 * A thread calling this function will block in the call until
 * the number of registered clients has reached zero, at which
 * point the function will return.
 *
 * @param cr  The client registry.
 */
void creg_wait_for_empty(CLIENT_REGISTRY *cr) {
    P(&cr->mutex);
    while (cr->fdArraySize != 0) {
        sleep(1);
    }
    V(&cr->mutex);
}

/*
 * Shut down all the currently registered client file descriptors.
 *
 * @param cr  The client registry.
 */
void creg_shutdown_all(CLIENT_REGISTRY *cr) {
    P(&cr->mutex);
    int i = 0;
    for (i = 0; i < cr->fdArraySize; i++) {
        shutdown(cr->fdArray[i], SHUT_RD);
    }
    V(&cr->mutex);
}