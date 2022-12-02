/**
 * @file packet.h
 * @brief Packet API for communicating between the client and the server
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2020-12-27
 */
#ifndef PACKET_H
#define PACKET_H

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/**
 * Message code of packets
 *
 * @note new codes must be add right before @c MSG_COUNT
 */
enum packet_msg_code {
        MSG_OK,             /**< operation success                          */
        MSG_NOK,            /**< operation failure                          */
        MSG_ABORT,          /**< fatal error                                */
        MSG_SETTINGS,       /**< settings of the sender                     */
        MSG_CREATE_FILE,    /**< create a regular file                      */
        MSG_SET_TIMESTAMPS, /**< timestamps of the files                    */
        MSG_SET_PERM_MODES, /**< set file permission mode for the open file */
        MSG_SET_OWNER,      /**< set owner of the open file                 */
        MSG_WRITE_BLOCK,    /**< write block of data to the open file       */
        MSG_DONE,           /**< the end of the request                     */
        MSG_END_CONNECTION, /**< the end of the connection                  */
        MSG_DELETE_FILE,    /**< delete a file                              */
        MSG_CHANGE_FILE,    /**< change a regular file                      */
        MSG_REJECTED,       /**< client rejected by server                  */
        MSG_COUNT,          /**< count of MSG codes                         */
};

static_assert(MSG_COUNT <= (1u << sizeof(uint8_t) * CHAR_BIT),
              "message codes cannot be represented by uint8_t");

struct packet {
        enum packet_msg_code code; /**< code representing the action */
        size_t payload_size;       /**< size of payload */
        unsigned char payload[];   /**< code specific payload */
};

/**************************************************
 * Payload definitions for corresponding messages *
 **************************************************/

struct packet_payload_abort {
        int32_t error_number; /**< errno coresponding to a failure */
};

struct packet_payload_settings {
        uint64_t fs_block_size; /**< filesystem block size */
};

struct packet_payload_set_perm_modes {
        mode_t mode; /**< unix file permissions */
};

struct packet_payload_set_owner {
        uid_t uid; /**< uid of the owner of the file */
        gid_t gid; /**< gid of the owner of the file */
};

struct packet_payload_timestamps {
        struct timespec atim; /**< time of last access */
        struct timespec mtim; /**< time of last modification */
};

//               Table of messages with variable length of the payload and their meaning
//
// +===================+============================+===============================================+
// |      code         |        payload_size        |                    payload                    |
// +===================+============================+===============================================+
// | MSG_WRITE_BLOCK   | number of bytes            | bytes of data                                 |
// +-------------------+----------------------------+-----------------------------------------------+
// | MSG_CREATE_FILE   | path length including '\0' | null-terminated byte string representing path |
// +-------------------+----------------------------+-----------------------------------------------+
// | MSG_CHANGE_FILE   | path length including '\0' | null-terminated byte string representing path |
// +-------------------+----------------------------+-----------------------------------------------+
// | MSG_DELETE_FILE   | path length including '\0' | null-terminated byte string representing path |
// +-------------------+----------------------------+-----------------------------------------------+

/***********************************************
 * Functions for sending and receiving packets *
 ***********************************************/

/**
 * Reads a packet from @c fd.
 *
 * packet_read() will read a whole packet from the @c fd and stores it at
 * @c *packet_buffptr depending on the message code contained in buffer.
 *
 * If @c *packet_buffptr is set to @c NULL before the call, then packet_read()
 * will allocate a buffer for storing the packet. This buffer
 * should be freed by the user program even if packet_read() failed.
 *
 * Alternatively, before calling packet_read(), @c *packet_buffptr can contain
 * a pointer to a malloc(3)-allocated buffer @c *n bytes in size. If the buffer
 * is not large enough to hold the packet, packet_read() resizes it with realloc(3),
 * updating @c *packet_buffptr and @c *n as necessary.
 *
 * In either case, on a successful call, @c *packet_buffptr and @c *n
 * will be updated to reflect the buffer address and allocated size respectively.
 *
 * Errors:
 *  	ENOMSG - packet contained unknown message code.
 *
 * For other errors see read(2), malloc(3) and realloc(3).
 *
 * @param fd 		     a file descriptor open for writing
 * @param packet_buffptr an address of the pointer of type struct packet *
 * @param n 		     a pointer to a value representing size of @c packet_buffptr
 * @return		         true on success;
 * 			             false on failure and errno is set appropriately
 */
bool packet_read(int fd, struct packet **packet_buffptr, size_t *n);

/**
 * Sends a packet through @c fd.
 *
 * @note If the packet doesn't have payload then @c payload_size should
 * 	 be 0 and @c payload NULL
 *
 * @note For errors see write(2).
 *
 * @param fd 		    a file descriptor open for writing
 * @param code 		    a message code of a packet
 * @param payload_size  a size of the payload in bytes
 * @param payload       a pointer to a payload buffer
 * @return		        true on success;
 * 			            false on failure and errno is set appropriately
 */
bool packet_send(int fd, enum packet_msg_code code, size_t payload_size,
                 const unsigned char payload[payload_size]);

#endif //PACKET_H
