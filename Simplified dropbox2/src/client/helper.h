/**
 * @file helper.h
 * @brief Helper functions for packet handling
 * @author Peter Mercell
 * @date 2021-08-05
 */
#ifndef HELPER_H
#define HELPER_H

#include "traverse_data.h"

/**
 * Checks if an expected code is equal to got code and logs it
 *
 * @param got_code       a message code of a packet
 * @param expected_code  a message code of a packet
 * @return               true on success;
 *                       false on failure
 */
bool client_helper_check_expected_code(enum packet_msg_code got_code,
                                       enum packet_msg_code expected_code);

/**
 * Gets answer from server
 *
 * @param data   struct holding data, which are used when traversing a folder
 * @return       an answer from server
 */
enum packet_msg_code
client_helper_get_answer(const struct client_traverse_data *data);

#endif //HELPER_H
