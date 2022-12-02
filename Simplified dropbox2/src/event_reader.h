/**
 * @file event_reader.h
 * @brief Event handling module
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2021-08-04
 */

#ifndef EVENT_READER_H
#define EVENT_READER_H

#include <poll.h>
#include <signal.h>
#include <stdbool.h>

#include "utils.h"

/**
 * @brier Function called when an event occurs on file descriptors
 *
 * @param pollfds[n]     an array of read file descriptors
 * @param singal_pollfd  file descriptor created by signalfd(2)
 *
 * @return   UTILS_LOOP_CONTINUE - indicating to go on
 *           UTILS_LOOP_BREAK    - to exit the event loop
 *           UTILS_LOOP_ERROR    - an error occurred and the event loop
 *                                 cannot continue
 */
typedef enum utils_loop_status (*event_reader_callback_t)(
        nfds_t n, struct pollfd pollfds[n], struct pollfd *singal_pollfd);

#define EVENT_READER_CALLBACK(NAME)                                            \
        enum utils_loop_status NAME(nfds_t n, struct pollfd pollfds[n],        \
                                    struct pollfd *signal_pollfd)

/**
 * Monitor read events on file descriptors @c read_fds and signals specified
 * in @c mask.
 *
 * @param mask      pointer to the mask of signals to monitored
 * @param n         number of read file descriptors
 * @param fds[n]    array of poll file descriptors to monitor
 *                  specified events
 *
 * @param callback  function for processing poll file descriptors
 *                  corresponding to the @c read_fds and signals
 *                  specified in @c mask
 *
 * @return true  if the &c callback wanted to end the event_loop
 *         false if an error occurred
 */
bool event_reader(const sigset_t *mask, size_t n, const struct pollfd fds[n],
                  event_reader_callback_t callback);

#endif //EVENT_READER_H
