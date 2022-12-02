/**
 * @file server.h
 * @brief Server interface for processing clients requests
 * @author Matej Kleman (xkleman@fi.muni.cz)
 * @date 2020-12-31
 */
#ifndef SERVER_H
#define SERVER_H

#include "settings.h"

/**
 * @brief Entry point of the server.
 *
 * @param  settings pointer to configuration structure
 *
 * @return standard exit code, EXIT_SUCCESS or EXIT_FAILURE
 */
int server_main(struct settings *settings);

#endif //SERVER_H
