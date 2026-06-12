#include "rule.h"
#include "jsonParser.h"

/*
 *
 * ASSUMED JSON STRUCTURE:
 * {
  "facts": {
    "isAdmin": true,
    "age": 25
  },
  "rules": [
    {
      "name": "canAccessDashboard",
      "action": "ALLOW_DASHBOARD",
      "if": {
        "and": [
          "isAdmin",
          { ">": ["age", 18] }
        ]
      }
    }
  ]
  }
 */
bool fileExists(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;

}



yyjson_doc* parseJSON(const char * file){
    if (!fileExists(file)) {
        printf("File not found: %s\n", file);
        exit(1);
    }
    yyjson_doc * doc = yyjson_read_file(file, 0, NULL, NULL);
    if (!doc){
        printf("Invalid JSON format!\n");
        exit(1);
    }
    return doc;
}

RuleEngine* build_ast(yyjson_doc* doc, FactDB* db) {
    yyjson_val* root = yyjson_doc_get_root(doc);
    build_factdb(db, root);
    yyjson_val* rulesArr = yyjson_obj_get(root, "rules");

    RuleEngine* engine = createEngine();

    size_t idx, max;
    yyjson_val* rule;

    yyjson_arr_foreach(rulesArr, idx, max, rule){
        yyjson_val* name = yyjson_obj_get(rule, "name");
        yyjson_val* action = yyjson_obj_get(rule, "action");
        yyjson_val* cond = yyjson_obj_get(rule, "if");
Rule r;
        r.ruleName = strdup(yyjson_get_str(name));
        r.action = strdup(yyjson_get_str(action));
        r.condition = build_node(cond);

        engine->rules = realloc(
            engine->rules,
            sizeof(Rule) * (engine->ruleCount + 1)
        );

        engine->rules[engine->ruleCount++] = r;
    }
    yyjson_doc_free(doc);
    return engine;
}

Node* build_node(yyjson_val* v){
    // FACT STRING
    if (yyjson_is_str(v))
        return build_fact(v);
    if (!yyjson_is_obj(v))
        return NULL;

    yyjson_obj_iter iter;
    yyjson_obj_iter_init(v, &iter);
    yyjson_val* key;
    while ((key = yyjson_obj_iter_next(&iter))){
        const char* op = yyjson_get_str(key);
        yyjson_val* val = yyjson_obj_iter_get_val(key);

        if (strcmp(op, "and") == 0)
            return build_and(val);

        if (strcmp(op, "or") == 0)
            return build_or(val);

        if (strcmp(op, "not") == 0)
            return build_not(val);

        if (strcmp(op, ">") == 0 ||
            strcmp(op, "<") == 0 ||
            strcmp(op, ">=") == 0 ||
            strcmp(op, "<=") == 0 ||
            strcmp(op, "==") == 0 ||
            strcmp(op, "!=") == 0)
        {
            return build_compare(op, val);
        }
    }
    return NULL;
}
Node* build_fact(yyjson_val* v){
    Node* n = createNode(NODE_FACT);
    n->data.Fact.factName = strdup(yyjson_get_str(v));
    return n;
}

Node* build_compare(const char* op, yyjson_val* arr){
    Node* n = createNode(NODE_COMPARE);
 
    yyjson_val* left = yyjson_arr_get(arr, 0);
    yyjson_val* right = yyjson_arr_get(arr, 1);
    n->data.Compare.factName = strdup(yyjson_get_str(left));

    if (yyjson_is_int(right))
        n->data.Compare.val = yyjson_get_int(right);
    if (yyjson_is_real(right))
        n->data.Compare.val = yyjson_get_real(right);
    if (n->data.Compare.val == NAN)
        printf("Invalid comparison value for fact '%s'\n", n->data.Compare.factName);
    if (strcmp(op, ">") == 0) n->data.Compare.op = OP_GT;
    else if (strcmp(op, "<") == 0) n->data.Compare.op = OP_LT;
    else if (strcmp(op, ">=") == 0) n->data.Compare.op = OP_GE;
    else if (strcmp(op, "<=") == 0) n->data.Compare.op = OP_LE;
    else if (strcmp(op, "==") == 0) n->data.Compare.op = OP_EQ;
    else if (strcmp(op, "!=") == 0) n->data.Compare.op = OP_NE;
 
    return n;
}

Node* build_and(yyjson_val* arr){
    size_t len = yyjson_arr_size(arr);

    Node* left = build_node(yyjson_arr_get(arr, 0));
    for (size_t i = 1; i < len; i++){
        Node* right = build_node(yyjson_arr_get(arr, i));
        Node* parent = createNode(NODE_AND);
        parent->data.op.left = left;
        parent->data.op.right = right;
        left = parent;
    }
    return left;
}

Node* build_or(yyjson_val* arr){
    size_t len = yyjson_arr_size(arr);
    Node* left = build_node(yyjson_arr_get(arr, 0));
    for (size_t i = 1; i < len; i++){
        Node* right = build_node(yyjson_arr_get(arr, i));
        Node* parent = createNode(NODE_OR);
        parent->data.op.left = left;
        parent->data.op.right = right;
        left = parent;
    }
    return left;
}

Node* build_not(yyjson_val* v){
    Node* n = createNode(NODE_NOT);
    n->data.unary.child = build_node(v);
    return n;
}

void build_factdb(FactDB* db, yyjson_val* root)
{
    yyjson_val* facts = yyjson_obj_get(root, "facts");

    if (!facts || !yyjson_is_obj(facts))
        return;

    yyjson_obj_iter iter;
    yyjson_obj_iter_init(facts, &iter);
    yyjson_val *key, *val;

    while ((key = yyjson_obj_iter_next(&iter))) {
        const char* name = yyjson_get_str(key);
        val = yyjson_obj_iter_get_val(key);

        if (yyjson_is_bool(val)) {
            setBoolFact(db, name, yyjson_get_bool(val));
        }
        else if (yyjson_is_int(val)){
            setNumFact(db, name, (double)yyjson_get_int(val));
        }
    }
}