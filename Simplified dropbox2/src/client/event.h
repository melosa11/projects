/**
 * @file event.h
 * @brief Event loop of the client
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2021-08-05
 */
#ifndef EVENT_H
#define EVENT_H

#include "watcher_list.h"
#include "traverse_data.h"

/**
 * Prepares then runs event loop.
 *
 * @param watchers  pointer to the list of watchers
 * @param inot_fd   inotify file descriptor
 * @param settings  struct holding information about behaviour of the program
 * @return          true on success;
 *                  false on failure
 */
bool client_event_loop(client_watcher_list *watchers, int inot_fd,
                       const struct settings *settings);

#endif //EVENT_H
