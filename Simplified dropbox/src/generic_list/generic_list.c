#include "generic_list.h"

#include "log.h"

#include <string.h>
#include <unistd.h>

static inline size_t min(size_t l, size_t r)
{
        return l <= r ? l : r;
}

static const size_t DEFAULT_CREATE_SIZE = 2;

// Value determined at runtime
static volatile size_t PAGE_SIZE = 0;

static bool is_correct(const generic_list *list)
{
        // clang-format off
        return list->_array != NULL
                && list->_elem_size > 0
                && list->_size > 0
                && list->_used <= list->_size;
        // clang-format on
}

static void set_page_size()
{
        if (PAGE_SIZE != 0)
                return;

        const long page_size = sysconf(_SC_PAGE_SIZE);
        log_assert(page_size > 0);
        PAGE_SIZE = page_size;
}

bool generic_list_create(generic_list *list, size_t elem_size)
{
        log_assert(elem_size > 0);
        set_page_size();
        const size_t size = DEFAULT_CREATE_SIZE * elem_size;
        unsigned char *array = malloc(size);
        if (array == NULL)
                return false;

        list->_array = array;
        list->_elem_size = elem_size;
        list->_size = size;
        list->_used = 0;

        return list;
}

static bool expand(generic_list *list)
{
        list->_size += min(list->_size, PAGE_SIZE) * list->_elem_size;
        log_assert(list->_size > list->_used);

        unsigned char *tmp = realloc(list->_array, list->_size);
        bool result = tmp != NULL;

        list->_array = result ? tmp : list->_array;
        return result;
}

static void push_back(generic_list *list, const void *elem)
{
        memcpy(&list->_array[list->_used], elem, list->_elem_size);
        list->_used += list->_elem_size;
}

bool generic_list_push_back(generic_list *list, const void *elem)
{
        log_assert(list != NULL);
        log_assert(is_correct(list));
        log_assert(elem != NULL);

        if (list->_used + list->_elem_size > list->_size && !expand(list))
                return false;

        push_back(list, elem);
        return true;
}

static void _generic_list_foreach(generic_list *list, iter_func func,
                                  void *data, bool reverse)
{
        log_assert(list != NULL);
        log_assert(is_correct(list));
        log_assert(func != NULL);

        ssize_t start = reverse ? list->_used : 0;
        ssize_t end = reverse ? 0 : list->_used;

        ssize_t step = list->_elem_size;
        if (reverse) {
                step = -step;
                start -= (ssize_t)list->_elem_size;
                end -= (ssize_t)list->_elem_size;
        }

        for (ssize_t i = start; i != end; i += step) {
                if (!func(&list->_array[i], data))
                        return;
        }
}

void generic_list_foreach(generic_list *list, iter_func func, void *data)
{
        _generic_list_foreach(list, func, data, false);
}

void generic_list_foreach_reverse(generic_list *list, iter_func func,
                                  void *data)
{
        _generic_list_foreach(list, func, data, true);
}

void generic_list_destroy(generic_list *list)
{
        log_assert(list != NULL);
        log_assert(is_correct(list));
        free(list->_array);
        list->_elem_size = 0;
        list->_array = NULL;
        list->_size = 0;
        list->_used = 0;
}
