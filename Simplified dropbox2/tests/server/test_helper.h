/**
 * Declaration of function to help with server testing.
 *
 * @file test_helper.h
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2021-01-08
 */

#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include "packet.h"
#include "settings.h"
#include <pthread.h>

/**
 *  Enum representing status of ended server
 */
enum server_status {
        SERVER_OK,      /**< server returned EXIT_SUCCESS */
        SERVER_ERROR,   /**< server returned EXIT_FAILURE */
        SERVER_STOPPED, /**< server was terminated by stop_server */
};

/**
 * Prepares everything needed to test server.
 *
 * Creates a new folder named @c name with a prefix `test_`, e.g.
 * for a @c name = "server", the function creates folder `test_server`.
 * `cwd` field of @c settings contains the path to that folder.
 *
 * Then it opens two pipes for communication. Proper ends of pipes are
 * stored in @c readfd , @c writefd and @c settings.
 *
 * Other fields of @c settings which were not set are zero.
 *
 * Finally this function returns file descriptor to the test folder.
 *
 * @note The @c name shall be unique between the tests.
 *
 * @param name     pointer to a null-terminated byte string
 * @param readfd   pointer to a int for storing read end file descriptor
 * @param writefd  pointer to a int for storing write end file descriptor
 * @param settings pointer to a settings structure
 * @return int     file descriptor of test folder
 */
int prepare_test(const char *name, int *readfd, int *writefd,
                 struct settings *settings);

/**
 * Closes and frees resources.
 *
 * @param settings pointer to a settings
 * @param pack     pointer to a allocated packet struct
 */
void clean_after_test(struct settings *settings, struct packet *pack);

/**
 * Starts server on thread @c th with @c settings.
 *
 * @param th       pointer to a thread structure
 * @param settings pointer to a settings
 */
void start_server(pthread_t *th, struct settings *settings);

/**
 * Terminate server by cancelling the thread @c th
 *
 * @param th a thread of running server
 */
void stop_server(pthread_t th);

/**
 * Gets the status of the stopped server and compares it with @c st
 *
 * @param th a thread of running server
 * @param st an expected status code
 */
void wait_server(pthread_t th, enum server_status st);

#endif //TEST_HELPER_H
