#include "setup.h"

#include "target.h"

#include "log.h"

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

static int open_socket(const struct addrinfo *info)
{
        int fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (fd == -1)
                log_warning("socket");
        return fd;
}

static void close_socket(int sock_fd)
{
        if (close(sock_fd) == -1)
                log_warning("close");
}

static int get_socket(const struct addrinfo *list,
                      bool (*setup)(int, const struct addrinfo *))
{
        for (const struct addrinfo *e = list; e != NULL; e = e->ai_next) {
                int sock_fd = open_socket(e);
                if (sock_fd == -1)
                        continue;

                if (setup(sock_fd, e))
                        return sock_fd;
                close_socket(sock_fd);
        }
        return -1;
}

static bool setup_connected_socket(int sock_fd, const struct addrinfo *info)
{
        if (connect(sock_fd, info->ai_addr, info->ai_addrlen) == -1) {
                log_warning("connect");
                return false;
        }
        return true;
}

static int get_connected_socket(const struct addrinfo *list)
{
        int sock_fd = get_socket(list, setup_connected_socket);
        if (sock_fd == -1)
                syslog(LOG_ERR, "cannot establish connection to the server");
        return sock_fd;
}

static bool open_client_connection(const char *host, struct settings *settings)
{
        const struct addrinfo hints = {
                .ai_family = AF_INET,
                .ai_socktype = SOCK_STREAM,
                .ai_protocol = 0,
                .ai_flags = AI_NUMERICSERV,
        };

        struct addrinfo *result = NULL;
        int ret = getaddrinfo(host, settings->port, &hints, &result);
        if (ret != 0) {
                syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(ret));
                return false;
        }

        settings->sock_fd = get_connected_socket(result);
        settings->write_fd = settings->sock_fd;
        settings->read_fd = settings->sock_fd;
        freeaddrinfo(result);

        return settings->sock_fd != -1;
}

static bool setup_listening_socket(int sock_fd, const struct addrinfo *info)
{
#ifdef DROPBOX_TESTS
        // Without this the tests fail because the source:port
        // will be still used from the previous tests.
        if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 },
                       sizeof(int))
            == -1) {
                log_warning("setsockopt");
                return false;
        }
#endif
        if (bind(sock_fd, info->ai_addr, info->ai_addrlen) == -1) {
                log_warning("bind");
                return false;
        }

        if (listen(sock_fd, 1) == -1) {
                log_warning("listen");
                return false;
        }
        return true;
}

static int get_listening_socket(const struct addrinfo *list)
{
        int sock_fd = get_socket(list, setup_listening_socket);
        if (sock_fd == -1)
                syslog(LOG_ERR, "cannot listen on the port");
        return sock_fd;
}

static bool open_server_connection(struct settings *settings)
{
        const struct addrinfo hints = {
                .ai_family = AF_INET,
                .ai_socktype = SOCK_STREAM,
                .ai_protocol = 0,
                .ai_flags = AI_NUMERICSERV | AI_PASSIVE,
        };

        struct addrinfo *result = NULL;
        int ret = getaddrinfo(NULL, settings->port, &hints, &result);
        if (ret != 0) {
                syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(ret));
                return false;
        }

        settings->sock_fd = get_listening_socket(result);

        freeaddrinfo(result);

        return settings->sock_fd != -1;
}

MAIN_SETUP(client)
{
        if (!open_client_connection(args[0], settings))
                return false;
        settings->cwd = args[1];

        return true;
}

MAIN_SETUP(server)
{
        const char *target_name = args[0];
        if (!main_target_prepare_dir(target_name, settings))
                return false;
        // need to lock after daemonization because daemon(2)
        // implementation would not close the lock file after
        // forking
        if (!main_target_lock_dir(target_name, settings))
                return false;
        if (!main_target_write_pid_to_lock_file(settings))
                return false;

        if (!open_server_connection(settings))
                return false;

        settings->cwd = target_name;
        return true;
}
