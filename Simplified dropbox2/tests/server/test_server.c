/**
 * Unit tests for server.
 *
 * @file server.h
 * @author Matej Kleman (xkleman@fi.muni.cz)
 * @date 2021-01-01
 */

#define CUT_MAIN

#include "packet.h"
#include "settings.h"
#include "test_helper.h"
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <errno.h>

#include "cut.h"

/**
 * Struct holding information needed for testing.
 */
struct test_info {
        struct settings
                settings; /**< struct holding information about behaviour of the program */
        int writefd;  /**< write end of byte stream */
        int readfd;   /**< read end of byte stream */
        pthread_t th; /**< testing thread */
        struct packet
                *pack; /**< struct holding information that is send/received */
        size_t pack_len; /**< size of packet */
        int dirfd;       /**< file descriptor of testing directory */
};

/**
 * Checks whether we received correct message from server.
 *
 * @param info Struct holding information needed for testing
 * @param expected_code Numeric value of expected code
 */
static void check_return_message(struct test_info *info,
                                 enum packet_msg_code expected_code)
{
        ASSERT(packet_read(info->readfd, &info->pack, &info->pack_len));
        CHECK(info->pack->code == expected_code);
}

/**
 * Prepares everything needed for subtest.
 *
 * @param test_name Name of the subtest
 * @param info Struct holding information needed for testing
 */
static void subtest_starter(const char *test_name, struct test_info *info)
{
        info->dirfd = prepare_test(test_name, &info->readfd, &info->writefd,
                                   &info->settings);
        start_server(&info->th, &info->settings);
}

static void _subtest_end(struct test_info *info, enum server_status st)
{
        stop_server(info->th);
        wait_server(info->th, st);
        clean_after_test(&info->settings, info->pack);
}

/**
 * Sends MSG_END_CONNECTION to server, signaling that session is over.
 *
 * @param info Struct holding information needed for testing
 */
static void subtest_end_connection(struct test_info *info)
{
        ASSERT(packet_send(info->writefd, MSG_END_CONNECTION, 0, NULL));
        _subtest_end(info, SERVER_OK);
}

/**
 * Wait for server to end and checks if return value is EXIT_FAILURE.
 *
 * @param info Struct holding information needed for testing
 */
static void subtest_waiter(struct test_info *info)
{
        _subtest_end(info, SERVER_OK);
}

/**
 * Checks whether file was created successfully.
 *
 * @param info Struct holding information needed for testing
 */
static void subtest_create_file(struct test_info *info)
{
        char *test_file = "test_file";
        info->pack_len = strlen(test_file) + 1;
        ASSERT(packet_send(info->writefd, MSG_CREATE_FILE, info->pack_len,
                           (unsigned char *)test_file));
        check_return_message(info, MSG_OK);
        ASSERT(faccessat(info->dirfd, test_file, F_OK, 0) == 0);
}

/**
 * Checks whether file was deleted successfully.
 *
 * @param info Struct holding information needed for testing
 */
static void subtest_delete_file(struct test_info *info)
{
        char *test_file = "test_file";
        ASSERT(faccessat(info->dirfd, test_file, F_OK, 0) == 0);
        info->pack_len = strlen(test_file) + 1;
        ASSERT(packet_send(info->writefd, MSG_DELETE_FILE, info->pack_len,
                           (unsigned char *)test_file));
        check_return_message(info, MSG_OK);
        ASSERT(faccessat(info->dirfd, test_file, F_OK, 0) != 0);
}

/**
 * Sends MSG_DONE to server signaling proccessing of current file is finished.
 *
 * @param info Struct holding information needed for testing
 */
static void send_msg_done(struct test_info *info)
{
        ASSERT(packet_send(info->writefd, MSG_DONE, 0, NULL));
}

/**
 * Checks if timestamps were set successfully on new file.
 *
 * @param info Struct holding information needed for testing
 * @param test_time Timespec structure containing timeinfo
 *                  that we want to check againts.
 */
static void check_timestamps(struct test_info *info, struct timespec test_time)
{
        sync();
        struct stat statbuf;
        ASSERT(fstatat(info->dirfd, "test_file", &statbuf, 0) == 0);
        CHECK(statbuf.st_atim.tv_sec == test_time.tv_sec);
        CHECK(statbuf.st_atim.tv_nsec == test_time.tv_nsec);
        CHECK(statbuf.st_mtim.tv_sec == test_time.tv_sec);
        CHECK(statbuf.st_mtim.tv_nsec == test_time.tv_nsec);
}

