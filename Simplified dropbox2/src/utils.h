/**
 * @file utils.h
 * @brief Declaration of helper functions
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2020-12-27
 */
#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>

#define UNUSED(VAR) ((void)(VAR))

enum utils_loop_status {
        UTILS_LOOP_CONTINUE,
        UTILS_LOOP_BREAK,
        UTILS_LOOP_ERROR,
};

/**
 * Writes exactly @c size bytes from the @c buff into @c fd.
 *
 * If @c size is 0 then this function do nothing and returns 0.
 *
 * @note For errors see write(2).
 *
 * @param fd    a file descriptor open for writing
 * @param size  a number of bytes to write
 * @param buff  a pointer to a buffer containing data for writing
 * @return      the number of bytes written on success;
 * 	        -1 on failure and errno is set appropriately
 */
ssize_t utils_write(int fd, size_t size, const void *buff);

/**
 * Reads at most @c size bytes from the @c fd into @c buff.
 *
 * @note The difference between this function and read(2) is that
 *       if there are at least @c size bytes available then this
 *       function will try to read exactly @c size bytes a.k.a a
 *       full read.
 *
 * @note Return value of 0 means that the EOF was reached. This holds
 *       true for positive @c size
 *
 * @note For errors see read(2).
 *
 * @param fd 	a file descriptor open for reading
 * @param size  a number of bytes to read
 * @param buff  a pointer to a buffer for storing data
 * @return      the number of bytes read on success;
 *              -1 on failure and errno is set appropriately
 */
ssize_t utils_read(int fd, size_t size, void *buff);

/**
 * @brief Same sa @c utils_read but will not block for nonblocking
 *        @c fd.
 */
ssize_t utils_read_nonblocking(int fd, size_t size, void *buff);

#endif //UTILS_H
