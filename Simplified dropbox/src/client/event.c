#include "event.h"

#include "copy.h"
#include "traverse_data.h"
#include "send.h"

#include "event_reader.h"
#include "log.h"
#include "utils.h"

#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>

/**
 * Finds a name of the file among all of the file watchers.
 *
 * @return                name when file was found;
 *                        NULL otherwise
 */
static const char *find_name(client_watcher_list *watchers, int needle_wd)
{
        const struct client_watcher *w = client_watcher_list_find_last(
                watchers,
                (struct client_watcher){ .name = NULL, .wd = needle_wd });

        if (w == NULL)
                syslog(LOG_DEBUG, "Find name returning NULL.");

        return w->name;
}

/**
 * @brief Sends message with given code and name of file
 *
 * @param name name of file that is affected by code
 * @param code instruction for server
 * @param settings struct holding information about behaviour of the program
 * @return true
 * @return false
 */
static bool send_change_file(const char *name, enum packet_msg_code code,
                             const struct settings *settings)
{
        return log_packet_send(settings->write_fd, code, strlen(name) + 1,
                               (const unsigned char *)name);
}

/**
 * This function changes meta data of a file.
 *
 * @param name      name of the file
 * @param data      struct holding data, which are used when traversing a folder
 * @return          true on success;
 *                  false on failure
 */
static bool change_metadata(const char *name,
                            const struct client_traverse_data *data)
{
        if (fstatat(data->settings->dirfd, name, data->sb, 0) == -1) {
                log_warning("fstatat");
                return true;
        }

        if (!send_change_file(name, MSG_CHANGE_FILE, data->settings))
                return false;
        if (client_send_timestamps(data) == FTW_STOP
            || client_send_done(data) == FTW_STOP)
                return false;

        if (!send_change_file(name, MSG_CHANGE_FILE, data->settings))
                return false;
        if (client_send_owner(data) == FTW_STOP
            || client_send_done(data) == FTW_STOP)
                return false;

        if (!send_change_file(name, MSG_CHANGE_FILE, data->settings))
                return false;
        if (client_send_permission_modes(data) == FTW_STOP
            || client_send_done(data) == FTW_STOP)
                return false;

        return true;
}

/**
 * IN_CLOSE_WRITE event
 *
 * @param name name of processed file
 * @param data traverse structure
 * @return true if deleting and copying succeeded
 * @return false otherwise;
 */
static bool close_write(const char *name,
                        const struct client_traverse_data *data)
{
        if (fstatat(data->settings->dirfd, name, data->sb, 0) == -1) {
                log_warning("fstatat");
                return true;
        }

        if (client_send_delete_file(data) == FTW_STOP)
                return false;

        const int copy_file = client_copy_regular_file(*data);
        if (copy_file != FTW_CONTINUE) {
                syslog(LOG_DEBUG, "copying of file failed");
                return false;
        }
        return true;
}

/**
 * IN_MOVED_IN or IN_CREATE event
 *
 * @param name name of processed file
 * @param data traverse structure
 * @return true if copying succeeded;
 * @return false otherwise;
 */
static bool moved_to_create(const char *name,
                            const struct client_traverse_data *data)
{
        syslog(LOG_DEBUG, "IN_CREATE || IN_MOVED_TO");
        if (fstatat(data->settings->dirfd, name, data->sb, 0) == -1) {
                log_warning("fstatat");
                return true;
        }

        syslog(LOG_DEBUG, "STAT SUCCESS");
        int copy_file = client_copy_regular_file(*data);
        if (copy_file != FTW_CONTINUE) {
                syslog(LOG_DEBUG, "copying of file failed");
                return false;
        }
        return true;
}

