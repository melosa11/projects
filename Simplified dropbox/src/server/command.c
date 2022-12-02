#include "command.h"
#include "extract.h"

#include "set.h"

#include "log.h"
#include "utils.h"

#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/**
 * Checks if we are either creating or modifying file right now.
 *
 * @param file_info
 * @return true if file is being modified;
 * @return false otherwise;
 */
static bool are_modifying_flags_set(const struct server_file_info *file_info)
{
        return file_info->creating_file || file_info->changing_file;
}

CMD(change_file)
{
        if (are_modifying_flags_set(data->file_info)) {
                syslog(LOG_ERR, "already modifying another file");
                return MSG_ABORT;
        }

        strncpy(data->file_info->change_name, (char *)data->packet->payload,
                NAME_MAX);
        if (faccessat(data->file_info->dirfd, data->file_info->change_name,
                      F_OK, 0)
            == -1) {
                syslog(LOG_WARNING, "file %s does not exist",
                       data->file_info->change_name);
                return MSG_NOK;
        }

        // Almighty Pazuzu forgive the race condition.
        if ((data->file_info->filefd
             = openat(data->file_info->dirfd, data->file_info->change_name,
                      O_RDWR))
            == -1) {
                syslog(LOG_ERR, "failed to get fd of %s, openat: %s",
                       data->file_info->change_name, strerror(errno));
                return MSG_ABORT;
        }

        syslog(LOG_DEBUG, "%s found and ready to be changed by next command",
               data->file_info->change_name);
        data->file_info->changing_file = true;
        return MSG_OK;
}

CMD(create_file)
{
        if (are_modifying_flags_set(data->file_info)) {
                syslog(LOG_ERR, "already modifying another file");
                return MSG_ABORT;
        }
        data->file_info->creating_file = true;
        data->file_info->file_name = (char *)data->packet->payload;
        const int flags = O_CREAT | O_WRONLY;
        if ((data->file_info->filefd
             = openat(data->file_info->dirfd, data->file_info->file_name, flags,
                      0666))
            == -1) {
                if (errno == EEXIST) {
                        syslog(LOG_WARNING, "cannot overwrite existing file");
                        return MSG_NOK;
                }
                log_error("openat");
                return MSG_ABORT;
        }
        syslog(LOG_DEBUG, "file %s was created successfully",
               data->file_info->file_name);
        return MSG_OK;
}

CMD(delete_file)
{
        const char *file_name = (char *)data->packet->payload;
        if (unlinkat(data->file_info->dirfd, file_name, 0) == -1) {
                log_error("unlinkat");
                return MSG_ABORT;
        }
        syslog(LOG_DEBUG, "file %s was deleted successfully", file_name);
        return MSG_OK;
}

static enum packet_msg_code
modify_metadata(struct server_command_data *data,
                enum packet_msg_code (*extract)(const unsigned char *payload,
                                                struct server_file_info *),
                enum packet_msg_code (*set)(const struct server_file_info *))
{
        if (!are_modifying_flags_set(data->file_info)) {
                syslog(LOG_ERR, "no file to modify");
                return MSG_ABORT;
        }

        enum packet_msg_code ret_code
                = extract(data->packet->payload, data->file_info);
        if (ret_code == MSG_ABORT)
                return MSG_ABORT;

        if (data->file_info->changing_file
            && ((ret_code = set(data->file_info)) == MSG_ABORT))
                return MSG_ABORT;

        data->file_info->changing_file = false;

        return ret_code;
}

CMD(set_timestamps)
{
        return modify_metadata(data, server_extract_time_info,
                               server_set_timestamps);
}

CMD(set_perm_modes)
{
        return modify_metadata(data, server_extract_permissions_info,
                               server_set_perm_modes);
}

CMD(set_owner)
{
        return modify_metadata(data, server_extract_owner_info,
                               server_set_owner);
}

CMD(write_block)
{
        if (!are_modifying_flags_set(data->file_info)) {
                syslog(LOG_ERR, "no file to modify");
                return MSG_ABORT;
        }

        // Got empty block of sparse file
        if (data->packet->payload_size == 0) {
                if (lseek(data->file_info->filefd,
                          data->settings->fs_block_size, SEEK_CUR)
                    == -1) {
                        log_error("lseek");
                        return MSG_ABORT;
                }
        } else if (utils_write(data->file_info->filefd,
                               data->packet->payload_size,
                               &data->packet->payload)
                   == -1) {
                log_error("write");
                return MSG_ABORT;
        }
        syslog(LOG_DEBUG, "block written successfully");
        return MSG_OK;
}
