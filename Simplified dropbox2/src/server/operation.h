/**
 * @file operation.h
 * @brief Module managing processing of the packet from the client
 * @author Matej Kleman (xkleman@fi.muni.cz)
 * @date 2021-08-06
 */
#ifndef OPERATION_H
#define OPERATION_H

#include "settings.h"
#include "packet.h"
#include "file_info.h"

enum operation_status {
        OPERATION_NOK,
        OPERATION_OK,
        OPERATION_END,
};

/**
 * Processes command from client.
 *
 * @param settings   Struct holding information about behaviour of the program
 * @param file_info  Struct containing all
 *                   important information about current file
 * @param packet     Packet received from the client to be processed
 *
 * @return OPERATION_OK   on success;
 *         OPERATION_NOK  if one of the functions performing command failed;
 *         OPERATION_END  after MSG_END_CONNECTION was received;
 */
enum operation_status
server_operation_process_operation(const struct settings *settings,
                                   struct server_file_info *file_info,
                                   const struct packet *packet);

#endif //OPERATION_H