static bool add_watcher(const struct settings *settings,
                        const char *relative_path, int inot_fd,
                        client_watcher_list *watchers)
{
        char *full_path = NULL;
        if (asprintf(&full_path, "%s/%s", settings->cwd, relative_path) == -1) {
                log_error("asprintf");
                return false;
        }

        const struct client_watcher_data watcher_data = {
                .fpath = full_path,
                .inot_fd = inot_fd,
                .relative_path = relative_path,
        };

        bool success = false;
        struct client_watcher watcher;
        if (!watcher_create(&watcher, &watcher_data))
                goto clean;

        if (!client_watcher_list_push_back(watchers, &watcher)) {
                log_error("client_watcher_list_push_back");
                goto clean;
        }
        success = true;
clean:
        free(full_path);
        return success;
}

static void remove_watcher(const char *name, int inot_fd,
                           client_watcher_list *watchers)
{
        syslog(LOG_DEBUG, "removing '%s' from watch list", name);
        const struct client_watcher needle = { .wd = -1, .name = name };
        const struct client_watcher *w
                = client_watcher_list_find_last(watchers, needle);
        if (w == NULL)
                return;

        syslog(LOG_DEBUG, "found watcher: wd: '%d' | name : '%s'", w->wd,
               w->name);
        // can succeed if there exists hardlink to that file.
        // The i-node itself wasn't deleted.
        if (inotify_rm_watch(inot_fd, w->wd) == -1)
                log_warning("inotify_rm_watch");
}

/**
 * This function process one particular event, which can happen.
 *
 * @param event     system structure inotify_event
 * @param name      name of the file
 * @param data      struct holding data, which are used when traversing a folder
 * @param inot_fd   inotify file descriptor
 * @param watchers  pointer to the list of watchers
 * @return          true on success;
 *                  false on failure
 */

static bool process_event(const struct inotify_event *event, const char *name,
                          const struct client_traverse_data *data, int inot_fd,
                          client_watcher_list *watchers)
{
        syslog(LOG_DEBUG, "process_event started");
        if (event->mask & IN_CLOSE_WRITE) {
                if (!close_write(name, data))
                        return false;
        }
        if (event->mask & IN_ATTRIB) {
                if (!change_metadata(name, data))
                        return false;
        }
        if (event->mask & IN_MOVED_FROM || event->mask & IN_DELETE) {
                syslog(LOG_DEBUG, "IN_DELETE || IN_MOVED_FROM");
                remove_watcher(name, inot_fd, watchers);
                if (client_send_delete_file(data) == FTW_STOP)
                        return false;
        }
        if (event->mask & IN_MOVED_TO || event->mask & IN_CREATE) {
                if (!add_watcher(data->settings, name, inot_fd, watchers)
                    || !moved_to_create(name, data))
                        return false;
        }
        syslog(LOG_DEBUG, "mask processed successfully");
        return true;
}

/**
 * @brief Get the name of the file or proccessed directory
 */
static const char *get_name(client_watcher_list *watchers,
                            const struct inotify_event *event)
{
        const char *name;
        if (event->mask & IN_CLOSE_WRITE || event->mask & IN_ATTRIB) {
                name = find_name(watchers, event->wd);
                syslog(LOG_DEBUG, "Event happened on file");
        } else {
                name = event->name;
                syslog(LOG_DEBUG, "Event happened in directory.");
        }
        return name;
}

static enum utils_loop_status
prepare_process_event(const struct inotify_event *event, size_t *nptr,
                      struct packet **packet_buffptr, int inot_fd,
                      client_watcher_list *watchers,
                      const struct settings *settings)
{
        if (event->mask & IN_ISDIR)
                return UTILS_LOOP_CONTINUE;
        const char *name = get_name(watchers, event);

        syslog(LOG_DEBUG, "event file name: %s", name);
        struct stat sb;
        const struct client_traverse_data data = {
                .settings = settings,
                .packet_buffptr = packet_buffptr,
                .nptr = nptr,
                .fpath = name,
                .ftwbuf = NULL,
                .sb = &sb,
        };
        const bool success
                = process_event(event, name, &data, inot_fd, watchers);
        if (!success)
                syslog(LOG_DEBUG, "process_event failure");

        return success;
}

