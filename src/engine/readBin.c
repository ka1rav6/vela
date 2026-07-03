#include "../../include/rule_internal.h"
#include "../../include/bytecode.h"

#define VELABC_MAGIC    0x524C4542
#define VELABC_VERSION  3
#define MAX_RULE_NAME   64

static uint32_t read_u32le(const uint8_t* buf)
{
    return (uint32_t)buf[0]
         | ((uint32_t)buf[1] << 8)
         | ((uint32_t)buf[2] << 16)
         | ((uint32_t)buf[3] << 24);
}

RuleEngine* loadBytecode(const char* file, FactDB* db)
{
    (void)db;

    FILE* fp = fopen(file, "rb");
    if (!fp)
    {
        fprintf(stderr, "Could not open file: %s\n", file);
        return NULL;
    }

    uint8_t header[16];
    if (fread(header, 1, 16, fp) != 16)
    {
        fprintf(stderr, "Could not read header from: %s\n", file);
        fclose(fp);
        return NULL;
    }

    uint32_t magic      = read_u32le(header);
    uint32_t version    = read_u32le(header + 4);
    uint32_t instrCount = read_u32le(header + 8);
    uint32_t ruleCount  = read_u32le(header + 12);

    if (magic != VELABC_MAGIC)
    {
        fprintf(stderr, "Invalid magic in %s (expected 0x%08X, got 0x%08X)\n",
                file, VELABC_MAGIC, magic);
        fclose(fp);
        return NULL;
    }
    if (version != VELABC_VERSION)
    {
        fprintf(stderr, "Unsupported version %d in %s (expected %d)\n",
                version, file, VELABC_VERSION);
        fclose(fp);
        return NULL;
    }

    RuleEngine* engine = createRuleEngine();
    if (!engine)
    {
        fclose(fp);
        return NULL;
    }

    for (uint32_t r = 0; r < ruleCount; r++)
    {
        uint8_t nameLen;
        if (fread(&nameLen, 1, 1, fp) != 1)
        {
            fprintf(stderr, "Could not read rule name length\n");
            fclose(fp);
            deleteRuleEngine(engine);
            return NULL;
        }
        if (nameLen >= MAX_RULE_NAME)
        {
            fprintf(stderr, "Rule name too long (%u bytes)\n", nameLen);
            fclose(fp);
            deleteRuleEngine(engine);
            return NULL;
        }

        char ruleName[MAX_RULE_NAME];
        if (nameLen > 0 && fread(ruleName, 1, nameLen, fp) != nameLen)
        {
            fprintf(stderr, "Could not read rule name\n");
            fclose(fp);
            deleteRuleEngine(engine);
            return NULL;
        }
        ruleName[nameLen] = '\0';

        uint8_t actionLen;
        if (fread(&actionLen, 1, 1, fp) != 1)
        {
            fprintf(stderr, "Could not read action name length for rule: %s\n", ruleName);
            fclose(fp);
            deleteRuleEngine(engine);
            return NULL;
        }
        if (actionLen >= MAX_RULE_NAME)
        {
            fprintf(stderr, "Action name too long (%u bytes) for rule: %s\n", actionLen, ruleName);
            fclose(fp);
            deleteRuleEngine(engine);
            return NULL;
        }

        char actionName[MAX_RULE_NAME];
        if (actionLen > 0 && fread(actionName, 1, actionLen, fp) != actionLen)
        {
            fprintf(stderr, "Could not read action name for rule: %s\n", ruleName);
            fclose(fp);
            deleteRuleEngine(engine);
            return NULL;
        }
        actionName[actionLen] = '\0';

        Bytecode* bc = arena_alloc(engine->arena, sizeof(Bytecode));
        if (!bc)
        {
            fprintf(stderr, "Memory allocation failed for bytecode\n");
            fclose(fp);
            deleteRuleEngine(engine);
            return NULL;
        }
        bc->capacity = 8;
        bc->count    = 0;
        bc->code     = arena_alloc(engine->arena, sizeof(Instr) * bc->capacity);
        if (!bc->code)
        {
            fprintf(stderr, "Memory allocation failed for bytecode instructions\n");
            fclose(fp);
            deleteRuleEngine(engine);
            return NULL;
        }

        while (1)
        {
            uint8_t opcode;
            if (fread(&opcode, 1, 1, fp) != 1)
            {
                fprintf(stderr, "Unexpected EOF while reading instructions\n");
                fclose(fp);
                deleteRuleEngine(engine);
                return NULL;
            }

            if (bc->count >= bc->capacity)
            {
                int newCap   = bc->capacity * 2;
                Instr* grown = arena_alloc(engine->arena, sizeof(Instr) * newCap);
                if (!grown)
                {
                    fprintf(stderr, "Memory allocation failed for bytecode growth\n");
                    fclose(fp);
                    deleteRuleEngine(engine);
                    return NULL;
                }
                memcpy(grown, bc->code, sizeof(Instr) * bc->count);
                bc->code     = grown;
                bc->capacity = newCap;
            }

            Instr* instr = &bc->code[bc->count++];
            memset(instr, 0, sizeof(Instr));
            instr->op = (OpCode)opcode;

            if (opcode == OP_HALT)
                break;

            if (opcode == OP_PUSH_FACT || opcode == OP_PUSH_CMP)
            {
                uint8_t cmp;
                if (fread(&cmp, 1, 1, fp) != 1)
                {
                    fprintf(stderr, "Could not read compare op\n");
                    fclose(fp);
                    deleteRuleEngine(engine);
                    return NULL;
                }
                instr->cmp = (CompareOp)cmp;

                char nameBuf[256];
                size_t i = 0;
                while (1)
                {
                    char ch;
                    if (fread(&ch, 1, 1, fp) != 1)
                    {
                        fprintf(stderr, "Could not read fact name\n");
                        fclose(fp);
                        deleteRuleEngine(engine);
                        return NULL;
                    }
                    if (ch == '\0')
                        break;
                    if (i >= sizeof(nameBuf) - 1)
                    {
                        fprintf(stderr, "Fact name too long\n");
                        fclose(fp);
                        deleteRuleEngine(engine);
                        return NULL;
                    }
                    nameBuf[i++] = ch;
                }
                nameBuf[i] = '\0';
                instr->factName = arena_strdup(engine->arena, nameBuf);

                uint8_t valBytes[8];
                if (fread(valBytes, 1, 8, fp) != 8)
                {
                    fprintf(stderr, "Could not read double value\n");
                    fclose(fp);
                    deleteRuleEngine(engine);
                    return NULL;
                }
                memcpy(&instr->val, valBytes, sizeof(double));
            }
        }

        Rule* rule = arena_alloc(engine->arena, sizeof(Rule));
        if (!rule)
        {
            fprintf(stderr, "Memory allocation failed for rule\n");
            fclose(fp);
            deleteRuleEngine(engine);
            return NULL;
        }
        memset(rule, 0, sizeof(Rule));
        strcpy(rule->ruleName, ruleName);
        rule->action    = arena_strdup(engine->arena, actionName);
        rule->bc        = bc;
        rule->condition = NULL;
        addRule(engine, rule);
    }

    fclose(fp);

    uint32_t readInstrs = 0;
    for (Rule *cr = engine->rules, *tmp; cr; cr = tmp)
    {
        tmp = cr->hh.next;
        readInstrs += cr->bc->count;
    }
    if (readInstrs != instrCount)
    {
        fprintf(stderr, "Instruction count mismatch: header says %u, read %u\n",
                instrCount, readInstrs);
        deleteRuleEngine(engine);
        return NULL;
    }

    return engine;
}
