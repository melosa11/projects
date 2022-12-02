#include "run.h"

#include "log.h"
#include "client.h"
#include "server.h"

#include <syslog.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// blocks signal for client and server process
static bool prepare_signal_handling(struct settings *settings)
{
        sigset_t mask;
        log_assert(sigemptyset(&mask) == 0);
        log_assert(sigaddset(&mask, SIGINT) == 0);
        log_assert(sigaddset(&mask, SIGTERM) == 0);
        log_assert(sigaddset(&mask, SIGPIPE) == 0);

        // need valid mask in settings in case of error
        settings->mask = mask;
        if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
                log_error("sigprocmask");
                return false;
        }
        return true;
}
static void init_logger(const char *log_name, const struct settings *settings)
{
        int option = 0;
        if (settings->foreground && settings->verbose)
                option |= LOG_PERROR;

        int mask = LOG_INFO;
        if (settings->debug)
                mask = LOG_DEBUG;

        if (settings->quiet)
                mask = LOG_WARNING;

        openlog(log_name, option, LOG_USER);
        setlogmask(LOG_UPTO(mask));
}

int main_run_module(struct settings *settings, const struct run_data *data)
{
        init_logger(data->log_name, settings);

        if (!prepare_signal_handling(settings))
                return EXIT_FAILURE;

        if (!data->setup(settings, data->setup_args))
                return EXIT_FAILURE;

        // cwd is set in setup
        if ((settings->dirfd = open(settings->cwd, O_DIRECTORY)) == -1) {
                log_error("open");
                return EXIT_FAILURE;
        }

        return data->module_main(settings);
}
