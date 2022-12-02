/**
 * @file settings.h
 * @brief API containing program settings for client and server
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2020-12-28
 */
#ifndef SETTINGS_H
#define SETTINGS_H

#include <signal.h>
#include <stdbool.h>

/**
 * Struct holding information about behaviour of the program.
 */
struct settings {
        int lock_file_fd;            /**< file lock */
        int read_fd;                 /**< read end of byte stream */
        int write_fd;                /**< write end of byte stream */
        const char *cwd;             /**< path to working directory */
        int dirfd;                   /**< file descriptor of the cwd */
        unsigned long fs_block_size; /**< size of a block on file system */
        sigset_t mask;               /**< mask of blocked signals */
        int sock_fd;                 /**< underlying socket for communication*/
        /* command line flags */
        bool verbose;
        bool sparse;
        bool foreground;
        bool one_shot;
        bool force;
        bool debug;
        bool quiet;
        bool client;
        bool server;
        const char *port;
};

#endif //SETTINGS_H
