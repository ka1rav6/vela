#include "../../include/engine_internal.h"

// Engine constructor. To be called by the user.
Engine* createEngine(const char* file, FileType type){
    Engine* temp = (Engine*)malloc(sizeof(Engine));

    if (!temp){
        fprintf(stderr, "The engine memory could not be allocated\n");
        return NULL;
    }
    memset(temp, 0, sizeof(Engine));

    if (file == NULL){
        fprintf(stderr, "ERROR: the engine cannot be created with a NULL json file\n");
        free(temp);
        return NULL;
    }
    temp->file      = file;
    temp->db        = createFactDB();
    if (!temp->db) {
        free(temp);
        return NULL;
    }
    if (type == JSON) {
        yyjson_doc* doc = parseJSON(file);
        if (!doc) {
            deleteFactDB(temp->db);
            free(temp);
            return NULL;
        }
        temp->r_engine = build_ast(doc, temp->db, temp->action_registry);
    } else {
        temp->r_engine = loadBytecode(file, temp->db);
    }
    if (!temp->r_engine) {
        deleteFactDB(temp->db);
        free(temp);
        return NULL;
    }
    if (pthread_mutex_init(&temp->lock, NULL) != 0) {
        fprintf(stderr, "Could not initialize Engine mutex\n");
        deleteFactDB(temp->db);
        deleteRuleEngine(temp->r_engine);
        free(temp);
        return NULL;
    }
    return temp;
}

// engine destructor. the arena is also deleted through the rule engine
void deleteEngine(Engine* e){
    deleteFactDB(e->db);
    deleteRuleEngine(e->r_engine);
    freeRegistry(&e->action_registry);
    pthread_mutex_destroy(&e->lock);
    free(e);
}

// to register the action in the action registry.
int registerTheAction(Engine* e, const char* name, Action_f f, void* ctx){
    if (!e || !name || !f) return -1;
    int ret = 0;
    pthread_mutex_lock(&e->lock);
    if (registerAction(&e->action_registry, name, f, ctx) != 0) ret = -1;
    rule_engine_bind_action(e->r_engine, name, f, ctx);
    pthread_mutex_unlock(&e->lock);
    return ret;
}
// just internally runs the rule engine
int runEngine(Engine* e){
    if (!e) return -1;
    runRuleEngine(e->r_engine, e->db);
    return 0;
}

FactDB* engine_get_factdb(Engine* e) {
    return e ? e->db : NULL;
}
RuleEngine* engine_get_rule_engine(Engine* e) {
    return e ? e->r_engine : NULL;
}
