#include "../../include/rule_internal.h"
#include "../../include/bytecode.h"

#define VELABC_MAGIC    0x524C4542
#define VELABC_VERSION  3
#define MAX_RULE_NAME   64

static uint32_t read_u32le(const uint8_t* buf) {
    return (uint32_t)buf[0]
         | ((uint32_t)buf[1] << 8)
         | ((uint32_t)buf[2] << 16)
         | ((uint32_t)buf[3] << 24);
}

RuleEngine* loadBytecode(const char* file, FactDB* db) {
    (void)db;

    FILE* fp = fopen(file, "rb");
    if (!fp)
        FATAL("Could not open file: %s\n", file);

    uint8_t header[16];
    if (fread(header, 1, 16, fp) != 16)
        FATAL("Could not read header from: %s\n", file);

    uint32_t magic      = read_u32le(header);
    uint32_t version    = read_u32le(header + 4);
    uint32_t instrCount = read_u32le(header + 8);
    uint32_t ruleCount  = read_u32le(header + 12);

    if (magic != VELABC_MAGIC)
        FATAL("Invalid magic in %s (expected 0x%08X, got 0x%08X)\n",
              file, VELABC_MAGIC, magic);
    if (version != VELABC_VERSION)
        FATAL("Unsupported version %d in %s (expected %d)\n",
              version, file, VELABC_VERSION);

    RuleEngine* engine = createRuleEngine();

    for (uint32_t r = 0; r < ruleCount; r++) {
        uint8_t nameLen;
        if (fread(&nameLen, 1, 1, fp) != 1)
            FATAL("Could not read rule name length\n");
        if (nameLen >= MAX_RULE_NAME)
            FATAL("Rule name too long (%u bytes)\n", nameLen);

        char ruleName[MAX_RULE_NAME];
        if (nameLen > 0 && fread(ruleName, 1, nameLen, fp) != nameLen)
            FATAL("Could not read rule name\n");
        ruleName[nameLen] = '\0';

        Bytecode* bc = arena_alloc(engine->arena, sizeof(Bytecode));
        bc->capacity = 8;
        bc->count    = 0;
        bc->code     = arena_alloc(engine->arena, sizeof(Instr) * bc->capacity);

        while (1) {
            uint8_t opcode;
            if (fread(&opcode, 1, 1, fp) != 1)
                FATAL("Unexpected EOF while reading instructions\n");

            if (bc->count >= bc->capacity) {
                int newCap   = bc->capacity * 2;
                Instr* grown = arena_alloc(engine->arena, sizeof(Instr) * newCap);
                memcpy(grown, bc->code, sizeof(Instr) * bc->count);
                bc->code     = grown;
                bc->capacity = newCap;
            }

            Instr* instr = &bc->code[bc->count++];
            memset(instr, 0, sizeof(Instr));
            instr->op = (OpCode)opcode;

            if (opcode == OP_HALT)
                break;

            if (opcode == OP_PUSH_FACT || opcode == OP_PUSH_CMP) {
                uint8_t cmp;
                if (fread(&cmp, 1, 1, fp) != 1)
                    FATAL("Could not read compare op\n");
                instr->cmp = (CompareOp)cmp;

                char nameBuf[256];
                size_t i = 0;
                while (1) {
                    char ch;
                    if (fread(&ch, 1, 1, fp) != 1)
                        FATAL("Could not read fact name\n");
                    if (ch == '\0')
                        break;
                    if (i >= sizeof(nameBuf) - 1)
                        FATAL("Fact name too long\n");
                    nameBuf[i++] = ch;
                }
                nameBuf[i] = '\0';
                instr->factName = arena_strdup(engine->arena, nameBuf);

                uint8_t valBytes[8];
                if (fread(valBytes, 1, 8, fp) != 8)
                    FATAL("Could not read double value\n");
                memcpy(&instr->val, valBytes, sizeof(double));
            }
        }

        Rule* rule = arena_alloc(engine->arena, sizeof(Rule));
        memset(rule, 0, sizeof(Rule));
        strcpy(rule->ruleName, ruleName);
        rule->action    = arena_strdup(engine->arena, ruleName);
        rule->bc        = bc;
        rule->condition = NULL;
        addRule(engine, rule);
    }

    fclose(fp);

    uint32_t readInstrs = 0;
    for (Rule *cr = engine->rules, *tmp; cr; cr = tmp) {
        tmp = cr->hh.next;
        readInstrs += cr->bc->count;
    }
    if (readInstrs != instrCount)
        FATAL("Instruction count mismatch: header says %u, read %u\n",
              instrCount, readInstrs);

    return engine;
}
