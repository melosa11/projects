/**
 * Definitions of the packet API helper functions
 *
 * @file packet.c
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2020-12-27
 */

#include "packet.h"
#include "net.h"

#include "log.h"

#include <errno.h>
#include <stdlib.h>

static bool expand_buffer(struct packet **packet_buffptr, size_t *n,
                          size_t new_n)
{
        struct packet *bigger_packet
                = realloc(*packet_buffptr, new_n * sizeof(unsigned char));
        if (bigger_packet == NULL)
                return false;
        *packet_buffptr = bigger_packet;
        *n = new_n;
        return true;
}

static bool create_minimal_buffer(struct packet **packet_buffptr, size_t *n)
{
        return expand_buffer(packet_buffptr, n, sizeof(struct packet));
}

static bool read_payload(int fd, struct packet **packet_buffptr, size_t *n)
{
        const size_t min_size
                = sizeof(struct packet) + (*packet_buffptr)->payload_size;
        if (*n < min_size && !expand_buffer(packet_buffptr, n, min_size))
                return false;

        return packet_net_read_payload(fd, *packet_buffptr);
}

static bool validate_code(enum packet_msg_code code)
{
        if (code < MSG_OK || MSG_COUNT <= code) {
                errno = ENOMSG;
                return false;
        }
        return true;
}

static bool read_header(int fd, struct packet **packet_buffptr, size_t *n)
{
        if (*n < sizeof(struct packet)
            && !expand_buffer(packet_buffptr, n, sizeof(struct packet)))
                return false;

#ifdef DEBUG
        syslog(LOG_DEBUG, "packet_read fd: %d", fd);
#endif

        enum packet_msg_code code;
        if (!packet_net_read_code(fd, &code) || !validate_code(code))
                return false;

#ifdef DEBUG
        syslog(LOG_DEBUG, "packet_read code: %d", code);
#endif
        (*packet_buffptr)->code = code;

        size_t payload_size;
        if (!packet_net_read_size(fd, &payload_size))
                return false;
#ifdef DEBUG
        syslog(LOG_DEBUG, "packet_read size: %zu", payload_size);
#endif

        (*packet_buffptr)->payload_size = payload_size;

        return read_payload(fd, packet_buffptr, n);
}
bool packet_read(int fd, struct packet **packet_buffptr, size_t *n)
{
        log_assert(packet_buffptr != NULL);
        log_assert(n != NULL);

        if (*packet_buffptr == NULL
            && !create_minimal_buffer(packet_buffptr, n))
                return false;

        return read_header(fd, packet_buffptr, n);
}

bool packet_send(int fd, enum packet_msg_code code, size_t payload_size,
                 const unsigned char payload[payload_size])
{
        log_assert(MSG_OK <= code && code < MSG_COUNT);
        return packet_net_send(fd, code, payload_size, payload);
}
