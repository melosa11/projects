/**
 * Entry point of program
 *
 * @file main.c
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2020-12-29
 */
#include "target.h"
#include "options.h"
#include "run.h"

#include "client.h"
#include "log.h"
#include "settings.h"
#include "server.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

const char *DEFAULT_PORT = "42069";

static bool daemonize(void)
{
        if (daemon(true, false) == -1) {
                perror("deamon");
                return false;
        }

        return true;
}

static void cleanup(struct settings *settings)
{
        if (settings->sock_fd != -1 && close(settings->sock_fd) == -1)
                log_warning("close socket");
        if (settings->lock_file_fd != -1 && close(settings->lock_file_fd) == -1)
                log_warning("close lock file");
        if (settings->dirfd != -1 && close(settings->dirfd) == -1)
                log_warning("close directory");
        if (close(STDIN_FILENO) == -1)
                log_warning("close STDIN");
        if (close(STDOUT_FILENO) == -1)
                log_warning("close STDOUT");
        if (close(STDERR_FILENO) == -1)
                log_warning("close STDERR");

        closelog();
}

int main(int argc, char *argv[])
{
        struct settings settings = {
                .port = DEFAULT_PORT,
                .read_fd = -1,
                .write_fd = -1,
                .lock_file_fd = -1,
                .sock_fd = -1,
                .dirfd = -1,
        };
        const enum main_opt_code result
                = main_options_parse(argc, argv, &settings);
        if (result != OPT_OK)
                return result == OPT_EXIT ? EXIT_SUCCESS : EXIT_FAILURE;

        if (!settings.foreground && !daemonize())
                return EXIT_FAILURE;

        main_setup_args_t setup_args = &argv[optind];
        struct run_data modules[] = {
                {
                        .log_name = "dropbox_client",
                        .setup_args = setup_args,
                        .setup = main_setup_client,
                        .module_main = client_main,
                },
                {
                        .log_name = "dropbox_server",
                        .setup_args = setup_args,
                        .setup = main_setup_server,
                        .module_main = server_main,
                },
        };

        int exit_code
                = main_run_module(&settings, &modules[settings.client ? 0 : 1]);

        cleanup(&settings);

        // if received term signal then the program should be properly terminated
        if (sigprocmask(SIG_UNBLOCK, &settings.mask, NULL) == -1)
                log_warning("sigprocmask");
        return exit_code;
}