/**
 * Sends MSG_SET_TIMESTAMPS to server and checks if correct code was received back.
 *
 * @param info Struct holding information needed for testing
 * @param test_time Timespec structure containing timeinfo
 *                  that we want to check againts.
 * @param expected_code Numeric value of expected code
 */
static void timestamps_helper(struct test_info *info, struct timespec test_time,
                              enum packet_msg_code expected_code)
{
        struct packet_payload_timestamps timestamps = { test_time, test_time };
        info->pack_len = sizeof(struct packet_payload_timestamps);

        ASSERT(packet_send(info->writefd, MSG_SET_TIMESTAMPS, info->pack_len,
                           (unsigned char *)&timestamps));
        check_return_message(info, expected_code);
}

/**
 * Checks if server processes MSG_SET_TIMESTAMPS correctly.
 *
 * @param info Struct holding information needed for testing
 * @param msg_done Flag signaling if we want to end test after the request.
 */
static void subtest_timestamps(struct test_info *info, bool msg_done)
{
        struct timespec test_time = { 42, 0 };
        timestamps_helper(info, test_time, MSG_OK);
        if (msg_done) {
                send_msg_done(info);
                check_timestamps(info, test_time);
        }
}

/**
 * Checks if permissions were set successfully on new file.
 *
 * @param info Struct holding information needed for testing
 * @param mode Permissions that the file is tested against
 */
static void check_permissions(struct test_info *info, mode_t mode)
{
        sync();
        struct stat statbuf;
        ASSERT(fstatat(info->dirfd, "test_file", &statbuf, 0) == 0);
        CHECK((statbuf.st_mode & 0777) == mode);
}

/**
 * Sends MSG_SET_PERM_MODES to server and checks if correct code was received back.
 *
 * @param info Struct holding information needed for testing
 * @param mode Permissions that the file is tested against
 * @param expected_code Numeric value of expected code
 */
static void permissions_helper(struct test_info *info, mode_t mode,
                               enum packet_msg_code expected_code)
{
        info->pack_len = sizeof(mode_t);

        ASSERT(packet_send(info->writefd, MSG_SET_PERM_MODES, info->pack_len,
                           (unsigned char *)&mode));
        check_return_message(info, expected_code);
}

/**
 * Checks if server processes MSG_SET_PERM_NODES correctly.
 *
 * @param info Struct holding information needed for testing
 * @param msg_done Flag signaling if we want to end test after the request.
 */
static void subtest_permissions(struct test_info *info, bool msg_done)
{
        mode_t mode = 0123;
        permissions_helper(info, mode, MSG_OK);

        if (msg_done) {
                send_msg_done(info);
                check_permissions(info, mode);
        }
}

/**
 * Checks if owner uid and gid were set successfully on new file.
 *
 * @param info Struct holding information needed for testing
 */
static void check_owner(struct test_info *info,
                        struct packet_payload_set_owner owner_info)
{
        sync();
        struct stat statbuf;
        ASSERT(fstatat(info->dirfd, "test_file", &statbuf, 0) == 0);
        CHECK(statbuf.st_uid == owner_info.uid
              && statbuf.st_gid == owner_info.gid);
}

/**
 * Sends MSG_SET_OWNER to server and checks if correct code was received back.
 *
 * @param info Struct holding information needed for testing
 * @param owner_info Structure containing owner info
 *                  that we want to check againts.
 * @param expected_code Numeric value of expected code
 */
static void owner_helper(struct test_info *info,
                         struct packet_payload_set_owner owner_info,
                         enum packet_msg_code expected_code)
{
        info->pack_len = sizeof(struct packet_payload_set_owner);

        ASSERT(packet_send(info->writefd, MSG_SET_OWNER, info->pack_len,
                           (unsigned char *)&owner_info));
        check_return_message(info, expected_code);
}

/**
 * Checks if server processes MSG_SET_OWNER correctly.
 *
 * @param info Struct holding information needed for testing
 * @param msg_done Flag signaling if we want to end test after the request.
 */
static void subtest_owner(struct test_info *info, bool msg_done)
{
        struct packet_payload_set_owner owner_info = { 1, 2 };
        owner_helper(info, owner_info, MSG_OK);

        if (msg_done) {
                send_msg_done(info);
                check_owner(info, owner_info);
        }
}

/**
 * Requests server to write one block of data.
 *
 * @param info Struct holding information needed for testing
 * @param buffer Pointer to block of data that sent to server
 */
static void write_helper(struct test_info *info, void *buffer)
{
        ASSERT(packet_send(info->writefd, MSG_WRITE_BLOCK,
                           info->settings.fs_block_size,
                           (unsigned char *)buffer));
        check_return_message(info, MSG_OK);
}

