#include "copy.h"
#include "event.h"
#include "helper.h"
#include "send.h"
#include "watcher_list.h"

#include "log.h"

#include <sys/inotify.h>

/**
 * Gets the block_size from server and stores it into settings
 *
 * @param packet_buff   an address of the pointer of type struct packet *
 * @param settings      struct holding information about behaviour of the program.
 */
static void set_block_size(struct packet *packet_buff,
                           struct settings *settings)
{
        struct packet_payload_settings *size
                = (struct packet_payload_settings *)packet_buff->payload;
        settings->fs_block_size = size->fs_block_size;

        syslog(LOG_DEBUG, "set block size: %lu", settings->fs_block_size);
}

/**
 * Gets settings from server
 *
 * @param settings  struct holding information about behaviour of the program.
 * @return          true on success;
 *                  false on failure
 */
static bool get_settings_from_server(struct settings *settings)
{
        struct packet *packet_buff = NULL;
        size_t n = 0;

        bool is_success = false;
        if (!log_packet_read(settings->read_fd, &packet_buff, &n))
                goto clean;

        if (packet_buff->code == MSG_REJECTED) {
                syslog(LOG_ERR, "rejected by server");
                goto clean;
        }

        if (!client_helper_check_expected_code(packet_buff->code, MSG_SETTINGS))
                goto clean;
        syslog(LOG_DEBUG, "received settings from server");

        set_block_size(packet_buff, settings);

        is_success = true;
clean:
        free(packet_buff);
        return is_success;
}

/**
 * Main function of clients which calls other functions.
 *
 * @param settings  struct holding information about behaviour of the program.
 * @return          EXIT_SUCCESS if everything was successful;
 *                  EXIT_FAILURE otherwise
 */
int client_main(struct settings *settings)
{
        syslog(LOG_DEBUG, "call main");

        const int inot_fd = inotify_init1(IN_NONBLOCK);
        if (inot_fd == -1) {
                log_error("inotify_init1");
                syslog(LOG_DEBUG, "Client main EXIT_FAILURE.");
                return EXIT_FAILURE;
        }
        int exit_status = EXIT_FAILURE;

        client_watcher_list watchers;

        if (!client_watcher_list_create(&watchers)) {
                log_error("malloc watchers");
                goto clean_inot;
        }

        if (!get_settings_from_server(settings)
            || !client_copy_files(inot_fd, &watchers, settings))
                goto clean_inot;

        if (!settings->one_shot
            && !client_event_loop(&watchers, inot_fd, settings))
                goto clean_inot;

        if (!log_packet_send(settings->write_fd, MSG_END_CONNECTION, 0, NULL))
                goto clean_inot;

        syslog(LOG_DEBUG, "Server end connection successfully");

        exit_status = EXIT_SUCCESS;
clean_inot:
        client_watcher_list_destroy(&watchers);
        if (close(inot_fd) == -1) {
                log_error("inotify_close");
                exit_status = EXIT_FAILURE;
        }

        syslog(LOG_DEBUG, "Client main %s.",
               exit_status == EXIT_SUCCESS ? "EXIT_SUCCESS" : "EXIT_FAILURE");

        return exit_status;
}
