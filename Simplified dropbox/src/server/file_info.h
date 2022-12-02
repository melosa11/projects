/**
 * @file server.h
 * @brief Server interface for processing clients requests.
 * @author Matej Kleman (xkleman@fi.muni.cz)
 * @date 2021-01-01
 */

#ifndef FILE_INFO_H
#define FILE_INFO_H

#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

struct server_file_info {
        char *file_name;      /**< name of current file                     */
        int filefd;           /**< file descriptor of current file          */
        int dirfd;            /**< file descriptor of working directory     */
        struct timespec atim; /**< access time of current file              */
        struct timespec mtim; /**< modification time of current file        */
        uid_t uid;            /**< user ID of file owner                    */
        gid_t gid;            /**< group ID of file owner                   */
        mode_t mode;          /**< permissions of current file              */
        bool creating_file;   /**< flag signaling file is being created     */
        bool changing_file;   /**< next command(s) modifies or rewrite file */
        char change_name[NAME_MAX + 1]; /**< name of file to be changed     */
};

#endif //FILE_INFO_H
