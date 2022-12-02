/**
 * @file watcher_list.h
 * @brief Dynamic array of struct watcher
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2021-08-05
 */
#ifndef WATCHER_LIST_H
#define WATCHER_LIST_H

#include "watcher.h"

#include "generic_list.h"

typedef generic_list client_watcher_list;

/**
 * @brief Creates list of watchers
 *
 * @param[out] list pointer to a uninitialized list
 *
 * @return true on success;
 *         false otherwise
 */
bool client_watcher_list_create(client_watcher_list *list);

/**
 * @brief Push @c elem at the end of the @c list
 *
 * @param list pointer to the list
 * @param elem pointer to the watcher to insert
 *
 * @return true on success; false otherwise
 */
bool client_watcher_list_push_back(client_watcher_list *list,
                                   const struct client_watcher *elem);

/**
 * @brief Iterates through the @c list from the start to the end.
 *        Each element of the list and @c data is passed to the @c func.
 *
 * @note See typedef iter_func for detailed description.
 *
 * @param list  pointer to the list
 * @param func  function applied to each element
 * @param data  pointer to a data passed to he @c func
 */
void client_watcher_list_foreach(client_watcher_list *list, iter_func func,
                                 void *data);

/**
 * @brief Analogous to the @e client_watcher_list_foreach(), but the elements
 *        are iterated from end to start.
 * @see generic_list_foreach
 */
void client_watcher_list_foreach_reverse(client_watcher_list *list,
                                         iter_func func, void *data);

/**
 * Finds last watcher specified by @c needle.
 *
 * If @c needle.wd is -1 then every value of this field is matched.
 * If @c needle.name is @c NULL every value of this field is matched.
 *
 * @param list    pointer to the list
 * @param needle  structure containing searching parameters
 *
 * @return a pointer to matched element in the list;
 *         NULL otherwise
 */
struct client_watcher *
client_watcher_list_find_last(client_watcher_list *list,
                              struct client_watcher needle);

/**
 * @brief Release all the resource of the @c list.
 *
 * @param list  pointer to the list
 */
void client_watcher_list_destroy(client_watcher_list *list);

#endif //WATCHER_LIST_H
