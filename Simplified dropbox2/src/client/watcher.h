/**
 * @file watcher.h
 * @brief Implementaion of struction for holding intotify watch descriptor
 *        and the relative path of the corresponding file/folder
 * @author Peter Mercell
 * @date 2021-08-05
 */
#ifndef WATCHER_H
#define WATCHER_H

#include <stdbool.h>

struct client_watcher {
        int wd;           /**< inotify watch descriptor         */
        const char *name; /**< relative path of the file/folder */
};

/**
 * @brief Data for constructing creating inotify watch
 */
struct client_watcher_data {
        const char *fpath;
        const char *relative_path;
        int inot_fd;
};

/**
 * @brief Creates watcher from the watcher data
 *
 * @param watcher       pointer to an uninitialized watcher object
 * @param watcher_data  a pointer to data
 *
 * @return true on success;
 *         false otherwise
 */
bool watcher_create(struct client_watcher *watcher,
                    const struct client_watcher_data *watcher_data);

#endif //WATCHER_H
