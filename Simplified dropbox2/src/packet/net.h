/**
 * @file net.h
 * @brief Abstract Layer between packet and network
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2021-08-16
 */
#ifndef NET_H
#define NET_H

#include "packet.h"

/**
 * @brief Reads message code from the @c fd
 *
 * @param fd             file descriptor open for reading
 * @param[out] code_ptr  a pointer where to store the code
 *
 * @return   true on success;
 *           false othewrise
 */
bool packet_net_read_code(int fd, enum packet_msg_code *code_ptr);

/**
 * @brief Reads size of the payload
 *
 * @param fd             file descriptor open for reading
 * @param[out] size_ptr  a pointer where to store the size
 *
 * @return   true on success;
 *           false othewrise
 */
bool packet_net_read_size(int fd, size_t *size_ptr);

/**
 * Reads payload into @c packet.payload.
 *
 * This operation can change the payload size stored in packet.
 *
 * @param fd      file descriptor open for reading
 * @param packet  a pointer to packet structure
 *
 * @return
 */
bool packet_net_read_payload(int fd, struct packet *packet);

/**
 * @see packet_send()
 */
bool packet_net_send(int fd, enum packet_msg_code code, size_t payload_size,
                     const unsigned char payload[payload_size]);
#endif /* NET_H */
