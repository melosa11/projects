/**
 * @file options.h
 * @brief Declaration of functions for parsing the options from command line.
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2020-12-30
 */

#ifndef OPTIONS_H
#define OPTIONS_H

#include "settings.h"

#include <stdbool.h>

enum main_opt_code {
        OPT_OK,
        OPT_EXIT,
        OPT_ERROR,
};

/**
 * @brief Parse commnad line options.
 *
 * @param argc      main's argc
 * @param argv[]    main's argv
 * @param settings     pointer to configuration structure
 *
 * @return  OPT_OK    on success
 *          OPT_EXIT  to exit sucessfuly
 *          OPT_ERROR on error
 */
enum main_opt_code main_options_parse(int argc, char *const argv[],
                                      struct settings *settings);

#endif //OPTIONS_H
