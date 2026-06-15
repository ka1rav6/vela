#include "engine.h"
#include "jsonParser.h"
Engine* createMainEngine(FactDB* db, RuleEngine* e, const char* json_file){
    Engine* temp = (Engine*)malloc(sizeof(Engine));
    if (!temp){
        fprintf(stderr, "The engine memory could not be allocated\n");
        perror("");
        exit(EXIT_FAILURE);
    }
    memset(temp, 0, sizeof(Engine));
    if (e != NULL)
        temp->r_engine = e;
    if (db != NULL)
        temp->db = db;
    if (json_file == NULL){
        fprintf(stderr, "ERROR: the engine cannot be created with a NULL json file\n");
        perror("");
        exit(EXIT_FAILURE);
    }
    temp->json_file = json_file;
    temp->r_engine = build_ast(parseJSON(json_file), temp->db);
    return temp;
}


void destroyMainEngine(Engine* e){
    deleteFactDB(e->db);
    deleteEngine(e->r_engine);
    free(e);
    e = NULL;
}

void linkToRule(Engine* e, const char* name, Action_f f, void* ctx){
    Rule* r = findRule(e->r_engine, name);
    r->func = f;
    r->ctx = ctx;
} // name = rule name

void runMainEngine(Engine* e){
    run(e->r_engine, e->db);
}
