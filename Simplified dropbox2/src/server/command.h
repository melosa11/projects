/**
 * @file command.h
 * @brief Subroutines corresponding to protocol codes.
 * @author Matej Kleman (xkleman@fi.muni.cz)
 * @date 2021-08-06
 */
#ifndef COMMAND_H
#define COMMAND_H

#include "packet.h"
#include "settings.h"
#include "file_info.h"

struct server_command_data {
        const struct settings *settings;
        const struct packet *packet;
        struct server_file_info *file_info;
};

typedef enum packet_msg_code (*server_command_t)(struct server_command_data *);

/**
 * @brief Runs a command and returns the code according to protocol.
 */
#define CMD(NAME)                                                              \
        enum packet_msg_code server_command_##NAME(                            \
                struct server_command_data *data)

CMD(create_file);
CMD(change_file);
CMD(delete_file);

CMD(set_timestamps);
CMD(set_perm_modes);
CMD(set_owner);

CMD(write_block);
#endif /* COMMAND_H */
