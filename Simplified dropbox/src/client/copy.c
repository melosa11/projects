#include "copy.h"

#include "helper.h"
#include "log.h"
#include "traverse_data.h"
#include "send.h"
#include "watcher.h"
#include "watcher_list.h"

/**
 * Converts absolute path to a relative path.
 *
 * @param fpath   absolute path
 * @param ftwbuf  struct holding information about base and level
 * @return        relative path
 */
static const char *get_relative_path(const char *fpath,
                                     const struct FTW *ftwbuf)
{
        if (ftwbuf == NULL)
                return fpath;

        const char *rel_path = fpath + ftwbuf->base;
        int counter = ftwbuf->level;
        while (true) {
                rel_path--;
                if (*rel_path == '/')
                        counter--;

                if (counter == 0)
                        break;
        }
        return rel_path + 1;
}
/**
 * Gets answer from a server and logs it.
 *
 * @param op    operation which was performed.
 * @param data  struct holding data, which are used when traversing a folder
 * @return      NEXT_STEP on success;
 *              FTW_STOP on failure
 */
static int get_op_response(const char *op,
                           const struct client_traverse_data *data)
{
        const enum packet_msg_code answer = client_helper_get_answer(data);
        if (answer == MSG_NOK) {
                syslog(LOG_ERR, "set %s: %s", op, strerror(EPERM));
                return NEXT_STEP;
        }
        if (!client_helper_check_expected_code(answer, MSG_OK))
                return FTW_STOP;
        syslog(LOG_DEBUG, "set %s success", op);
        return NEXT_STEP;
}

int client_copy_regular_file(struct client_traverse_data data)
{
        const char *path_for_server
                = get_relative_path(data.fpath, data.ftwbuf);

        syslog(LOG_INFO, "sending file: %s", path_for_server);

        data.fpath = path_for_server;

/**
 * Macro which decides within traversal if move to next step,
 * skip one iteration or stop.
 */
#define DO_STEP(RESULT)                                                        \
        do {                                                                   \
                if ((result = (RESULT)) != NEXT_STEP)                          \
                        goto end;                                              \
        } while (0)

        int result;
        DO_STEP(client_send_create_file(&data, path_for_server));
        DO_STEP(client_send_timestamps(&data));
        DO_STEP(client_send_permission_modes(&data));
        DO_STEP(client_send_owner(&data));
        DO_STEP(client_send_blocks(&data));
        DO_STEP(client_send_done(&data));

        const char *operations[]
                = { "timestamps", "permissions", "owner", NULL };
        for (size_t i = 0; operations[i] != NULL; i++) {
                const char *op = operations[i];
                DO_STEP(get_op_response(op, &data));
        }
        result = FTW_CONTINUE;
end:
        syslog(LOG_DEBUG, "copying of file ended with code %d", result);
        return result;
}

static bool
create_and_add_watcher(const struct client_watcher_data *watcher_data,
                       client_watcher_list *watchers)
{
        struct client_watcher watcher;

        return watcher_create(&watcher, watcher_data)
               && client_watcher_list_push_back(watchers, &watcher);
}

static int _traversal(const char *fpath, const struct stat *sb, int tflag,
                      const struct FTW *ftwbuf, const struct settings *settings,
                      struct packet **packet_buffptr, size_t *nptr, int inot_fd,
                      client_watcher_list *watchers)
{
        syslog(LOG_DEBUG, "traversal: fpath: %s, type: %d", fpath, tflag);

        if (ftwbuf->level == 0)
                return FTW_CONTINUE;

        if (ftwbuf->level > 1)
                return FTW_STOP;

        const struct client_traverse_data data = {
                .settings = settings,
                .packet_buffptr = packet_buffptr,
                .nptr = nptr,
                .fpath = fpath,
                .sb = (struct stat *)sb,
                .ftwbuf = ftwbuf,
        };

        switch (tflag) {
        case FTW_F:
                if (!settings->one_shot) {
                        struct client_watcher_data watcher_data = {
                                .inot_fd = inot_fd,
                                .fpath = fpath,
                                .relative_path
                                = get_relative_path(data.fpath, data.ftwbuf),
                        };
                        if (!create_and_add_watcher(&watcher_data, watchers)) {
                                syslog(LOG_DEBUG,
                                       "create_watcher failed in traversal");
                                return FTW_STOP;
                        }
                }
                return client_copy_regular_file(data);
        case FTW_D:
                return FTW_CONTINUE;
        case FTW_DNR:
                syslog(LOG_WARNING, "Could read folder: %s", fpath);
        default:
                break;
        }
        return FTW_CONTINUE;
}

bool client_copy_files(int inot_fd, client_watcher_list *watchers,
                       const struct settings *settings)
{
        syslog(LOG_DEBUG, "starting to copy files");

        if (!settings->one_shot) {
                struct client_watcher_data watcher_data = {
                        .fpath = settings->cwd,
                        .inot_fd = inot_fd,
                        .relative_path = settings->cwd,
                };
                if (!create_and_add_watcher(&watcher_data, watchers)) {
                        syslog(LOG_DEBUG, "create_watcher failed");
                        return false;
                }
        }

        struct packet *packet_buff = NULL;
        size_t n = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        /* Nested function. GNU extension */
        int traversal(const char *fpath, const struct stat *sb, int tflag,
                      struct FTW *ftwbuf)
        {
                return _traversal(fpath, sb, tflag, ftwbuf, settings,
                                  &packet_buff, &n, inot_fd, watchers);
        }
#pragma GCC diagnostic pop

        bool is_success = false;
        if (nftw(settings->cwd, traversal, 64, FTW_ACTIONRETVAL | FTW_PHYS)
            == -1)
                goto clean;

        syslog(LOG_DEBUG, "copying finished");
        is_success = true;
clean:
        free(packet_buff);
        return is_success;
}
