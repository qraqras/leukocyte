/* Threaded test for per-file rules cache concurrency */
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "rules/rule.h"
#include "rule_registry.h"
#include "rules/rule_manager.h"
#include "configs/config.h"

typedef struct
{
    const config_t *cfg;
    const char *file;
    const rules_by_type_t *result;
} thread_arg_t;

static void *thread_func(void *v)
{
    thread_arg_t *a = v;
    a->result = get_rules_by_type_for_file(a->cfg, a->file);
    return NULL;
}

int main(void)
{
    config_t cfg = {0};
    initialize_config(&cfg);
    init_rules();

    const char *file = "tests/bench/bench_5000.rb";

    const int N = 8;
    pthread_t th[N];
    thread_arg_t args[N];

    for (int i = 0; i < N; i++)
    {
        args[i].cfg = &cfg;
        args[i].file = file;
        args[i].result = NULL;
        pthread_create(&th[i], NULL, thread_func, &args[i]);
    }

    for (int i = 0; i < N; i++)
    {
        pthread_join(th[i], NULL);
        assert(args[i].result != NULL);
    }

    /* All should observe the same cached pointer */
    const rules_by_type_t *first = args[0].result;
    for (int i = 1; i < N; i++)
    {
        assert(first == args[i].result);
    }

    rule_manager_clear_cache();
    free_config(&cfg);
    return 0;
}
