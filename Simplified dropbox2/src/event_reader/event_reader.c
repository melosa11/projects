#include "event_reader.h"

#include "log.h"

#include <stdlib.h>
#include <sys/signalfd.h>
#include <syslog.h>
#include <unistd.h>

static void set_signal_fd(int fd, struct pollfd *pollfd)
{
        pollfd->fd = fd;
        pollfd->events = POLLIN;
        pollfd->revents = 0;
}

static void insert_fds(size_t n, const struct pollfd fds[n],
                       struct pollfd pollfds[])
{
        for (size_t i = 0; i < n; i++) {
                pollfds[i] = fds[i];
        }
}

static bool main_loop(nfds_t nfds, struct pollfd pollfds[nfds],
                      event_reader_callback_t callback)
{
        while (true) {
                if (poll(pollfds, nfds, -1) == -1) {
                        log_error("poll");
                        break;
                }
                syslog(LOG_DEBUG, "Event was caught.");

                enum utils_loop_status status
                        = callback(nfds - 1, pollfds, &pollfds[nfds - 1]);
                syslog(LOG_DEBUG, "callback status: %d", status);

                if (status == UTILS_LOOP_ERROR)
                        return false;
                if (status == UTILS_LOOP_BREAK)
                        return true;

                syslog(LOG_DEBUG, "all pollfd checked");
        }
        syslog(LOG_DEBUG, "main event loop failed");
        return false;
}

bool event_reader(const sigset_t *mask, size_t n, const struct pollfd fds[n],
                  event_reader_callback_t callback)
{
        syslog(LOG_DEBUG, "Event loop started.");

        const nfds_t nfds = n + 1;
        struct pollfd *pollfds = malloc(nfds * sizeof(*pollfds));
        if (pollfds == NULL) {
                log_error("malloc");
                return false;
        }
        insert_fds(n, fds, pollfds);

        bool success = false;

        const int signal_fd = signalfd(-1, mask, SFD_NONBLOCK);
        if (signal_fd == -1) {
                log_error("signalfd");
                goto clean_pollfds;
        }
        set_signal_fd(signal_fd, &pollfds[n]);

        success = main_loop(nfds, pollfds, callback);

        if (close(signal_fd) == -1)
                log_error("close signal_fd");

clean_pollfds:
        free(pollfds);
        return success;
}