/**
 * @brief This function goes through all events and process them.
 */
static bool processing(size_t size, const unsigned char events_raw[size],
                       int inot_fd, client_watcher_list *watchers,
                       const struct settings *settings)
{
        const unsigned char *buff_ptr = events_raw;
        size_t n = 0;
        struct packet *packet_buff = NULL;
        bool success = true;
        while (size > 0) {
                struct inotify_event *event = (struct inotify_event *)buff_ptr;
                size_t event_len = sizeof(struct inotify_event) + event->len;
                buff_ptr += event_len;
                size -= event_len;

                if (!prepare_process_event(event, &n, &packet_buff, inot_fd,
                                           watchers, settings)) {
                        success = false;
                        break;
                }
        }
        free(packet_buff);
        return success;
}

static ssize_t get_events_buffer_size(int inot_fd)
{
        int size;
        if (ioctl(inot_fd, FIONREAD, &size) < 0) {
                log_error("ioctl");
                return -1;
        }
        return size;
}

static bool read_events(int inot_fd, size_t *buff_size,
                        unsigned char **buff_ptr)
{
        const ssize_t size = get_events_buffer_size(inot_fd);
        if (size == -1)
                return false;

        unsigned char *read_buff = malloc(size * sizeof(unsigned char));
        if (read_buff == NULL)
                return false;

        ssize_t bytes_read;
        syslog(LOG_DEBUG, "start reading event");
        if ((bytes_read = utils_read(inot_fd, size, read_buff)) == -1) {
                log_error("read");
                return false;
        }
        log_assert(bytes_read == size);
        syslog(LOG_DEBUG, "end reading events, read: %ld", bytes_read);

        *buff_size = (size_t)bytes_read;
        *buff_ptr = read_buff;
        return true;
}

/**
 * This function goes through all events and process them.
 */
static bool process_all_events(client_watcher_list *watchers, int inot_fd,
                               const struct settings *settings)
{
        size_t size = 0;
        unsigned char *events_raw = NULL;

        if (!read_events(inot_fd, &size, &events_raw))
                return false;

        const bool success
                = processing(size, events_raw, inot_fd, watchers, settings);

        free(events_raw);
        return success;
}

bool client_event_loop(client_watcher_list *watchers, int inot_fd,
                       const struct settings *settings)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        EVENT_READER_CALLBACK(callback)
        {
                log_assert(n == 2);

                struct pollfd *inotify_pollfd = &pollfds[0];
                struct pollfd *connection_pollfd = &pollfds[1];

                syslog(LOG_DEBUG, "inot_fd: %d | sock_fd: %d | signal_fd : %d",
                       inotify_pollfd->revents, connection_pollfd->revents,
                       signal_pollfd->revents);

                if (connection_pollfd->revents & POLLRDHUP
                    || connection_pollfd->revents & POLLERR) {
                        syslog(LOG_ERR, "connection to the server closed");
                        return UTILS_LOOP_ERROR;
                }

                if (inotify_pollfd->revents & POLLIN
                    && !process_all_events(watchers, inot_fd, settings)) {
                        syslog(LOG_DEBUG, "process_all_event failed");
                        return UTILS_LOOP_ERROR;
                }

                if (signal_pollfd->revents & POLLIN) {
                        syslog(LOG_DEBUG, "termination signal caught");
                        return UTILS_LOOP_BREAK;
                }
                return UTILS_LOOP_CONTINUE;
        }
#pragma GCC diagnostic pop

        const struct pollfd fds[] = {
                { .fd = inot_fd, .events = POLLIN },
                { .fd = settings->read_fd, .events = POLLRDHUP },
        };
        return event_reader(&settings->mask, sizeof(fds) / sizeof(*fds), fds,
                            callback);
}
