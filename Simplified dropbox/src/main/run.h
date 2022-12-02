/**
 * @file run.h
 * @brief Module managing running the client and server modules
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2021-08-06
 */
#ifndef RUN_H
#define RUN_H

#include "setup.h"

#include "settings.h"

#include <stdbool.h>
#include <sys/types.h>

struct run_data {
        const char *log_name; /**< name of the log for a program */
        main_setup_args_t setup_args;
        main_setup_t setup;
        int (*module_main)(struct settings *); /**< entry point of a module */
};

/**
 * @brief Runs client and server children programs.
 *
 * @param settings  pointer to the configuration structure
 * @param data      pointer to run information
 *
 * @return  EXIT_SUCCESS on success;
 *          EXIT_FAILURE otherwise
 */
int main_run_module(struct settings *settings, const struct run_data *data);
#endif // RUN_H