/**
 * Checks whether writing single block of data works correctly.
 *
 * @param info Struct holding information needed for testing
 */
static void subtest_write_one_block(struct test_info *info)
{
        void *buffer;
        ASSERT((buffer = malloc(info->settings.fs_block_size)) != NULL);
        ASSERT(memset(buffer, 42, info->settings.fs_block_size) != NULL);

        write_helper(info, buffer);
        struct stat statbuf;
        ASSERT(fstatat(info->dirfd, "test_file", &statbuf, 0) == 0);

        ASSERT((size_t)statbuf.st_size == info->settings.fs_block_size);
        free(buffer);
}

/**
 * Checks whether writing ten blocks of data works correctly.
 *
 * @param info Struct holding information needed for testing
 */
static void subtest_write_ten_blocks(struct test_info *info)
{
        void *buffer;
        ASSERT((buffer = malloc(info->settings.fs_block_size)) != NULL);
        ASSERT(memset(buffer, 0, info->settings.fs_block_size) != NULL);

        for (int i = 0; i < 10; i++) {
                write_helper(info, buffer);
        }
        struct stat statbuf;
        ASSERT(fstatat(info->dirfd, "test_file", &statbuf, 0) == 0);
        ASSERT((size_t)statbuf.st_size == 10 * info->settings.fs_block_size);
        free(buffer);
}

/**
 * Checks whether program processes sparse files correctly.
 *
 * @param info Struct holding information needed for testing
 */
static void subtest_sparse_file(struct test_info *info)
{
        info->settings.sparse = true;
        ASSERT(packet_send(info->writefd, MSG_WRITE_BLOCK, 0, NULL));
        check_return_message(info, MSG_OK);

        sync();
        struct stat statbuf;
        ASSERT(fstatat(info->dirfd, "test_file", &statbuf, 0) == 0);
        ASSERT(statbuf.st_blocks * 512 <= statbuf.st_size);
        ASSERT(statbuf.st_size == 0);
        ASSERT(statbuf.st_blocks == 0);
}

/**
 * Creates a test_file and informs server about its incoming change.
 *
 * @param info Struct holding information needed for testing
 */
static void change_helper(struct test_info *info)
{
        ASSERT(openat(info->dirfd, "test_file", O_CREAT, 0666) != -1);
        ASSERT(faccessat(info->dirfd, "test_file", F_OK, 0) == 0);
        ASSERT(packet_send(info->writefd, MSG_CHANGE_FILE,
                           strlen("test_file") + 1,
                           (unsigned char *)"test_file"));
        check_return_message(info, MSG_OK);
}

/**
 * Checks whether changing of timestamps works correctly.
 *
 * @param info Struct holding information needed for testing
 */
static void subtest_change_timestamps(struct test_info *info)
{
        change_helper(info);
        struct timespec test_time = { 420, 0 };
        timestamps_helper(info, test_time, MSG_OK);
        check_timestamps(info, test_time);
}

/**
 * Checks whether changing of owner works correctly.
 *
 * @param info Struct holding information needed for testing
 */
static void subtest_change_owner(struct test_info *info)
{
        change_helper(info);
        struct packet_payload_set_owner owner_info = { 1337, 420 };
        owner_helper(info, owner_info, MSG_OK);
        check_owner(info, owner_info);
}

/**
 * Checks whether changing of permissions works correctly.
 *
 * @param info Struct holding information needed for testing
 */
static void subtest_change_perms(struct test_info *info)
{
        change_helper(info);
        mode_t mode = 0321;
        permissions_helper(info, mode, MSG_OK);
        check_permissions(info, mode);
}

/**
 * Checks whether rewriting of file works correctly.
 *
 * @param info Struct holding information needed for testing
 */
static void subtest_rewrite_blocks(struct test_info *info)
{
        change_helper(info);
        subtest_write_ten_blocks(info);
}

/**
 * Sends MSG_CHANGE_FILE when not expected in protocol.
 *
 * @param info Struct holding information needed for testing
 */
static void another_change(struct test_info *info)
{
        char *test_file = "test_file";
        ASSERT(faccessat(info->dirfd, test_file, F_OK, 0) == 0);
        ASSERT(packet_send(info->writefd, MSG_CHANGE_FILE,
                           strlen(test_file) + 1, (unsigned char *)test_file));
        check_return_message(info, MSG_ABORT);
}

/**
 * Sends MSG_CHANGE_FILE when not expected in protocol.
 *
 * @param info Struct holding information needed for testing
 */
