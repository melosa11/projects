#include "options.h"

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#define SIMPLE_OPTIONS                                                         \
        X(client, 'C', "C")                                                    \
        X(server, 'S', "S")                                                    \
        X(verbose, 'v', "v")                                                   \
        X(sparse, 's', "s")                                                    \
        X(foreground, 'n', "n")                                                \
        X(force, 'f', "f")                                                     \
        X(debug, 'd', "d")                                                     \
        X(quiet, 'q', "q")

#define X(NAME, VAL, VAL_STR) VAL_STR
static const char *OPTSTRING = SIMPLE_OPTIONS "pho";
#undef X

#define X(NAME, VAL, VAL_STR) { .name = #NAME, .val = VAL },
static const struct option LONGOPTS[] = {
        // clang-format off
        SIMPLE_OPTIONS
#undef X
        // clang-format on
        { .name = "port", .val = 'p', .has_arg = required_argument },
        { .name = "one-shot", .val = 'o' },
        { 0 },
};

static void help(const char *program_name)
{
        printf("Usage: %s (-S|--server) [OPTIONS] TARGET\n"
               "       %s (-C|--client) [OPTIONS] HOST SOURCE\n"
               "\n"
               "DESCRIPTION:\n"
               "If the program is run as client, then it will send files from\n"
               "SOURCE to HOST. Additionally if -o isn't set, the client starts\n"
               "watching changes in the SOURCE and synchronize them with the server.\n"
               "If the program is run as server, then it will listen for the\n"
               "client requests.\n"
               "\n"
               "OPTIONS:\n"
               "    -p,--port=N       The port at which the server is listening.\n"
               "\n"
               "    -C,--client       Starts program as client.\n"
               "\n"
               "    -o,--one-shot     Synchronizes the folders and ends.\n"
               "\n"
               "    -S,--server       Starts program as server.\n"
               "\n"
               "    -f,--force        Allows synchronization with non-empty TARGET folder.\n"
               "\n"
               "\n"
               "    -h                Shows this message.\n"
               "\n"
               "    -v,--verbose      Prints every file/object it works with.\n"
               "\n"
               "    -n,--foreground   Runs in foreground.\n"
               "\n"
               "    -d,--debug        Sets DEBUG log level.\n"
               "\n"
               "    -q,--quiet        Sets WARNING log level.\n"
               "\n"
               "    -s,--sparse       Saves files with holes effectively.\n",
               program_name, program_name);
}

static bool validate_options(int argc, struct settings *settings)
{
        if (settings->server == settings->client) {
                fprintf(stderr,
                        "mutually exclusive flags --client and --server are required\n");
                return false;
        }

        int expected_non_option_arguments = settings->client ? 2 : 1;

        if (argc - optind != expected_non_option_arguments) {
                fprintf(stderr,
                        "required exactly %d non-option arguments\n"
                        "try '-h'\n",
                        expected_non_option_arguments);
                return false;
        }

        return true;
}

enum main_opt_code main_options_parse(int argc, char *const argv[],
                                      struct settings *settings)
{
        int opt;
        while ((opt = getopt_long(argc, argv, OPTSTRING, LONGOPTS, NULL))
               != -1) {
                switch (opt) {
#define X(NAME, VAL, VAL_STR)                                                  \
        case VAL:                                                              \
                settings->NAME = true;                                         \
                break;
                        SIMPLE_OPTIONS
#undef X
                case 'h':
                        help(argv[0]);
                        return OPT_EXIT;

                case 'o':
                        settings->one_shot = true;
                        settings->foreground = true;
                        break;
                case 'p':
                        settings->port = optarg;
                        break;
                default:
                        return OPT_ERROR;
                        break;
                }
        }

        return validate_options(argc, settings) ? OPT_OK : OPT_ERROR;
}
