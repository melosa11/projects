#define CUT_MAIN

#include "cut.h"

#include "generic_list.h"

struct int_equal_data {
        int *array;
        int curr;
};

static bool iter_equal_reverse(void *elem, void *data)
{
        int *num = elem;
        struct int_equal_data *ed = data;

        if (*num != ed->array[ed->curr]) {
                return false;
        }
        ed->curr--;
        return true;
}

static bool iter_equal(void *elem, void *data)
{
        int *num = elem;
        struct int_equal_data *ed = data;

        if (*num != ed->array[ed->curr]) {
                return false;
        }
        ed->curr++;
        return true;
}

static void test_push_back_int(generic_list *list, int count)
{
        ASSERT(count > 0);
        int *expected = malloc(count * sizeof(int));
        ASSERT(expected != NULL);
        for (int i = 0; i < count; i++) {
                expected[i] = i;
                ASSERT(generic_list_push_back(list, &i));
        }

        struct int_equal_data ed = {
                .array = expected,
                .curr = 0,
        };
        generic_list_foreach(list, iter_equal, &ed);
        ASSERT(ed.curr == count);

        ed.curr = count - 1;
        generic_list_foreach_reverse(list, iter_equal_reverse, &ed);
        ASSERT(ed.curr == -1);

        free(expected);
}

TEST(generic_list_push_back_int)
{
        generic_list list;
        ASSERT(generic_list_create(&list, sizeof(int)));

        SUBTEST(one_elem)
        {
                test_push_back_int(&list, 1);
        }

        SUBTEST(multiple_elems)
        {
                test_push_back_int(&list, 5);
        }

        generic_list_destroy(&list);
}
