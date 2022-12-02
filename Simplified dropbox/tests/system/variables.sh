#!/bin/bash
VALGRIND="valgrind --leak-check=full --track-fds=yes --trace-children=yes --error-exitcode=1 --track-origins=yes"
# Path to program
PROGRAM="./dropbox -d"
# Default host for client
SERVER_HOST="localhost"
