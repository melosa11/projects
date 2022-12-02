#!/bin/bash
error()
{
    echo "$1" >&2
}

print_pass()
{
    echo -e "\e[92mPassed\e[0m"
}

print_fail()
{
    echo -e "\e[91mFailed\e[0m"
}

create_dir()
{
    if ! [ -d "$1" ]; then
        mkdir -p "$1"
    fi
}

remove_dir()
{
    if [ -d "$1" ]; then
        rm -rf "$1"
    fi
}

get_permissions()
{
    ls -l "$1" | cut -d' ' -f1
}

get_owner()
{
    ls -l "$1" | cut -d' ' -f3,4
}

prepare_dirs()
{
    local TEST_NAME="$1"
    TEST_FOLDER="data/$TEST_NAME"

    remove_dir "$TEST_FOLDER"
    create_dir "$TEST_FOLDER"

    CLIENT_FOLDER="$TEST_FOLDER/client"
    SERVER_FOLDER="$TEST_FOLDER/server"
    create_dir "$CLIENT_FOLDER"
    create_dir "$SERVER_FOLDER"
}

prepare_one_test_file()
{
    prepare_dirs "$1"

    TEST_FILE="$2"
    CLIENT_FILE="$CLIENT_FOLDER/$TEST_FILE"
    SERVER_FILE="$SERVER_FOLDER/$TEST_FILE"


    echo "test data" > "$CLIENT_FILE"
}

# $1 test folder
get_pid()
{
    cat "$1/."*".dropbox.lock"
}

is_equal_perms()
{
    [ `get_permissions "$1"` != `get_permissions "$2"` ]
}

is_equal_owner()
{
    [ "`get_owner "$1"`" != "`get_owner "$2"`" ]
}

is_equal_data()
{
    ! cmp "$1" "$2" 1> /dev/null
}

is_empty_dir()
{
    [ -z "$(ls -A $1)" ]
}

are_files_equals()
{
    if is_equal_perms "$1" "$2"; then
        error "files have different permissions";
        return 1;
    fi

    if is_equal_owner "$1" "$2"; then
        error "files have different owners";
        return 1;
    fi

    if is_equal_data "$1" "$2"; then
        error "files have different data";
        return 1;
    fi
}

# $1 path to valgrind log file
are_open_file_descriptors()
{
    local OPEN_FILES_COUNT=`grep 'Open' "$1" | wc -l`
    local INHERITED_FILES_COUNT=`grep '<inherited from parent>' "$1" | wc -l`

    [ "$OPEN_FILES_COUNT" == "$INHERITED_FILES_COUNT" ]
}

# $1 path to valgrind log file
are_memory_leaks()
{
    ! grep -E -e 'in use at exit: [1-9][0-9]* bytes' "$1" > /dev/null
}

# $1 path to valgrind log file
analyze_valgrind_log()
{
    sleep 1
    if ! are_memory_leaks "$1"; then
        error "Memory leak"
        error "See $1 for more information";
        return 1;
    fi

    if ! are_open_file_descriptors "$1"; then
        error "Not all files closed";
        error "See $1 for more information";
        return 1;
    fi
}
