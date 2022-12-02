/**
 * @file server.c
 * @brief Server interface for processing clients requests.
 * @author Matej Kleman (xkleman@fi.muni.cz)
 * @date 2021-01-01
 */
#include "server.h"

#include "file_info.h"
#include "operation.h"

#include "event_reader.h"
#include "log.h"
#include "utils.h"

#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/statvfs.h>

/**
 * Opens directory on which server performs requested commands.
 *
 * @param settings Struct holding information about behaviour of the program
 * @param file_info Struct containing all
 *                  important information about current file
 * @return true on success;
           false if open failed;
 */
static bool open_dir(const struct settings *settings,
                     struct server_file_info *file_info)
{
        // historical reasons
        file_info->dirfd = settings->dirfd;
        return true;
}
static bool get_filesystem_block_size(struct settings *settings,
                                      const struct server_file_info *file_info)
{
        struct statvfs stats;
        if (fstatvfs(file_info->dirfd, &stats) == -1)
                return false;
        settings->fs_block_size = stats.f_bsize;
        return true;
}
/**
 * Sends information about filesystem to the client.
 *
 * @param settings Struct holding information about behaviour of the program
 * @param file_info Struct containing all
 *                  important information about current file
 * @return true on success;
           false if packet_send failed;
 */
static bool send_settings(const struct settings *settings)
{
        const struct packet_payload_settings payload = {
                .fs_block_size = settings->fs_block_size,
        };

        if (!log_packet_send(settings->write_fd, MSG_SETTINGS,
                             sizeof(struct packet_payload_settings),
                             (const unsigned char *)&payload))
                return false;

        syslog(LOG_DEBUG, "Settings sent successfully.");
        return true;
}

static enum operation_status process_packet(const struct settings *settings,
                                            struct server_file_info *file_info,
                                            const struct packet *packet)
{
        enum operation_status operation_result
                = server_operation_process_operation(settings, file_info,
                                                     packet);

        if (operation_result == OPERATION_NOK)
                syslog(LOG_ERR, "Packet processing failed.");

        if (operation_result == OPERATION_END)
                syslog(LOG_DEBUG, "Reading ended.");

        return operation_result;
}

static bool read_packets_until(int read_fd, struct settings *settings,
                               struct server_file_info *file_info,
                               struct packet **packet_ptr,
                               size_t *packet_size_ptr,
                               bool (*p)(struct server_file_info *))
{
        if (p != NULL && p(file_info))
                return true;

        while (log_packet_read(read_fd, packet_ptr, packet_size_ptr)) {
                enum operation_status operation_result
                        = process_packet(settings, file_info, *packet_ptr);

                if (operation_result == OPERATION_NOK)
                        return false;
                if (operation_result == OPERATION_END)
                        return true;
                if (p != NULL && p(file_info))
                        return true;
        }
        return false;
}

static bool read_packets_loop(int read_fd, struct settings *settings,
                              struct server_file_info *file_info,
                              struct packet **packet_ptr,
                              size_t *packet_size_ptr)
{
        return read_packets_until(read_fd, settings, file_info, packet_ptr,
                                  packet_size_ptr, NULL);
}

static void close_socket(int sock_fd)
{
        log_assert(sock_fd != -1);
        if (close(sock_fd) == -1)
                log_warning("connection socket");
}

static void close_connection(struct pollfd *pollfd)
{
        close_socket(pollfd->fd);
        pollfd->fd = -1;
}

static enum utils_loop_status
_connection_callback(struct settings *settings,
                     struct server_file_info *file_info,
                     struct pollfd *connection_pollfd, size_t *packet_size_ptr,
                     struct packet **packet_ptr)
{
        if (connection_pollfd->revents & POLLERR) {
                syslog(LOG_ERR, "error occurred on the connection");
                close_connection(connection_pollfd);
                return UTILS_LOOP_ERROR;
        }

        if (connection_pollfd->revents & POLLRDHUP) {
                syslog(LOG_DEBUG, "connection ended");
                bool success = read_packets_loop(connection_pollfd->fd,
                                                 settings, file_info,
                                                 packet_ptr, packet_size_ptr);
                close_connection(connection_pollfd);
                return success ? UTILS_LOOP_BREAK : UTILS_LOOP_ERROR;
        }

        if (connection_pollfd->revents & POLLIN) {
                if (!log_packet_read(connection_pollfd->fd, packet_ptr,
                                     packet_size_ptr))
                        return UTILS_LOOP_ERROR;

                enum operation_status status
                        = process_packet(settings, file_info, *packet_ptr);

                if (status == OPERATION_NOK)
                        return UTILS_LOOP_ERROR;

                if (status == OPERATION_END) {
                        return UTILS_LOOP_BREAK;
                }
        }

        return UTILS_LOOP_CONTINUE;
}

