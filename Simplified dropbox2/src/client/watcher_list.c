#include "watcher_list.h"

#include "utils.h"

#include <string.h>

bool client_watcher_list_create(client_watcher_list *list)
{
        return generic_list_create(list, sizeof(struct client_watcher));
}

bool client_watcher_list_push_back(client_watcher_list *list,
                                   const struct client_watcher *elem)
{
        return generic_list_push_back(list, elem);
}

void client_watcher_list_foreach(client_watcher_list *list, iter_func func,
                                 void *data)
{
        generic_list_foreach(list, func, data);
}

void client_watcher_list_foreach_reverse(client_watcher_list *list,
                                         iter_func func, void *data)
{
        generic_list_foreach_reverse(list, func, data);
}

struct iter_find_watcher_data {
        const struct client_watcher needle;
        struct client_watcher *found_watcher;
};

static bool iter_find_watcher(void *elem, void *data)
{
        struct client_watcher *w = elem;
        struct iter_find_watcher_data *find_data = data;

        bool wd_equal = true;
        bool name_equal = true;

        if (find_data->needle.wd != -1 && find_data->needle.wd != w->wd)
                wd_equal = false;

        if (find_data->needle.name != NULL
            && strcmp(find_data->needle.name, w->name) != 0)
                name_equal = false;

        bool found = wd_equal && name_equal;

        if (found)
                find_data->found_watcher = w;

        return !found;
}

struct client_watcher *
client_watcher_list_find_last(client_watcher_list *list,
                              struct client_watcher needle)
{
        struct iter_find_watcher_data data = {
                .needle = needle,
                .found_watcher = NULL,
        };
        client_watcher_list_foreach_reverse(list, iter_find_watcher, &data);

        return data.found_watcher;
}

static bool iter_free_watcher_name(void *elem, void *data)
{
        UNUSED(data);
        const struct client_watcher *w = elem;
        free((void *)w->name);
        return true;
}

void client_watcher_list_destroy(client_watcher_list *list)
{
        client_watcher_list_foreach(list, iter_free_watcher_name, NULL);
        generic_list_destroy(list);
}
