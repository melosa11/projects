/**
 * @file send.h
 * Module for sending messages to the server.
 *
 * All functions take struct holding data, which are used when
 * traversing a folder.
 *
 * They return NEXT_STEP, FTW_CONTINUE on success;
 *             FTW_STOP on failure;

 *
 * @author Peter Mercell
 * @date 2021-08-05
 */
#ifndef SEND_H
#define SEND_H

#include "traverse_data.h"

#define NEXT_STEP (-1)

/** Sends data about owner of the file. */
int client_send_owner(const struct client_traverse_data *data);

/** Sends permissions of the file. */
int client_send_permission_modes(const struct client_traverse_data *data);

/** Sends timestamps of the file. */
int client_send_timestamps(const struct client_traverse_data *data);

/** Sends name of file to delete. */
int client_send_delete_file(const struct client_traverse_data *data);

/**
 * Sends information about creating of the file.
 *
 * @param file_path  relative path to a file for server
 */
int client_send_create_file(const struct client_traverse_data *data,
                            const char *file_path);

/** Sends blocks to server of predefined size */
int client_send_blocks(const struct client_traverse_data *data);

/** Sends done message to a server. */
int client_send_done(const struct client_traverse_data *data);

#endif //SEND_H
