#include "target.h"

#include "log.h"
#include "utils.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

enum directory_status {
        DIR_EMPTY,
        DIR_NON_EMPTY,
        DIR_ERROR,
};

static int create_target_lock(const char *lock_file_name)
{
        const int lock_fd
                = open(lock_file_name, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR);

        if (lock_fd == -1) {
                log_error(lock_file_name);
                return -1;
        }

        const struct flock lock = {
                .l_type = F_WRLCK,
                .l_whence = SEEK_SET,
                .l_start = 0,
                .l_len = 0,
        };

        if (fcntl(lock_fd, F_SETLK, &lock) == -1) {
                if (errno == EACCES || errno == EAGAIN) {
                        syslog(LOG_ERR,
                               "another instance of dropbox is running");
                } else {
                        log_error(lock_file_name);
                }
                return -1;
        }

        return lock_fd;
}

static bool lock_target_file(const char *base_name, const char *dir_name,
                             struct settings *settings)
{
        char *lock_file_name = NULL;
        if (asprintf(&lock_file_name, "%s/.%s.dropbox.lock", dir_name,
                     base_name)
            == -1) {
                syslog(LOG_ERR, "asprintf failed in lock file");
                return false;
        }

        const int lock_fd = create_target_lock(lock_file_name);
        free(lock_file_name);

        settings->lock_file_fd = lock_fd;
        return lock_fd >= 0;
}

bool main_target_lock_dir(const char *target_name, struct settings *settings)
{
        bool success = false;

        char *copydir = strdup(target_name);
        if (copydir == NULL) {
                log_error(target_name);
                return false;
        }

        char *copybase = strdup(target_name);
        if (copybase == NULL) {
                log_error(target_name);
                goto clean;
        }

        const char *dname = dirname(copydir);
        const char *bname = basename(copybase);

        success = lock_target_file(bname, dname, settings);
        free(copybase);
clean:
        free(copydir);
        return success;
}

static enum directory_status target_dir_is_empty(const char *target_name)
{
        DIR *dirp = opendir(target_name);
        if (dirp == NULL) {
                perror(target_name);
                return DIR_ERROR;
        }
        size_t n = 0;
        errno = 0;
        // clang-format off
        while (readdir(dirp) != NULL && n++ < 3) { /* Nop */ };
        // clang-format on
        assert(closedir(dirp) == 0);
        if (errno != 0) {
                perror(target_name);
                return DIR_ERROR;
        }

        return n == 2 ? DIR_EMPTY : DIR_NON_EMPTY;
}

bool main_target_prepare_dir(const char *target_name,
                             const struct settings *settings)
{
        if (mkdir(target_name, 0700) == -1) {
                if (errno != EEXIST) {
                        perror(target_name);
                        return false;
                }
        }

        const enum directory_status status = target_dir_is_empty(target_name);
        if (status == DIR_ERROR)
                return false;

        if (status == DIR_NON_EMPTY && !settings->force) {
                fprintf(stderr, "directory '%s' is not empty\n", target_name);
                return false;
        }

        return true;
}

bool main_target_write_pid_to_lock_file(const struct settings *settings)
{
        if (ftruncate(settings->lock_file_fd, 0) == -1) {
                log_error("truncate lock file");
                return false;
        }

        char *pid_str = NULL;
        if (asprintf(&pid_str, "%ld", (long)getpid()) == -1) {
                log_error("asprintf");
                return false;
        }

        bool success = false;
        if (utils_write(settings->lock_file_fd, strlen(pid_str), pid_str)
            == -1) {
                log_error("write pid to lock file");
                goto clean;
        }

        success = true;
clean:
        free(pid_str);
        return success;
}
