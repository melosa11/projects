/**
 * @file copy.h
 * @brief Module for copying files from SOURCE folder
 * @author Peter Mercell
 * @date 2021-08-05
 */
#ifndef COPY_H
#define COPY_H

#include "traverse_data.h"
#include "watcher_list.h"

/**
 * Makes the whole process of copying a regular file to the server.
 *
 * @param data  struct holding data, which are used when traversing a folder
 * @return      FTW_CONTINUE on success
 *              FTW_STOP on failure
 */
int client_copy_regular_file(const struct client_traverse_data data);

/**
 * Traversal through all files and copying them into the server.
 *
 * @param inot_fd   inotify file descriptor
 * @param watchers  pointer to the list of watchers
 * @param settings  pointer to the configuration struct
 * @return          true on success;
 *                  false on failure
 */
bool client_copy_files(int inot_fd, client_watcher_list *watchers,
                       const struct settings *settings);

#endif //COPY_H
