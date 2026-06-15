#include "uthash.h"
#include "engine.h"

/* ── helpers ─────────────────────────────────────────────────────────────── */

static int passed = 0;
static int failed = 0;

/* Each action callback receives ctx = (char*)"RULE_NAME : expected to fire" */
void generic_cb(FactDB* db, void* ctx){
    (void)db;
    printf("  [CALLBACK] %s\n", (char*)ctx);
}

void printIndent(int depth){
    for (int i = 0; i < depth; i++) printf("  ");
}

void printAST(Node* n, int depth){
    if (!n) return;
    printIndent(depth);
    switch (n->type){
        case NODE_AND:
            printf("AND\n");
            printAST(n->data.op.left,  depth + 1);
            printAST(n->data.op.right, depth + 1);
            break;
        case NODE_OR:
            printf("OR\n");
            printAST(n->data.op.left,  depth + 1);
            printAST(n->data.op.right, depth + 1);
            break;
        case NODE_NOT:
            printf("NOT\n");
            printAST(n->data.unary.child, depth + 1);
            break;
        case NODE_FACT:
            printf("FACT(%s)\n", n->data.Fact.factName);
            break;
        case NODE_COMPARE:
            printf("COMPARE(%s op=%d %.2f)\n",
                n->data.Compare.factName,
                n->data.Compare.op,
                n->data.Compare.val);
            break;
    }
}

void printFactDB(FactDB* db){
    printf("=== FACT DB ===\n");
    printf("[BOOL FACTS]\n");
    BoolFact* bf, *btmp;
    HASH_ITER(hh, db->boolFacts, bf, btmp)
        printf("  %s = %s\n", bf->name, bf->val ? "true" : "false");
    printf("[NUM FACTS]\n");
    NumFact* nf, *ntmp;
    HASH_ITER(hh, db->numFacts, nf, ntmp)
        printf("  %s = %.2f\n", nf->name, nf->val);
    printf("================\n\n");
}

/* ── per-rule callbacks (each just notes which rule fired) ───────────────── */

/*
 * EXPECTED TO FIRE  (facts make condition true)
 *   rule_simple_bool_true          isAdmin=true
 *   rule_compare_gt                age(25) > 18
 *   rule_compare_lt                score(42) < 100
 *   rule_compare_ge                balance(1500) >= 1500
 *   rule_compare_le                loginAttempts(2) <= 5
 *   rule_compare_eq                itemCount(3) == 3
 *   rule_compare_float             temperature(98.6) > 98.0
 *   rule_and_two                   isAdmin && isVerified
 *   rule_or_two                    isRegular||isPremium → isPremium=true
 *   rule_not_bool                  !isBanned → true
 *   rule_and_three                 isAdmin && isVerified && isPremium
 *   rule_or_three                  isGuest||isRegular||isAdmin → isAdmin
 *   rule_and_with_compare          age>18 && balance>=1000
 *   rule_or_with_compare           score>100 is false, but isPremium=true
 *   rule_not_with_compare          !(loginAttempts>10) → true
 *   rule_nested_and_or             isAdmin && (balance>500 || isPremium)
 *   rule_nested_or_and             isBanned is false, but isVerified&&age>18
 *   rule_deep_nesting              complex → true
 *   rule_not_or                    !(isBanned||isGuest) → true
 *   rule_not_and                   !(isGuest&&isBanned) → true
 *   rule_old_account_big_balance   accountAgeDays>=300 && balance>1000
 *   rule_range_check               0<=score<=100
 *   rule_zero_discount_and_items   discount==0 && itemCount>0
 *   rule_complex_access            all conditions satisfied
 *
 * EXPECTED NOT TO FIRE
 *   rule_simple_bool_false         isBanned=false
 *   rule_compare_ne                discount(0) != 0 → false
 *   rule_not_bool_false            !isAdmin → false
 */

