/**
 * @file log.h
 * @brief Definitions of the logging function
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2021-08-05
 */
#ifndef LOG_H
#define LOG_H

#include "packet.h"

#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief analogous to perror(3)
 */
static inline void log_error(const char *function_name)
{
        syslog(LOG_ERR, "%s: %s.", function_name, strerror(errno));
}

static inline void log_warning(const char *function_name)
{
        syslog(LOG_WARNING, "%s: %s.", function_name, strerror(errno));
}

/**
 * @brief Log version of the assert(3).
 *
 * @param EXPR  boolean expression to be asserted.
 */
#define log_assert(EXPR)                                                       \
        do {                                                                   \
                if (!(EXPR)) {                                                 \
                        syslog(LOG_ERR, "'%s', file:'%s', line:'%d'", #EXPR,   \
                               __FILE__, __LINE__);                            \
                        abort();                                               \
                }                                                              \
        } while (0)

/**
 * @brief Log version of the packet_read from packet.h module.
 *
 * @see packet_read()
 */
static inline bool log_packet_read(int fd, struct packet **packet_buffptr,
                                   size_t *nptr)
{
        bool success = packet_read(fd, packet_buffptr, nptr);
        if (success) {
                syslog(LOG_DEBUG,
                       "received packet code: %d | payload_size: %zu",
                       (*packet_buffptr)->code,
                       (*packet_buffptr)->payload_size);
        } else {
                log_error("packet read");
        }
        return success;
}

/**
 * @brief Log version of the packet_send from packet.h module.
 *
 * @see packet_send()
 */
static inline bool log_packet_send(int fd, enum packet_msg_code code,
                                   size_t payload_size,
                                   const unsigned char payload[payload_size])
{
        syslog(LOG_DEBUG, "sent packet code: %d | payload_size: %zu", code,
               payload_size);
        bool success = packet_send(fd, code, payload_size, payload);
        if (!success)
                log_error("packet send");
        return success;
}
#endif //LOG_H