static void another_create(struct test_info *info)
{
        char *test_file = "test_file";
        info->pack_len = strlen(test_file) + 1;
        ASSERT(packet_send(info->writefd, MSG_CREATE_FILE, info->pack_len,
                           (unsigned char *)test_file));
        check_return_message(info, MSG_ABORT);
}

TEST(server_basic)
{
        struct test_info info = { 0 };

        SUBTEST(create_file)
        {
                subtest_starter("create_file", &info);
                subtest_create_file(&info);
                subtest_end_connection(&info);
        }

        SUBTEST(delete_file)
        {
                subtest_starter("delete_file", &info);
                subtest_create_file(&info);
                subtest_delete_file(&info);
                subtest_end_connection(&info);
        }

        SUBTEST(timestamps)
        {
                subtest_starter("timestamps", &info);
                subtest_create_file(&info);
                subtest_timestamps(&info, true);
                subtest_end_connection(&info);
        }

        SUBTEST(permissions)
        {
                subtest_starter("permissions", &info);
                subtest_create_file(&info);
                subtest_permissions(&info, true);
                subtest_end_connection(&info);
        }
        SUBTEST(owner)
        {
                subtest_starter("owner", &info);
                subtest_create_file(&info);
                subtest_owner(&info, true);
                subtest_end_connection(&info);
        }

        SUBTEST(write_one_block)
        {
                subtest_starter("write_one_block", &info);
                subtest_create_file(&info);
                subtest_write_one_block(&info);
                subtest_end_connection(&info);
        }

        SUBTEST(write_ten_blocks)
        {
                subtest_starter("write_ten_blocks", &info);
                subtest_create_file(&info);
                subtest_write_ten_blocks(&info);
                subtest_end_connection(&info);
        }

        SUBTEST(test_all_together)
        {
                subtest_starter("test_all_together", &info);
                subtest_create_file(&info);
                subtest_timestamps(&info, false);
                subtest_permissions(&info, false);
                subtest_owner(&info, false);
                subtest_write_one_block(&info);
                send_msg_done(&info);
                subtest_end_connection(&info);
        }
}

TEST(server_opts)
{
        struct test_info info = { 0 };

        SUBTEST(sparse_file)
        {
                subtest_starter("sparse_file", &info);
                subtest_create_file(&info);
                subtest_sparse_file(&info);
                subtest_end_connection(&info);
        }
}

TEST(server_change)
{
        struct test_info info = { 0 };

        SUBTEST(change_timestamps)
        {
                subtest_starter("change_timestamps", &info);
                subtest_change_timestamps(&info);
                subtest_end_connection(&info);
        }

        SUBTEST(change_timestamps_no_file)
        {
                subtest_starter("change_timestamps_no_file", &info);
                timestamps_helper(&info, (struct timespec){ 420, 0 },
                                  MSG_ABORT);
                subtest_waiter(&info);
        }

        SUBTEST(change_owner)
        {
                subtest_starter("change_owner", &info);
                subtest_change_owner(&info);
                subtest_end_connection(&info);
        }

        SUBTEST(change_owner_no_file)
        {
                subtest_starter("change_timestamps_no_file", &info);
                owner_helper(&info, (struct packet_payload_set_owner){ 10, 20 },
                             MSG_ABORT);
                subtest_waiter(&info);
        }

        SUBTEST(change_perms)
        {
                subtest_starter("change_perms", &info);
                subtest_change_perms(&info);
                subtest_end_connection(&info);
        }

        SUBTEST(change_perms_no_file)
        {
                subtest_starter("change_timestamps_no_file", &info);
                permissions_helper(&info, 0420, MSG_ABORT);
                subtest_waiter(&info);
        }

        SUBTEST(change_write_blocks)
        {
                subtest_starter("change_write_blocks", &info);
                subtest_rewrite_blocks(&info);
                send_msg_done(&info);
                subtest_end_connection(&info);
        }

        SUBTEST(double_change)
        {
                subtest_starter("double_change", &info);
                change_helper(&info);
                another_change(&info);
                subtest_waiter(&info);
        }

        SUBTEST(double_create)
        {
                subtest_starter("double_create", &info);
                subtest_create_file(&info);
                another_create(&info);
                subtest_waiter(&info);
        }

        SUBTEST(create_after_change)
        {
                subtest_starter("create_after_change", &info);
                change_helper(&info);
                another_create(&info);
                subtest_waiter(&info);
        }

        SUBTEST(change_after_create)
        {
                subtest_starter("change_after_create", &info);
                subtest_create_file(&info);
                another_change(&info);
                subtest_waiter(&info);
        }
}