void cb_simple_bool_true       (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_compare_gt             (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_compare_lt             (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_compare_ge             (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_compare_le             (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_compare_eq             (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_compare_float          (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_and_two                (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_or_two                 (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_not_bool               (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_and_three              (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_or_three               (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_and_with_compare       (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_or_with_compare        (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_not_with_compare       (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_nested_and_or          (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_nested_or_and          (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_deep_nesting           (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_not_or                 (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_not_and                (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_old_account            (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_range_check            (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_zero_discount          (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }
void cb_complex_access         (FactDB* d, void* c){ (void)d; printf("  [PASS] %s\n", (char*)c); passed++; }

/* These should NEVER be called; if they are, it's a bug */
void cb_should_not_fire(FactDB* d, void* c){
    (void)d;
    printf("  [FAIL] Rule fired but should NOT have: %s\n", (char*)c);
    failed++;
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void){
    Engine* e = createMainEngine(NULL, NULL, "../src/test.json");

    printf("\n=== FACT DATABASE ===\n");
    printFactDB(e->db);

    printf("=== RULE ASTs ===\n");
    Rule* r;
    Rule* tmp2;
    HASH_ITER(hh, e->r_engine->rules, r, tmp2){
        printf("Rule: %s\n", r->ruleName);
        printAST(r->condition, 1);
        printf("\n");
    }

    linkToRule(e, "rule_simple_bool_true",      cb_simple_bool_true,    "rule_simple_bool_true: isAdmin=true");
    linkToRule(e, "rule_compare_gt",            cb_compare_gt,          "rule_compare_gt: age(25)>18");
    linkToRule(e, "rule_compare_lt",            cb_compare_lt,          "rule_compare_lt: score(42)<100");
    linkToRule(e, "rule_compare_ge",            cb_compare_ge,          "rule_compare_ge: balance(1500)>=1500");
    linkToRule(e, "rule_compare_le",            cb_compare_le,          "rule_compare_le: loginAttempts(2)<=5");
    linkToRule(e, "rule_compare_eq",            cb_compare_eq,          "rule_compare_eq: itemCount==3");
    linkToRule(e, "rule_compare_float",         cb_compare_float,       "rule_compare_float: temperature(98.6)>98.0");
    linkToRule(e, "rule_and_two",               cb_and_two,             "rule_and_two: isAdmin&&isVerified");
    linkToRule(e, "rule_or_two",                cb_or_two,              "rule_or_two: isPremium=true");
    linkToRule(e, "rule_not_bool",              cb_not_bool,            "rule_not_bool: !isBanned");
    linkToRule(e, "rule_and_three",             cb_and_three,           "rule_and_three: admin&&verified&&premium");
    linkToRule(e, "rule_or_three",              cb_or_three,            "rule_or_three: isAdmin=true");
    linkToRule(e, "rule_and_with_compare",      cb_and_with_compare,    "rule_and_with_compare: age>18&&balance>=1000");
    linkToRule(e, "rule_or_with_compare",       cb_or_with_compare,     "rule_or_with_compare: isPremium=true");
    linkToRule(e, "rule_not_with_compare",      cb_not_with_compare,    "rule_not_with_compare: !(loginAttempts>10)");
    linkToRule(e, "rule_nested_and_or",         cb_nested_and_or,       "rule_nested_and_or");
    linkToRule(e, "rule_nested_or_and",         cb_nested_or_and,       "rule_nested_or_and: verified&&age>18");
    linkToRule(e, "rule_deep_nesting",          cb_deep_nesting,        "rule_deep_nesting");
    linkToRule(e, "rule_not_or",                cb_not_or,              "rule_not_or: !(banned||guest)");
    linkToRule(e, "rule_not_and",               cb_not_and,             "rule_not_and: !(guest&&banned)");
    linkToRule(e, "rule_old_account_big_balance", cb_old_account,       "rule_old_account_big_balance");
    linkToRule(e, "rule_range_check",           cb_range_check,         "rule_range_check: 0<=score<=100");
    linkToRule(e, "rule_zero_discount_and_items", cb_zero_discount,     "rule_zero_discount_and_items");
    linkToRule(e, "rule_complex_access",        cb_complex_access,      "rule_complex_access");

    /* ── link callbacks for rules that should NOT fire ── */
    linkToRule(e, "rule_simple_bool_false",  cb_should_not_fire, "rule_simple_bool_false");
    linkToRule(e, "rule_compare_ne",         cb_should_not_fire, "rule_compare_ne");
    linkToRule(e, "rule_not_bool_false",     cb_should_not_fire, "rule_not_bool_false");

    printf("=== RUNNING ENGINE ===\n");
    runMainEngine(e);

    printf("\n=== RESULTS ===\n");
    printf("  Expected to fire   : 24\n");
    printf("  Expected NOT to fire: 3\n");
    printf("  Callbacks fired [PASS]: %d\n", passed);
    printf("  Callbacks fired [FAIL]: %d\n", failed);
    if (failed == 0 && passed == 24)
        printf("  ALL TESTS PASSED\n");
    else
        printf("  SOMETHING IS WRONG\n");

    destroyMainEngine(e);
    return failed > 0 ? 1 : 0;
}
