/**
 * @file extract.h
 * Functions to extract data from payloads and store them
 * into file_info structure.
 *
 * @author Matej Kleman (xkleman@fi.muni.cz)
 * @date 2021-08-06
 */
#ifndef EXTRACT_H
#define EXTRACT_H

#include "packet.h"

#include "file_info.h"

/**
 * Saves info about timestamps from the packet.
 *
 * @return MSG_OK;
 */
enum packet_msg_code
server_extract_time_info(const unsigned char payload[],
                         struct server_file_info *file_info);

/**
 * Saves info about permissions of the file from the packet.
 *
 * @return MSG_OK;
 */
enum packet_msg_code
server_extract_permissions_info(const unsigned char payload[],
                                struct server_file_info *file_info);

/**
 * Saves info about owner of the file from the packet.
 *
 * @return MSG_OK;
 */
enum packet_msg_code
server_extract_owner_info(const unsigned char payload[],
                          struct server_file_info *file_info);

#endif /* EXTRACT_H */
