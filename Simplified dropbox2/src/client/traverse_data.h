/**
 * @file traverse_data.h
 * @brief Defintion of structure to hold information while traversing the
 *        SOURCE folder.
 * @author Peter Mercell
 * @date 2021-08-05
 */
#ifndef TRAVERSE_DATA_H
#define TRAVERSE_DATA_H

#include "settings.h"
#include "packet.h"

#include <errno.h>
#include <ftw.h>
#include <string.h>
#include <syslog.h>

struct client_traverse_data {
        const struct settings *settings;
        struct packet **packet_buffptr;
        size_t *nptr;
        const char *fpath; /**< path of the file */
        struct stat *sb;
        const struct FTW *ftwbuf;
        int fd;
};

#endif //TRAVERSE_DATA_H
