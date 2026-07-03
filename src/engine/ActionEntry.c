#include "../../include/ActionEntry_internal.h"

int registerAction(ActionEntry** g_registry, const char* action, Action_f func, void* ctx) {
    if (strlen(action) > MAX_ACTION_NAME){
        fprintf(stderr, "Cannot have an action name that exceeds the limit of %d letters: %s\n", MAX_ACTION_NAME, action);
        return -1;
    }
    ActionEntry* e = malloc(sizeof(ActionEntry));
    if (!e) {
        fprintf(stderr, "Memory allocation failed for action entry\n");
        return -1;
    }
    memset(e, 0, sizeof(ActionEntry));
    strcpy(e->action, action);
    e->func = func;
    e->ctx  = ctx;
    HASH_ADD_STR(*g_registry, action, e);
    return 0;
}

ActionEntry* lookupAction(ActionEntry* g_registry, const char* action) {
    ActionEntry* e;
    HASH_FIND_STR(g_registry, action, e);
    return e;
}

void freeRegistry(ActionEntry** g_registry) {
    ActionEntry *e, *tmp;
    HASH_ITER(hh, *g_registry, e, tmp) {
        HASH_DEL(*g_registry, e);
        free(e);
    }
}

Action_f action_entry_func(const ActionEntry* e) {
    return e ? e->func : NULL;
}

void* action_entry_ctx(const ActionEntry* e) {
    return e ? e->ctx : NULL;
}
