#include "helper.h"

#include "log.h"

bool client_helper_check_expected_code(enum packet_msg_code got_code,
                                       enum packet_msg_code expected_code)
{
        if (got_code != expected_code)
                syslog(LOG_ERR, "got: %d, expected: %d", got_code,
                       expected_code);

        return got_code == expected_code;
}

/**
 * Reads a packet and return its code.
 *
 * @param settings        struct holding information about behaviour of the program
 * @param packet_buffptr  an address of the pointer of type struct packet *
 * @param nptr            a pointer to a value representing size of packet_buffptr
 * @return                Code from server
 */
static enum packet_msg_code answer_from_server(const struct settings *settings,
                                               struct packet **packet_buffptr,
                                               size_t *nptr)
{
        if (!log_packet_read(settings->read_fd, packet_buffptr, nptr))
                return MSG_ABORT;

        enum packet_msg_code code = (*packet_buffptr)->code;
        if (code == MSG_ABORT) {
                struct packet_payload_abort *p
                        = (struct packet_payload_abort *)(*packet_buffptr)
                                  ->payload;
                syslog(LOG_ERR, "server aborted with: %s",
                       strerror(p->error_number));
        }
        return (*packet_buffptr)->code;
}

enum packet_msg_code
client_helper_get_answer(const struct client_traverse_data *data)
{
        return answer_from_server(data->settings, data->packet_buffptr,
                                  data->nptr);
}
