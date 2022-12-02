#include "utils.h"

#include "log.h"

#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>

#define BLOCK false
#define NOBLOCK true

ssize_t utils_write(int fd, size_t size, const void *buff)
{
        log_assert(buff != NULL);
        const unsigned char *ptr = buff;
        ssize_t bytes_written = 0;
        while (size - bytes_written > 0) {
                const ssize_t ret
                        = write(fd, &ptr[bytes_written], size - bytes_written);
                if (ret == -1) {
                        if (errno == EINTR)
                                continue;
                        return -1;
                }
                bytes_written += ret;
        }
        return bytes_written;
}

static ssize_t _utils_read(int fd, size_t size, void *buff, bool nonblocking)
{
        unsigned char *ptr = buff;
        ssize_t bytes_read = 0;
        ssize_t ret;
        while (size - bytes_read > 0
               && (ret = read(fd, &ptr[bytes_read], size - bytes_read)) != 0) {
                if (ret == -1) {
                        if (errno == EINTR)
                                continue;

                        if (nonblocking
                            && (errno == EAGAIN || errno == EWOULDBLOCK))
                                break;

                        return -1;
                }
                bytes_read += ret;
        }
        return bytes_read;
}

ssize_t utils_read(int fd, size_t size, void *buff)
{
        return _utils_read(fd, size, buff, BLOCK);
}

ssize_t utils_read_nonblocking(int fd, size_t size, void *buff)
{
        return _utils_read(fd, size, buff, NOBLOCK);
}
