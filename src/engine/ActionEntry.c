#include "ActionEntry_internal.h"

int registerAction(ActionEntry** registry, const char* action, Action_f func, void* ctx)
{
    if (strlen(action) > MAX_ACTION_NAME)
        return -1;
    ActionEntry* e = malloc(sizeof(ActionEntry));
    if (!e)
        return -1;
    memset(e, 0, sizeof(ActionEntry));
    strncpy(e->action, action, MAX_ACTION_NAME - 1);
    e->action[MAX_ACTION_NAME - 1] = '\0';
    e->func = func;
    e->ctx  = ctx;
    HASH_ADD_STR(*registry, action, e);
    return 0;
}

// searches through the hashmapped registry to find the action based on the name
ActionEntry* lookupAction(ActionEntry* registry, const char* action)
{
    ActionEntry* e;
    HASH_FIND_STR(registry, action, e);
    return e;
}

// frees the actionEntry registry
void freeRegistry(ActionEntry** registry)
{
    ActionEntry *e, *tmp;
    HASH_ITER(hh, *registry, e, tmp) {
        HASH_DEL(*registry, e);
        free(e);
    }
}

// returns the function of the action entry if it exists
Action_f action_entry_func(const ActionEntry* e)
{
    return e ? e->func : NULL;
}

// returns the context of the function of the action entry if it exists
void* action_entry_ctx(const ActionEntry* e)
{
    return e ? e->ctx : NULL;
}
