/**
 * @file client.h
 * @brief Module encapulating the client functionality
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2021-08-05
 */
#ifndef CLIENT_H
#define CLIENT_H

#include "settings.h"

/**
 * @brief Entry point of the client.
 *
 * @param  settings pointer to configuration structure
 *
 * @return standard exit code, EXIT_SUCCESS or EXIT_FAILURE
 */
int client_main(struct settings *settings);

#endif //CLIENT_H
