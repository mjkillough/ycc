#include <stdio.h>
#include <string.h>

#include "framework.h"
#include "ident.h"

TEST(add_same_twice) {
    struct ident_table *t = ident_table_new();

    ASSERT(ident_table_len(t) == 0);

    struct ident *one1 = ident_from_str(t, "one");

    ASSERT(ident_table_len(t) == 1);

    struct ident *two = ident_from_str(t, "two");

    ASSERT(ident_table_len(t) == 2);

    struct ident *one2 = ident_from_str(t, "one");

    ASSERT(ident_table_len(t) == 2);

    ASSERT(strcmp("one", ident_to_str(one1)) == 0);
    ASSERT(strcmp("one", ident_to_str(one2)) == 0);
    ASSERT(strcmp("two", ident_to_str(two)) == 0);

    // one1 and one2 should be same ptr as we've interned
    ASSERT(ident_to_str(one1) == ident_to_str(one2));

    ident_table_free(t);
}
