/**
 * @file set.h
 * @brief Functions for chaning files in server folder
 * @author Matej Kleman (xkleman@fi.muni.cz)
 * @date 2021-08-06
 */
#ifndef SET_H
#define SET_H

#include "packet.h"
#include "settings.h"
#include "file_info.h"

/**
 * Sets access and modification time of a file.
 *
 * @param file_info Struct containing all
 *                  important information about current file
 *
 * @return MSG_OK on success;
 *         MSG_NOK if futimes failed;
 */
enum packet_msg_code
server_set_timestamps(const struct server_file_info *file_info);

/**
 * Sets permissions on file.
 *
 * @param file_info Struct containing all
 *                  important information about current file
 *
 * @return MSG_OK    on success;
 *         MSG_NOK   if don't have permissions;
 *         MSG_ABORT if fchmod failed;
 */
enum packet_msg_code
server_set_perm_modes(const struct server_file_info *file_info);

/**
 * Sets uid and gid of a file.
 *
 * @param file_info Struct containing all
 *                  important information about current file
 *
 * @return MSG_OK on success;
 *         MSG_NOK if fchown failed;
 */
enum packet_msg_code server_set_owner(const struct server_file_info *file_info);

#endif /* SET_H */