static void reject_connection(int client_fd)
{
        if (!log_packet_send(client_fd, MSG_REJECTED, 0, NULL))
                log_warning("reject connection");

        close_socket(client_fd);
}

static void create_new_connection(struct settings *settings,
                                  const struct pollfd *entry_pollfd,
                                  struct pollfd *connection_pollfd)
{
        syslog(LOG_DEBUG, "establishing new connection");
        int client_fd = accept(entry_pollfd->fd, NULL, NULL);
        if (client_fd == -1) {
                log_warning("accept");
                return;
        }

        // max one active connection
        if (connection_pollfd->fd != -1) {
                reject_connection(client_fd);
                return;
        }

        settings->read_fd = client_fd;
        settings->write_fd = client_fd;
        connection_pollfd->fd = client_fd;
        send_settings(settings);
}

static enum utils_loop_status _entry_callback(struct settings *settings,
                                              const struct pollfd *entry_pollfd,
                                              struct pollfd *connection_pollfd)
{
        if (entry_pollfd->revents & POLLERR) {
                syslog(LOG_ERR, "error occurred on the entry socket");
                return UTILS_LOOP_ERROR;
        }

        if (entry_pollfd->revents & POLLIN) {
                create_new_connection(settings, entry_pollfd,
                                      connection_pollfd);
        }
        return UTILS_LOOP_CONTINUE;
}

static bool _not_in_process(struct server_file_info *file_info)
{
        return file_info->filefd < 0;
}

static enum utils_loop_status
_signal_callback(struct settings *settings, struct server_file_info *file_info,
                 size_t *packet_size_ptr, struct packet **packet_ptr,
                 const struct pollfd *signal_pollfd,
                 struct pollfd *connection_pollfd)
{
        if (!(signal_pollfd->revents & POLLIN))
                return UTILS_LOOP_CONTINUE;

        struct signalfd_siginfo info;
        if (utils_read(signal_pollfd->fd, sizeof(info), &info)
            != sizeof(info)) {
                log_error("read signal");
                return UTILS_LOOP_ERROR;
        }
        syslog(LOG_DEBUG, "caught signal '%d'", info.ssi_signo);

        if (info.ssi_signo == SIGPIPE) {
                close_connection(connection_pollfd);
                return UTILS_LOOP_CONTINUE;
        }

        syslog(LOG_DEBUG, "shutting down");
        if (shutdown(settings->sock_fd, SHUT_RDWR) == -1)
                log_warning("shutdown");

        bool success = true;
        if (connection_pollfd->fd != -1) {
                syslog(LOG_DEBUG, "ending open connection");
                success = read_packets_until(settings->read_fd, settings,
                                             file_info, packet_ptr,
                                             packet_size_ptr, _not_in_process);
                close_connection(connection_pollfd);
        }
        return success ? UTILS_LOOP_BREAK : UTILS_LOOP_ERROR;
}

static bool event_loop(struct settings *settings,
                       struct server_file_info *file_info)
{
        size_t packet_size = 0;
        struct packet *packet = NULL;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        EVENT_READER_CALLBACK(callback)
        {
                log_assert(n == 2);

                struct pollfd *entry_pollfd = &pollfds[0];
                struct pollfd *connection_pollfd = &pollfds[1];

                if (_entry_callback(settings, entry_pollfd, connection_pollfd)
                    == UTILS_LOOP_ERROR)
                        return UTILS_LOOP_ERROR;

                if (_connection_callback(settings, file_info, connection_pollfd,
                                         &packet_size, &packet)
                    != UTILS_LOOP_CONTINUE)
                        close_connection(connection_pollfd);

                return _signal_callback(settings, file_info, &packet_size,
                                        &packet, signal_pollfd,
                                        connection_pollfd);
        }
#pragma GCC diagnostic pop

        const struct pollfd fds[] = {
                { .fd = settings->sock_fd, .events = POLLIN },
                { settings->read_fd, .events = POLLIN | POLLRDHUP },
        };
        bool success = event_reader(&settings->mask, sizeof(fds) / sizeof(*fds),
                                    fds, callback);
        free(packet);

        return success;
}

/**
 * Main function of servers which calls other functions.
 *
 * @param settings Struct holding information about behaviour of the program
 * @return EXIT_SUCCESS if everything was successful;
 *         EXIT_FAILURE otherwise;
 */
int server_main(struct settings *settings)
{
        syslog(LOG_DEBUG, "Server call main");
        struct server_file_info file_info = { 0 };
        file_info.filefd = -1;

        if (!open_dir(settings, &file_info)
            || !get_filesystem_block_size(settings, &file_info)
            || !event_loop(settings, &file_info)) {
                syslog(LOG_DEBUG, "Server main EXIT_FAILURE.");
                return EXIT_FAILURE;
        }

        syslog(LOG_DEBUG, "Server main EXIT_SUCCESS.");
        return EXIT_SUCCESS;
}
