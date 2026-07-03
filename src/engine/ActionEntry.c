#include "../../include/ActionEntry_internal.h"

// registers the action in the registry
int registerAction(ActionEntry** registry, const char* action, Action_f func, void* ctx)
{
    if (strlen(action) > MAX_ACTION_NAME)
    {
        fprintf(stderr, "Cannot have an action name that exceeds the limit of %d letters: %s\n", MAX_ACTION_NAME, action);
        return -1;
    }
    ActionEntry* e = malloc(sizeof(ActionEntry));
    if (!e)
    {
        fprintf(stderr, "Memory allocation failed for action entry\n");
        return -1; // the error code
    }
    memset(e, 0, sizeof(ActionEntry));
    strcpy(e->action, action);
    e->func = func;
    e->ctx  = ctx;
    HASH_ADD_STR(*registry, action, e); // uthash macro to add the action to the registry hashmap
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
