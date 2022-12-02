#include "extract.h"

#include "operation.h"

#include "log.h"

enum packet_msg_code
server_extract_time_info(const unsigned char payload[],
                         struct server_file_info *file_info)
{
        const struct packet_payload_timestamps *timestamps
                = (struct packet_payload_timestamps *)payload;

        file_info->atim = timestamps->atim;
        file_info->mtim = timestamps->mtim;
        syslog(LOG_DEBUG, "Timestamps received successfully.");
        return MSG_OK;
}

enum packet_msg_code
server_extract_permissions_info(const unsigned char payload[],
                                struct server_file_info *file_info)
{
        const struct packet_payload_set_perm_modes *modes
                = (struct packet_payload_set_perm_modes *)payload;
        file_info->mode = modes->mode;
        syslog(LOG_DEBUG, "Permissions received successfully.");
        return MSG_OK;
}

enum packet_msg_code
server_extract_owner_info(const unsigned char payload[],
                          struct server_file_info *file_info)
{
        struct packet_payload_set_owner *owner
                = (struct packet_payload_set_owner *)payload;
        file_info->uid = owner->uid;
        file_info->gid = owner->gid;
        syslog(LOG_DEBUG, "Owner uid and gid received successfully.");
        return MSG_OK;
}
