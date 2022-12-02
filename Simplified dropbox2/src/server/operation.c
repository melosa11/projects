/**
 * Server operations.
 *
 * @file operation.c
 * @author Matej Kleman (xkleman@fi.muni.cz)
 * @date 2021-01-01
 */
#include "operation.h"

#include "command.h"
#include "log.h"
#include "set.h"

#include "utils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * Sends code to client about the result of operation.
 *
 * @param settings Struct holding information about behaviour of the program
 * @param code Information for client about requested operation result
 * @return true on success;
 *         false if invalid message_code was passed to function
 *         or packet_send failed;
 */
static bool send_operation_result(const struct settings *settings,
                                  enum packet_msg_code code)
{
        struct packet_payload_abort *payload = NULL;
        size_t payload_size = 0;
        struct packet_payload_abort abort_info;

        if (code == MSG_ABORT) {
                abort_info.error_number = errno;
                payload_size = sizeof(struct packet_payload_abort);
                payload = &abort_info;
        }

        log_assert(code >= MSG_OK && code <= MSG_ABORT);

        return log_packet_send(settings->write_fd, code, payload_size,
                               (unsigned char *)payload);
}

static void close_file(struct server_file_info *file_info)
{
        if (close(file_info->filefd) == -1) {
                log_warning("close");
        }
        file_info->filefd = -1;
        syslog(LOG_DEBUG, "File closed successfully.");
}

static enum operation_status done(const struct settings *settings,
                                  struct server_file_info *file_info)
{
        if (file_info->changing_file) {
                file_info->changing_file = false;
                goto end;
        }

        file_info->creating_file = false;
        if (!send_operation_result(settings, server_set_timestamps(file_info))
            || !send_operation_result(settings,
                                      server_set_perm_modes(file_info))
            || !send_operation_result(settings, server_set_owner(file_info)))
                return OPERATION_NOK;

end:
        close_file(file_info);
        syslog(LOG_DEBUG, "Current file finished.");
        return OPERATION_OK;
}

static enum operation_status run_command(server_command_t cmd,
                                         struct server_command_data *data)
{
        enum operation_status result = OPERATION_OK;
        enum packet_msg_code code = cmd(data);

        if (code == MSG_ABORT)
                result = OPERATION_NOK;

        if (!send_operation_result(data->settings, code))
                result = OPERATION_NOK;

        return result;
}

enum operation_status
server_operation_process_operation(const struct settings *settings,
                                   struct server_file_info *file_info,
                                   const struct packet *packet)
{
        const struct {
                enum packet_msg_code code;
                server_command_t cmd;
        } CODE_COMMAND_MAP[] = {
                { MSG_CREATE_FILE, server_command_create_file },
                { MSG_DELETE_FILE, server_command_delete_file },
                { MSG_CHANGE_FILE, server_command_change_file },
                { MSG_SET_TIMESTAMPS, server_command_set_timestamps },
                { MSG_SET_PERM_MODES, server_command_set_perm_modes },
                { MSG_SET_OWNER, server_command_set_owner },
                { MSG_WRITE_BLOCK, server_command_write_block },
                { .cmd = NULL },
        };

        if (packet->code == MSG_END_CONNECTION)
                return OPERATION_END;

        if (packet->code == MSG_DONE)
                return done(settings, file_info);

        enum operation_status result = OPERATION_NOK;
        for (size_t i = 0; CODE_COMMAND_MAP[i].cmd != NULL; i++) {
                if (packet->code == CODE_COMMAND_MAP[i].code) {
                        struct server_command_data data = {
                                .file_info = file_info,
                                .packet = packet,
                                .settings = settings,
                        };
                        result = run_command(CODE_COMMAND_MAP[i].cmd, &data);
                }
        }

        if (result == OPERATION_NOK)
                syslog(LOG_ERR, "invalid command received.");
        return result;
}
