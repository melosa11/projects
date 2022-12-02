#!/bin/bash

# $1 - flags
# $2 - arguments
# $3 - prefix
# $4 - PORT
_run_program()
{
    LOG_FILE="$TEST_FOLDER/valgrind.$3.log"

    STDOUT_FILE="$TEST_FOLDER/$3.out"
    STDERR_FILE="$TEST_FOLDER/$3.err"

    local FLAG="--"
    if [ "$4" != "" ]; then
        local FLAG="--port $4"
    fi

    if ! $VALGRIND --log-file="$LOG_FILE" $PROGRAM --$3 $1 $FLAG $2 \
        >"$STDOUT_FILE" 2>"$STDERR_FILE"
    then
        error "Program exited with non-zero value";
        return 1;
    fi

}

# $1 - flags
# $2 - SOURCE folder
# $3 - PORT
run_client()
{
    _run_program "$1" "$SERVER_HOST $2" "client" "$3"
    RET=$?
    CLIENT_LOG_FILE="$LOG_FILE"
    return $RET
}

# $1 - flags
# $2 - TARGET folder
# $3 - PORT
run_server()
{
    _run_program "$1" "$2" "server" "$3"
    RET=$?
    SERVER_LOG_FILE="$LOG_FILE"
    return $RET
}

end_server()
{
    local PID=`get_pid "$TEST_FOLDER"`
    if ! kill $PID; then
        return 1
    fi
}

# $1 - client flags
# $2 - SOURCE folder
# $3 - PORT
run_oneshot_client()
{
    if ! run_client "-o -n $1" "$2" "$3"; then
        return 1
    fi

    analyze_valgrind_log "$CLIENT_LOG_FILE"
}

# $1 - callback
# $2 - client flags
# $3 - SOURCE folder
# $4 - TARGET folder
# $5 - PORT
run_oneshot_server_with_callback()
{
    if ! run_server "-f" "$4" "$5"; then
        return 1
    fi

    $1 "$2" "$3" "$5"
    RET=$?

    if ! end_server; then
        return 1
    fi

    if ! analyze_valgrind_log "$SERVER_LOG_FILE"; then
        return 1
    fi
    return $RET
}

# $1 - client flags
# $2 - SOURCE folder
# $3 - TARGET folder
# $4 - PORT
run_oneshot()
{
    run_oneshot_server_with_callback "run_oneshot_client" "$1" "$2" "$3" "$4"
}

notify_start()
{
    if ! run_server "-f" "$SERVER_FOLDER"; then
        return 1
    fi

    SERVER_LOG_FILE="$LOG_FILE"

    run_client "" "$CLIENT_FOLDER"

    CLIENT_LOG_FILE="$TEST_FOLDER/valgrind.client.log"
}

notify_end()
{
    sleep 1
    if ! end_server; then
        return 1
    fi

    if ! analyze_valgrind_log "$SERVER_LOG_FILE"; then
        return 1
    fi

    analyze_valgrind_log "$CLIENT_LOG_FILE"
}

# $1 - callback
notify_run_test()
{
    if ! notify_start; then
        return 1
    fi

    $1
    RET=$?

    if ! notify_end; then
        return 1
    fi
    return $RET
}
