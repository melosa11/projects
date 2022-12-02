#define CUT_MAIN

#include "cut.h"

#include "packet.h"
#include "utils.h"

#include <errno.h>
#include <stdlib.h>
#include <time.h>

GLOBAL_TEAR_UP()
{
        srand(time(NULL));
}

static struct packet_map {
        enum packet_msg_code code;
        size_t payload_size;
} FIXED_PACKETS[] = {
        { .code = MSG_OK, .payload_size = 0 },
        { .code = MSG_NOK, .payload_size = 0 },
        { .code = MSG_DONE, .payload_size = 0 },
        { .code = MSG_END_CONNECTION, .payload_size = 0 },
        { .code = MSG_REJECTED, .payload_size = 0 },
        { .code = MSG_ABORT,
          .payload_size = sizeof(struct packet_payload_abort) },
        { .code = MSG_SETTINGS,
          .payload_size = sizeof(struct packet_payload_settings) },
        { .code = MSG_SET_PERM_MODES,
          .payload_size = sizeof(struct packet_payload_set_perm_modes) },
        { .code = MSG_SET_OWNER,
          .payload_size = sizeof(struct packet_payload_set_owner) },
        { .code = -1 },
};

static struct packet_map FMA_PACKETS[] = {
        { .code = MSG_CREATE_FILE },
        { .code = MSG_DELETE_FILE },
        { .code = MSG_CHANGE_FILE },
        { .code = MSG_WRITE_BLOCK },
        { .code = -1 },
};

static void trash_buffer(size_t size, unsigned char buff[size])
{
        for (size_t i = 0; i < size / sizeof(int); i++) {
                int num = rand();
                int *p = (int *)buff;
                memcpy(&p[i], &num, sizeof(int));
        }
        int mod = (size % sizeof(int));
        if (mod != 0) {
                int num = rand();
                memcpy(&buff[size - mod], &num, mod);
        }
}

static unsigned char *create_trashed_buffer(size_t size)
{
        unsigned char *buff = malloc(size);
        ASSERT(buff != NULL);
        trash_buffer(size, buff);
        return buff;
}

static void test_read_packet(int fds[2], struct packet **packet_buff, size_t *n,
                             struct packet_map test_packet)
{
        unsigned char *buff = NULL;
        if (test_packet.payload_size != 0)
                buff = create_trashed_buffer(test_packet.payload_size);
        ASSERT(packet_send(fds[1], test_packet.code, test_packet.payload_size,
                           buff));
        ASSERT(packet_read(fds[0], packet_buff, n));
        ASSERT(*packet_buff != NULL);
        ASSERT(*n >= test_packet.payload_size + sizeof(test_packet.code));
        ASSERT((*packet_buff)->code == test_packet.code);
        ASSERT((*packet_buff)->payload_size == test_packet.payload_size);
        ASSERT(memcmp((*packet_buff)->payload, buff, test_packet.payload_size)
               == 0);
        free(buff);
        DEBUG_MSG("packet: %d PASSED", test_packet.code);
}

static void test_fma_read_packet(int fds[2], struct packet **packet_buff,
                                 size_t *n, struct packet_map test_packet)
{
        test_packet.payload_size = (rand() % 4096) + 1;
        test_read_packet(fds, packet_buff, n, test_packet);
}

TEST(packet_communication)
{
        int fds[2];
        struct packet *packet = NULL;
        size_t n = 0;

        SUBTEST(force_packet_allocation)
        {
                ASSERT(pipe(fds) == 0);
                ASSERT(packet_send(fds[1], MSG_OK, 0UL, NULL));
                ASSERT(packet_read(fds[0], &packet, &n));
                ASSERT(packet != NULL);
                ASSERT(n == sizeof(struct packet));
        }
        SUBTEST(fixed_packets)
        {
                ASSERT(pipe(fds) == 0);
                for (size_t i = 0;
                     FIXED_PACKETS[i].code != (enum packet_msg_code) - 1; i++) {
                        test_read_packet(fds, &packet, &n, FIXED_PACKETS[i]);
                }
        }
        SUBTEST(fma_packets)
        {
                ASSERT(pipe(fds) == 0);
                for (size_t i = 0;
                     FMA_PACKETS[i].code != (enum packet_msg_code) - 1; i++) {
                        test_fma_read_packet(fds, &packet, &n, FMA_PACKETS[i]);
                }
        }

        SUBTEST(bad_fd)
        {
                ASSERT(!packet_send(-1, MSG_OK, 0, NULL));
                ASSERT(errno == EBADF);
                ASSERT(!packet_read(-1, &packet, &n));
                CHECK(errno == EBADF);
        }

        free(packet);
}
