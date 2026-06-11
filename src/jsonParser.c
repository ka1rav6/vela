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




yyjson_doc* parseJSON(const char * file){
    yyjson_doc * doc = yyjson_read_file(file, 0, NULL, NULL);
    if (!doc){
        printf("Invalid JSON format!\n");
        exit(1);
    }
    return doc;
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

