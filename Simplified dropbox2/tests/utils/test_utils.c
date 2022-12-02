#define CUT_MAIN

#include "cut.h"

#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

GLOBAL_TEAR_UP()
{
        srand(time(NULL));
}

static void trash_buffer(size_t size, unsigned char buff[size])
{
        for (size_t i = 0; i < size / sizeof(int); i++) {
                int num = rand();
                int *p = (int *)buff;
                memcpy(&p[i], &num, sizeof(int));
        }
}

static unsigned char *create_trashed_buffer(size_t size)
{
        unsigned char *buff = malloc(size);
        ASSERT(buff != NULL);
        trash_buffer(size, buff);
        return buff;
}

#define BUFF_SIZE 1024UL

TEST(read_write)
{
        int fds[2];
        SUBTEST(small_data)
        {
                int num = rand();
                int copy = num;
                int got = rand();
                ASSERT(pipe(fds) == 0);
                ASSERT(utils_write(fds[1], sizeof(copy), &copy) > 0);
                ASSERT(utils_read(fds[0], sizeof(copy), &got) > 0);
                ASSERT(got == num);
        }

        SUBTEST(big_data)
        {
                unsigned char *buff = create_trashed_buffer(BUFF_SIZE);
                trash_buffer(BUFF_SIZE, &buff[0]);
                unsigned char *buff_copy = create_trashed_buffer(BUFF_SIZE);
                memcpy(buff_copy, buff, BUFF_SIZE);
                ASSERT(pipe(fds) == 0);
                ASSERT(utils_write(fds[1], BUFF_SIZE, buff_copy) > 0);

                unsigned char got[BUFF_SIZE] = { 0 };
                ASSERT(utils_read(fds[0], BUFF_SIZE, got) > 0);
                ASSERT(memcmp(got, buff, BUFF_SIZE) == 0);
                free(buff);
                free(buff_copy);
        }
}