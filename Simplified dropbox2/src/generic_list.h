/**
 * @file generic_list.h
 * @brief Dynamic array of bytes
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2021-08-04
 */

#ifndef GENERIC_LIST_H
#define GENERIC_LIST_H

#include <stdlib.h>
#include <stdbool.h>

/**
 * @brief Opaque type representing dynamic list.
 *
 * @note Members of this struct should not be access directly.
 *       Accessible for an ability to allocate on the stack.
 */
typedef struct generic_list {
        unsigned char *_array;
        size_t _elem_size;
        size_t _used;
        size_t _size;
} generic_list;

/**
 * @brief Used in foreach funtions. Each element is passed to a function
 * of this type.
 *
 * @param element pointer to an element of the list
 * @param data    pointer to custom data
 *
 * @return true if continue iterating the list;
 *         false if stop iterating the list
 */
typedef bool (*iter_func)(void *element, void *data);

/**
 * @brief Dynamically creates the list and store it to @c list.
 *
 * @note If @c list was created by this function, the @e generic_list_destroy()
 *       should be called first
 *
 * @param[out] list  pointer to the uninitialized list
 * @param elem_size  size of the element in the list
 *
 * @return true on success;
 *         false otherwise
 */
bool generic_list_create(generic_list *list, size_t elem_size);

/**
 * @brief Push the element @c elem of expected size to end of the @c list.
 *
 * @param list  pointer to the list
 * @param elem  pointer to the element to push
 *
 * @return true on success;
 *         false otherwise
 */
bool generic_list_push_back(generic_list *list, const void *elem);

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
void generic_list_foreach(generic_list *list, iter_func func, void *data);

/**
 * @brief Analogous to the @e generic_list_foreach(), but the elements
 *        are iterated from end to start.
 * @see generic_list_foreach
 */
void generic_list_foreach_reverse(generic_list *list, iter_func func,
                                  void *data);

/**
 * Release all the resource of the @c list.
 *
 * After the call, the @list structure can be passed to the
 * @e generic_list_create().
 *
 * @param list  pointer to the list
 */
void generic_list_destroy(generic_list *list);

#endif //GENERIC_LIST_H
