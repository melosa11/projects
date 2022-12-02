/**
 * @file setup.h
 * @brief Setup function called before module's main
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2021-08-12
 */
#ifndef SETUP_H
#define SETUP_H

#include <settings.h>

typedef char **main_setup_args_t;
typedef bool (*main_setup_t)(struct settings *, main_setup_args_t);

/**
 * @brief Prepares data and enviroment for module
 *
 * @param settings  pointer to the configuration structure
 * @param args[]    module specific arguments
 *
 * @return          true on success;
 *                  false otherwise
 */
#define MAIN_SETUP(NAME)                                                       \
        bool main_setup_##NAME(struct settings *settings,                      \
                               main_setup_args_t args)

MAIN_SETUP(client);
MAIN_SETUP(server);

#endif /* SETUP_H */
