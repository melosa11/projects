#!/bin/bash

cd "$(dirname "$0")"

source variables.sh
source helper.sh
source runner.sh

source test_suite/oneshot.sh
source test_suite/notify.sh
source test_suite/net.sh

#1 test name
run_test()
{
    echo "Testing $1"
    $1
    if [ $? -eq 1 ]; then
        EXIT_VALUE=1
        cat "$TEST_FOLDER/server.err" 2> /dev/null
        cat "$TEST_FOLDER/client.err" 2> /dev/null
        print_fail
        return 1
    fi
    print_pass
}

run_test_suite()
{
    local EXIT_VALUE=0
    run_test "test_flag_help"
    run_test "test_flag_force"
    run_test "test_absolute_single_copy"
    run_test "test_relative_single_copy"
    run_test "test_single_copy_on_custom_port"
    run_test "test_hundred_files"
    run_test "test_sparse_file"

    run_test "test_notify_create"
    run_test "test_notify_create_then_change"
    run_test "test_notify_modify"
    run_test "test_notify_delete"
    run_test "test_notify_rename"
    run_test "test_notify_many_files"
    run_test "test_notify_create_delete_create"
    run_test "test_notify_move_to"
    run_test "test_notify_move_from"

    run_test "test_net_reject_second_connection"
    run_test "test_net_two_clients"

    return $EXIT_VALUE
}

run_test_suite
