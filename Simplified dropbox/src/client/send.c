#include "send.h"

#include "helper.h"

#include "utils.h"
#include "log.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

/**
 * Checks if all bytes are null bytes.
 *
 * @param buff  pointer to a memory, which is checked
 * @param size  size of this memory
 * @return      true if all bytes are null bytes;
 *              false if there is one or more not null bytes
 */
static bool check_null_bytes(const unsigned char *buff, ssize_t size)
{
        for (ssize_t i = 0; i < size; i++) {
                if (buff[i] != 0)
                        return false;
        }
        return true;
}

int client_send_owner(const struct client_traverse_data *data)
{
        const struct packet_payload_set_owner payload = {
                .uid = data->sb->st_uid,
                .gid = data->sb->st_gid,
        };

        syslog(LOG_DEBUG, "MSG_SET_OWNER: uid: %d, gid: %d", payload.uid,
               payload.gid);

        if (!log_packet_send(data->settings->write_fd, MSG_SET_OWNER,
                             sizeof(payload), (const unsigned char *)&payload))
                return FTW_STOP;

        const int answer = client_helper_get_answer(data);

        if (!client_helper_check_expected_code(answer, MSG_OK))
                return FTW_STOP;

        syslog(LOG_DEBUG, "send owner success");

        return NEXT_STEP;
}

int client_send_permission_modes(const struct client_traverse_data *data)
{
        struct packet_payload_set_perm_modes payload = {
                .mode = data->sb->st_mode,
        };
        syslog(LOG_DEBUG, "MSG_SET_PERM_MODES: %o", payload.mode);

        if (!log_packet_send(data->settings->write_fd, MSG_SET_PERM_MODES,
                             sizeof(payload), (const unsigned char *)&payload))
                return FTW_STOP;
        int answer = client_helper_get_answer(data);

        if (!client_helper_check_expected_code(answer, MSG_OK))
                return FTW_STOP;

        syslog(LOG_DEBUG, "send permission modes success");
        return NEXT_STEP;
}

int client_send_timestamps(const struct client_traverse_data *data)
{
        syslog(LOG_DEBUG, "client_send_timestamps started");
        const struct packet_payload_timestamps payload = {
                .atim = data->sb->st_atim,
                .mtim = data->sb->st_mtim,
        };
        syslog(LOG_DEBUG,
               "MSG_SET_TIMESTAMPS: st_atim.tv_sec: %ld, st_atim.tv_nsec: %ld"
               "; st_mtim.tv_sec: %ld, st_mtim.tv_nsec: %ld",
               payload.atim.tv_sec, payload.atim.tv_nsec, payload.mtim.tv_sec,
               payload.mtim.tv_nsec);

        if (!log_packet_send(data->settings->write_fd, MSG_SET_TIMESTAMPS,
                             sizeof(payload), (const unsigned char *)&payload))
                return FTW_STOP;

        const int answer = client_helper_get_answer(data);
        if (answer == MSG_NOK)
                return FTW_CONTINUE;

        if (!client_helper_check_expected_code(answer, MSG_OK))
                return FTW_STOP;

        syslog(LOG_DEBUG, "send timestamps success");
        return NEXT_STEP;
}

int client_send_delete_file(const struct client_traverse_data *data)
{
        const char *payload = data->fpath;
        syslog(LOG_DEBUG, "client_send_delete payload: %s", payload);
        if (!log_packet_send(data->settings->write_fd, MSG_DELETE_FILE,
                             strlen(payload) + 1,
                             (const unsigned char *)payload))
                return FTW_STOP;
        const int answer = client_helper_get_answer(data);
        if (answer == MSG_NOK) {
                syslog(LOG_DEBUG, "got MSG_NOK");
                return FTW_CONTINUE;
        }
        if (!client_helper_check_expected_code(answer, MSG_OK))
                return FTW_STOP;

        syslog(LOG_DEBUG, "send delete file successful");
        return NEXT_STEP;
}

int client_send_create_file(const struct client_traverse_data *data,
                            const char *file_path)
{
        syslog(LOG_DEBUG, "MSG_CREATE_FILE: %s", file_path);
        if (!log_packet_send(data->settings->write_fd, MSG_CREATE_FILE,
                             strlen(file_path) + 1,
                             (const unsigned char *)file_path))
                return FTW_STOP;

        const int answer = client_helper_get_answer(data);
        if (answer == MSG_NOK)
                return FTW_CONTINUE;

        if (!client_helper_check_expected_code(answer, MSG_OK))
                return FTW_STOP;

        syslog(LOG_DEBUG, "create file success");
        return NEXT_STEP;
}

static bool sending(const struct client_traverse_data *data, int fd,
                    unsigned char *read_buff)
{
        ssize_t size;
        syslog(LOG_DEBUG, "start reading blocks from %s", data->fpath);
        while ((size = utils_read(fd, data->settings->fs_block_size, read_buff))
               > 0) {
                syslog(LOG_DEBUG, "MSG_WRITE_BLOCK: %ld", size);

                if (data->settings->sparse && check_null_bytes(read_buff, size))
                        size = 0;

                if (!log_packet_send(data->settings->write_fd, MSG_WRITE_BLOCK,
                                     size, (const unsigned char *)read_buff))
                        return false;

                const int answer = client_helper_get_answer(data);
                if (!client_helper_check_expected_code(answer, MSG_OK))
                        return false;
        }
        if (size == -1)
                log_error("utils_read");

        syslog(LOG_DEBUG, "send blocks success");
        return true;
}

int client_send_blocks(const struct client_traverse_data *data)
{
        syslog(LOG_DEBUG, "opening %s for reading", data->fpath);
        const int fd = openat(data->settings->dirfd, data->fpath, O_RDONLY);
        if (fd == -1) {
                log_error("openat");
                return NEXT_STEP;
        }
        log_assert(data->settings->fs_block_size > 0);

        int result = FTW_STOP;
        unsigned char *read_buff = malloc(data->settings->fs_block_size);
        if (read_buff == NULL) {
                log_error("malloc");
                goto clean_fd;
        }
        if (!sending(data, fd, read_buff))
                goto clean;

        result = NEXT_STEP;
clean:
        free(read_buff);
clean_fd:
        if (close(fd) == -1)
                syslog(LOG_ERR, "close: %s", data->fpath);

        return result;
}

/**
 * Sends done message to a server.
 *
 * @param data  struct holding data, which are used when traversing a folder
 * @return      NEXT_STEP on success;
 *              FTW_STOP on failure
 */
int client_send_done(const struct client_traverse_data *data)
{
        syslog(LOG_DEBUG, "MSG_DONE");
        if (!log_packet_send(data->settings->write_fd, MSG_DONE, 0, NULL))
                return FTW_STOP;

        syslog(LOG_DEBUG, "send done success");
        return NEXT_STEP;
}
