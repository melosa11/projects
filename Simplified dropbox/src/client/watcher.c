#include "watcher.h"

#include "traverse_data.h"

#include "log.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>

static bool _watcher_create(struct client_watcher *watcher, int wd,
                            const char *name)
{
        const char *copy_name = strdup(name);
        if (copy_name == NULL)
                return false;

        watcher->wd = wd;
        watcher->name = copy_name;
        return true;
}

/**
 * Adds watcher for given file/directory to inotify
 *
 * @param fd File descriptor
 * @param obj_name Name of the file/directory
 * @param inot_fd Inotify object file descriptor
 * @param mask Mask representing watched events
 * @return true if adding succeeded;
 * @return false otherwise;
 */
static bool add_object_watch(int *wd, const char *obj_name, int inot_fd,
                             uint32_t mask)
{
        log_assert(mask != 0);
        int new_wd;
        if ((new_wd = inotify_add_watch(inot_fd, obj_name, mask)) == -1) {
                log_error("add_object_watch");
                return false;
        }
        *wd = new_wd;
        return true;
}

/**
 * Adds watcher for given file to inotify
 *
 * @param fd File descriptor
 * @param file_name Name of the file
 * @param inot_fd Inotify object file descriptor
 * @return true if adding succeeded;
 * @return false otherwise;
 */
static bool watch_file(int *wd, const char *file_name, int inot_fd)
{
        uint32_t mask = IN_CLOSE_WRITE | IN_ATTRIB;
        return add_object_watch(wd, file_name, inot_fd, mask);
}

/**
 * Adds watcher for given directory to inotify
 *
 * @param fd File descriptor
 * @param file_name Name of the directory
 * @param inot_fd Inotify object file descriptor
 * @return true if adding succeeded;
 * @return false otherwise;
 */
static bool watch_dir(int *wd, const char *dir_name, int inot_fd)
{
        uint32_t mask = IN_CREATE | IN_DELETE | IN_MOVE;
        return add_object_watch(wd, dir_name, inot_fd, mask);
}

/**
 * Identifies object type and calls appropriate watcher adder
 *
 * @param fd File descriptor
 * @param obj_name Name of the file/directory
 * @param inot_fd Inotify object file descriptor
 * @param st Stat struct
 * @return true
 * @return false
 */
static bool watch_object(int *wd, const char *obj_name, int inot_fd)
{
        struct stat st;
        if (stat(obj_name, &st) == -1) {
                log_error("watch_object");
                return false;
        }
        if (S_ISDIR(st.st_mode))
                return watch_dir(wd, obj_name, inot_fd);
        return watch_file(wd, obj_name, inot_fd);
}

bool watcher_create(struct client_watcher *watcher,
                    const struct client_watcher_data *watcher_data)
{
        syslog(LOG_DEBUG, "create_watcher started");
        int wd;
        if (!watch_object(&wd, watcher_data->fpath, watcher_data->inot_fd)) {
                syslog(LOG_DEBUG, "watch_object failed.");
                return false;
        }

        return _watcher_create(watcher, wd, watcher_data->relative_path);
}
