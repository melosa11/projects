/**
 * Definition of helping function.
 *
 * @file test_helper.c
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2021-01-08
 */

#define CUT
#include "test_helper.h"

#include "packet.h"
#include "server.h"
#include "settings.h"
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cut.h"

const char *FOLDER_PREFIX = "test_";

static void prepare_connection(int *readfd, int *writefd,
                               struct settings *settings)
{
        int fds[2];
        ASSERT(socketpair(AF_UNIX, SOCK_STREAM, 0, &fds[0]) == 0);
        *readfd = fds[0];
        *writefd = fds[0];
        settings->write_fd = fds[1];
        settings->read_fd = fds[1];
}

static char *create_test_dir_name(const char *name)
{
        const char *prefix = FOLDER_PREFIX;
        size_t prefix_len = strlen(prefix);
        size_t name_len = strlen(name);
        char *dir_name = malloc(prefix_len + name_len + 1);
        ASSERT(dir_name != NULL);
        stpncpy(stpncpy(dir_name, prefix, prefix_len), name, name_len + 1);
        return dir_name;
}

static void remove_dir(const char *dir_name)
{
        char *command = NULL;
        ASSERT(asprintf(&command, "rm -rf %s", dir_name) != -1);
        ASSERT(system(command) == 0);
        free(command);
}

static int make_test_dir(const char *dir_name)
{
        DIR *dir = opendir(dir_name);
        if (dir != NULL) {
                ASSERT(closedir(dir) == 0);
                remove_dir(dir_name);
        }
        ASSERT(mkdir(dir_name, 0777) == 0);
        int dirfd = open(dir_name, O_DIRECTORY);
        ASSERT(dirfd != -1);
        return dirfd;
}

int prepare_test(const char *name, int *readfd, int *writefd,
                 struct settings *settings)
{
        memset(settings, 0, sizeof(*settings));
        char *dir_name = create_test_dir_name(name);
        settings->cwd = dir_name;
        int dirfd = make_test_dir(dir_name);
        settings->dirfd = dirfd;
        prepare_connection(readfd, writefd, settings);
        return dirfd;
}

void clean_after_test(struct settings *settings, struct packet *pack)
{
        free(pack);
        free((void *)settings->cwd);
}

static void *server_task(void *data)
{
        struct settings *settings = data;

        sigset_t set;
        ASSERT(sigemptyset(&set) == 0);
        ASSERT(sigaddset(&set, SIGTERM) == 0);
        ASSERT(pthread_sigmask(SIG_BLOCK, &set, NULL) == 0);
        settings->mask = set;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
        return (void *)server_main(settings);
}
#pragma GCC diagnostic pop

void start_server(pthread_t *th, struct settings *settings)
{
        ASSERT(pthread_create(th, NULL, server_task, settings) == 0);
}

void stop_server(pthread_t th)
{
        ASSERT(pthread_kill(th, SIGTERM) == 0);
}

void wait_server(pthread_t th, enum server_status st)
{
        void *ret_val = NULL;
        ASSERT(pthread_join(th, &ret_val) == 0);

        switch (st) {
        case SERVER_OK:
        case SERVER_ERROR:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
                ASSERT((int)ret_val == (int)st);
                break;
        case SERVER_STOPPED:
                if (ret_val != PTHREAD_CANCELED) {
                        DEBUG_MSG("%d", (int)ret_val);
                }
#pragma GCC diagnostic pop
                ASSERT(ret_val == PTHREAD_CANCELED);
                break;
        default:
                ASSERT(false);
        }
}
