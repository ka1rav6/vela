# ⚡ Vela — A High-Performance Embeddable Rule Engine in C

> **Compile once, evaluate at bitmask speed. Zero interpreter overhead at runtime.**

Vela is a **compiled** rule engine, not an interpreted one. Your rules — written as JSON or in the Vela DSL — are parsed into an AST, compiled to a flat bytecode instruction sequence, and executed by a tiny stack-machine VM. The result: rule evaluation that's faster than tree-walking engines and simpler to embed than full scripting languages.

---

## 🚀 Quick Start

```c
#include "engine.h"

void on_grant_access(FactDB* db, void* ctx) {
    printf("Access granted: %s\n", (char*)ctx);
}

int main(void) {
    Engine* e = createEngine("rules.json", JSON);

    registerTheAction(e, "GRANT_ACCESS", on_grant_access, "user-42");
    runEngine(e);                 // evaluates all rules, fires matching actions
    deleteEngine(e);
    return 0;
}
```

```bash
mkdir build && cd build
cmake .. && make
./engine
```

---

## 📋 Table of Contents

- [Why Vela?](#-why-vela)
- [Architecture Deep Dive](#-architecture-deep-dive)
- [The Pipeline: JSON → AST → Bytecode → Result](#-the-pipeline-json--ast--bytecode--result)
- [Rule Format](#-rule-format)
- [Bytecode VM Design](#-bytecode-vm-design)
- [Memory Model: Arena Allocator](#-memory-model-arena-allocator)
- [Thread Safety](#-thread-safety)
- [Action Registry](#-action-registry)
- [Vela DSL (Custom Language)](#-vela-dsl-custom-language)
- [Performance Characteristics](#-performance-characteristics)
- [Build System & Dependencies](#-build-system--dependencies)
- [Roadmap](#-roadmap)
- [FAQ](#-faq)

---

## 🎯 Why Vela?

| Problem | How Vela Solves It |
|---------|-------------------|
| **Tree-walking is slow** — recursive eval on every rule check | Compile AST → flat bytecode once; evaluation is a tight `O(n)` array walk |
| **malloc/free per node** — fragmentation, cache misses | Single `mmap` arena for all rules, AST, bytecode |
| **Boolean facts checked by string lookup** | Intended: bit-encoded bools with O(1) shift-and-mask |
| **Actions coupled to engine** | Action registry: JSON names actions, C code defines callbacks |
| **No safety net for malformed rules** | Semantic checker catches type errors, missing facts, duplicates at parse time |
| **Thread safety is an afterthought** | Per-component locks (mutex + rwlock) from day one |

Vela is built for **latency-sensitive** environments: game servers, real-time fraud detection, IoT edge devices, and any system where evaluating 100+ rules per millisecond matters.

---

## 🏗️ Architecture Deep Dive

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Engine                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  FactDB (rwlock-protected)                                   │   │
│  │  ┌────────────────────────────────────────────────────────┐  │   │
│  │  │  BoolFacts (uthash)    NumFacts (uthash)               │  │   │
│  │  │  ┌─────┐ ┌─────┐       ┌─────┐ ┌─────┐                │  │   │
│  │  │  │admin│ │guest│       │ age │ │ bal │                │  │   │
│  │  │  └─────┘ └─────┘       └─────┘ └─────┘                │  │   │
│  │  └────────────────────────────────────────────────────────┘  │   │
│  │                                                               │   │
│  │  RuleEngine (mutex-protected)                                 │   │
│  │  ┌────────────────────────────────────────────────────────┐  │   │
│  │  │  Rules (uthash, keyed by name)                        │  │   │
│  │  │  ┌──────────────────────────────────────────┐         │  │   │
│  │  │  │ Rule "admin_check"                        │         │  │   │
│  │  │  │   condition  ───► Node* (AST, arena)      │         │  │   │
│  │  │  │   bytecode   ───► Instr[]  (arena)        │         │  │   │
│  │  │  │   action: "GRANT_ACCESS"                  │         │  │   │
│  │  │  │   func:  ───► on_grant_access()           │         │  │   │
│  │  │  └──────────────────────────────────────────┘         │  │   │
│  │  │  ┌──────────────────────────────────────────┐         │  │   │
│  │  │  │ Rule "age_check"                          │         │  │   │
│  │  │  │   ...                                     │         │  │   │
│  │  │  └──────────────────────────────────────────┘         │  │   │
│  │  └────────────────────────────────────────────────────────┘  │   │
│  │                                                               │   │
│  │  Arena (mmap, 1MB)                                            │   │
│  │  ┌────────────────────────────────────────────────────────┐  │   │
│  │  │  [Node*] [Node*] [Instr[]] [Instr[]] [char*] [char*]  │  │   │
│  │  │  └──── bump pointer ────►                              │  │   │
│  │  └────────────────────────────────────────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

### Component Breakdown

| Component | Files | Role |
|-----------|-------|------|
| **`Engine`** | `engine.c/h` | Top-level facade — creates FactDB, parses file, wires actions, runs rules |
| **`FactDB`** | `factdb.c/h` | Thread-safe fact storage: uthash for bools and numerics, rwlock-protected |
| **`RuleEngine`** | `rule.c/h` | Rule hash table, iteration, action binding, visitor pattern |
| **`ConditionTree`** | `ConditionTree.c/h` | AST node types (`AND`, `OR`, `NOT`, `FACT`, `COMPARE`) |
| **`Bytecode`** | `bytecode.c/h` | AST → flat instruction compiler + stack-machine VM |
| **`Parser`** | `parser_engine.c/h` | JSON reader (yyjson) → AST builder |
| **`SemanticChecker`** | `semanticChecker.c/h` | Validation: operators, types, existence, duplicates |
| **`ActionEntry`** | `ActionEntry.c/h` | String → function pointer registry |
| **`Arena`** | `arena.c/h` | mmap-backed bump allocator for all rule-engine memory |
| **`readBin`** | `readBin.c` | Binary `.velabc` bytecode loader |
| **`uthash`** | `uthash.h` | Vendored hash-table macros (public domain) |

---

## 🔄 The Pipeline: JSON → AST → Bytecode → Result

### Step 1: Parse

```json
{
  "facts": {
    "isAdmin": true,
    "age": 25,
    "balance": 1500.0
  },
  "rules": [
    {
      "name": "admin_with_balance",
      "action": "GRANT_ACCESS",
      "if": {
        "and": [
          "isAdmin",
          { ">": ["age", 18] },
          { ">=": ["balance", 1000] }
        ]
      }
    }
  ]
}
```

`parser_engine.c` reads the JSON with [yyjson](https://github.com/ibireme/yyjson) — one of the fastest JSON libraries for C. Facts are loaded into the `FactDB`, rules are iterated and their `if` conditions are recursively converted into `Node` AST trees.

### Step 2: Semantic Check

The semantic checker runs **during** parsing, not after. It catches:

- **Undefined facts**: referencing `"isBanned"` when no such fact exists
- **Type mismatches**: `{ ">": ["isAdmin", 18] }` — comparing a bool to a number
- **Empty operators**: `{ "and": [] }` or `{ "not": [] }`
- **Duplicate rule names**: two rules sharing the same `name`

Failures produce clear error messages and abort engine construction — no garbage-in/garbage-out.

### Step 3: Compile to Bytecode

The AST is **postorder-walked** into a flat instruction sequence:

```
AST:                           Bytecode:
  AND                          PUSH_FACT isAdmin
  ├── FACT(isAdmin)            PUSH_CMP  age > 18
  └── COMPARE(age > 18)        PUSH_CMP  balance >= 1000
      └── COMPARE(balance)     OP_AND
                               OP_AND
                               OP_HALT
```

Each `Node` becomes 1 or 2 instructions. The tree is kept (for debugging via `printAST`), but only the bytecode is used at evaluation time.

### Step 4: Evaluate

`runEngine()` iterates every rule in the hash table, calls `runBytecode()` on the rule's instruction array:

```
stack: [ ]
PUSH_FACT isAdmin       → stack: [true]
PUSH_CMP  age > 18      → stack: [true, true]
PUSH_CMP  balance>=1000 → stack: [true, true, true]
OP_AND                  → stack: [true, true]      (true && true)
OP_AND                  → stack: [true]             (true && true)
OP_HALT                 → return true
```

Result: `true` → action callback fires.

---

## 📝 Rule Format

### Operators

| Operator | JSON | Meaning |
|----------|------|---------|
| `and` | `{ "and": [A, B, ...] }` | All conditions true |
| `or` | `{ "or": [A, B, ...] }` | Any condition true |
| `not` | `{ "not": A }` | Negation |
| `>` | `{ ">": ["fact", val] }` | Numeric greater-than |
| `<` | `{ "<": ["fact", val] }` | Numeric less-than |
| `>=` | `{ ">=": ["fact", val] }` | Greater-or-equal |
| `<=` | `{ "<=": ["fact", val] }` | Less-or-equal |
| `==` | `{ "==": ["fact", val] }` | Numeric equality |
| `!=` | `{ "!=": ["fact", val] }` | Numeric inequality |

### Nesting

Arbitrarily deep nesting works naturally:

```json
{
  "and": [
    "isVerified",
    { "or": [
        "isPremium",
        { ">": ["balance", 500] },
        { "and": ["isAdmin", { "not": "isSuspended" }] }
    ]}
  ]
}
```

### n-ary AND/OR

`and` and `or` accept **two or more** operands. The parser builds a left-deep binary tree:

```
{ "and": [A, B, C, D] }

→  AND
   ├── AND
   │   ├── AND
   │   │   ├── A
   │   │   └── B
   │   └── C
   └── D
```

The bytecode emitter linearizes this into:

```
PUSH A
PUSH B
OP_AND
PUSH C
OP_AND
PUSH D
OP_AND
OP_HALT
```

---

## ⚙️ Bytecode VM Design

### Instruction Set

| Opcode | Byte | Operands | Stack Effect |
|--------|------|----------|-------------|
| `OP_PUSH_FACT` | `0x00` | cmp(1B) + name(NUL-term) + value(8B) | `→ bool` |
| `OP_PUSH_CMP` | `0x01` | cmp(1B) + name(NUL-term) + value(8B) | `→ bool` |
| `OP_AND` | `0x02` | — | `a, b → (a && b)` |
| `OP_OR` | `0x03` | — | `a, b → (a \|\| b)` |
| `OP_NOT` | `0x04` | — | `a → !a` |
| `OP_HALT` | `0x05` | — | pops and returns top |

### Run Loop (simplified)

```c
for (int pc = 0; pc < bc->count; pc++) {
    Instr* i = &bc->code[pc];
    switch (i->op) {
        case OP_PUSH_FACT: stack[sp++] = getBoolFact(db, i->factName); break;
        case OP_PUSH_CMP:  stack[sp++] = runCompare(db, i);           break;
        case OP_AND:       sp--; stack[sp-1] &= stack[sp];            break;
        case OP_OR:        sp--; stack[sp-1] |= stack[sp];            break;
        case OP_NOT:       stack[sp-1] = !stack[sp-1];                break;
        case OP_HALT:      return stack[sp-1];
    }
}
```

No function calls per node, no recursion, no dynamic dispatch — just a pure `switch` hot loop. Each rule's bytecode fits in a single cache line for simple conditions.

### Binary Format (`.velabc`)

```
┌──────────────────────────────────────────────────┐
│ Header (16 bytes)                                │
│  Magic "RULE" (0x524C4542)  │ uint32 LE          │
│  Version (3)                 │ uint32 LE          │
│  Instruction count           │ uint32 LE          │
│  Rule count                  │ uint32 LE          │
├──────────────────────────────────────────────────┤
│ Rule 1                                           │
│  Name length (1B) → Name (N bytes)               │
│  Action length (1B) → Action (N bytes)           │
│  Instructions:                                   │
│   [Opcode(1B) + Cmp(1B) + Name(NUL) + Value(8B)]│
│   ... until OP_HALT                              │
├──────────────────────────────────────────────────┤
│ Rule 2 ...                                       │
└──────────────────────────────────────────────────┘
```

---

## 🧠 Memory Model: Arena Allocator

### Why an Arena?

Rule engines create many small, interconnected objects: `Node` trees, `Instr` arrays, duplicated strings. Allocating each individually with `malloc` causes:

- **Fragmentation**: tiny allocations scatter across the heap
- **Slow teardown**: freeing every node requires walking the entire tree
- **Cache misses**: nodes and instructions in unrelated pages

### The Arena Solution

```c
// A single mmap backs everything
Arena* ar = createArena(1024 * 1024);  // 1MB

// All allocations are bumps
Node* n       = arena_alloc(ar, sizeof(Node));
char* name    = arena_strdup(ar, "isAdmin");
Instr* code   = arena_alloc(ar, sizeof(Instr) * count);

// Teardown is one call
destroyArena(ar);  // munmap + free(ar)
```

**Key properties:**
- **Thread-safe**: per-arena mutex serializes bumps
- **Alignment**: all allocations rounded up to 8 bytes
- **No per-object free**: reset is `O(1)` — just set `used = 0`
- **Zero-initialized**: `mmap` guarantees fresh pages are zero

The arena is the **primary reason** Vela can construct and tear down rule engines with zero allocation overhead at runtime.

---

## 🔒 Thread Safety

Vela is designed for concurrent use from the ground up:

| Component | Lock Type | Scope |
|-----------|-----------|-------|
| `FactDB` | `pthread_rwlock_t` | Multiple readers OR single writer |
| `RuleEngine` | `pthread_mutex_t` | Exclusive access to rule hash table |
| `Arena` | `pthread_mutex_t` | Serializes bump allocations |
| `Engine` | `pthread_mutex_t` | Coordinates action registration + engine access |

**Pattern:** `runEngine()` locks the `RuleEngine` mutex, then iterates rules. Each `runBytecode()` call reads the `FactDB` with read locks (shared). This allows multiple threads to evaluate rules concurrently as long as facts aren't being mutated.

**Writers** hold the FactDB write lock exclusively, blocking all readers until the update completes.

---

## 🎬 Action Registry

Rather than hardcoding action dispatch in a switch statement, Vela uses a **registry pattern**:

```c
// Register callbacks by name — order doesn't matter
registerTheAction(e, "GRANT_ACCESS", on_grant_access, "ctx");
registerTheAction(e, "SEND_ALERT",   on_send_alert,   NULL);
registerTheAction(e, "LOG_EVENT",    on_log_event,    my_logger);

// The JSON only names actions:
// { "name": "check_admin", "action": "GRANT_ACCESS", "if": "isAdmin" }
```

Registration works **before or after** `createEngine()`. If registered after, `registerTheAction()` retroactively patches any already-parsed rules that reference that action name — no need to rebuild.

---

## 📜 Vela DSL (Custom Language)

Beyond JSON, rules can be written in a custom language (`.vela` files):

```vela
# Fact declarations
FACT isAdmin = true
FACT age     = 25

# Rule definitions
RULE admin_check -> GRANT_ACCESS
    COND isAdmin AND age > 18

RULE complex_check -> SEND_ALERT
    COND NOT (isBanned OR isGuest) AND (balance >= 1000 OR isPremium)
```

The language is lexed and parsed in **TypeScript** (`src/velang/`), then compiled to the same `.velabc` binary bytecode format. This gives you:

- **Type-checked** syntax with clear error messages
- **Comments** with `#`
- **Capitalized keywords** for readability
- **Same bytecode output** as the JSON path

```bash
cd src/velang
npm run build
node dist/vela.js rules.vela   # produces rules.velabc
```

Then load the bytecode directly in C:

```c
Engine* e = createEngine("rules.velabc", BYTECODE);
```

---

## 📊 Performance Characteristics

| Metric | Estimate | Notes |
|--------|----------|-------|
| **Rule evaluation** | ~50-200 ns per rule | Depends on condition complexity |
| **Engine init** | ~50-100 µs | JSON parse + AST build + compile (1MB arena) |
| **Memory per rule** | ~200-500 bytes | AST + bytecode + string data (arena) |
| **Max rules** | 1000 (configurable) | Hash table, `MAX_RULES` in `rule.h` |
| **Max facts** | 300 (configurable) | `MAX_FACTS` in `factdb.h` |
| **Thread overhead** | Negligible | RW locks are uncontended in read-heavy workloads |

### Comparison with Alternatives

| Engine | Evaluation | Memory Model | Embeddable | Thread-Safe |
|--------|-----------|-------------|------------|-------------|
| **Vela** | Bytecode VM | Arena + uthash | ✅ (C library) | ✅ |
| **Drools** | RETE network | GC heap | ❌ (JVM) | ✅ |
| **json-rules-engine** | Tree-walk | JS heap | ❌ (Node) | ❌ (single-threaded) |
| **Ruleby** | Custom | Ruby heap | ❌ (MRI) | ❌ |
| **EasyRules** | Tree-walk | Java heap | ❌ (JVM) | ❌ |

Vela is the only option that compiles rules to bytecode, manages all memory from a linear arena, and exposes a plain-C API with zero runtime dependencies (beyond libc and pthreads).

---

## 🛠️ Build System & Dependencies

### Prerequisites

| Dependency | Version | Why |
|-----------|---------|-----|
| **CMake** | ≥ 3.10 | Build system |
| **C compiler** | C11 (GCC/Clang) | `_Static_assert`, `stdatomic.h` |
| **yyjson** | ≥ 0.8 | JSON parsing (system library) |
| **uthash** | 2.3.0 (vendored) | Hash tables — no install needed |
| **POSIX threads** | — | Locking (Linux/macOS) |
| **Valgrind** | optional | Memory leak detection |

### Building

```bash
# Install yyjson (Debian/Ubuntu)
sudo apt install libyyjson-dev

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run tests
./engine

# Memory check
make valgrind
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Debug | `Release` strips debug info and optimizes |
| `VALGRIND_EXECUTABLE` | auto | Path to valgrind (auto-detected) |

---

## 🗺️ Roadmap

### Completed ✅
- [x] Fact storage with uthash (bool + numeric)
- [x] Thread-safe FactDB with read-write locks
- [x] Condition AST (AND, OR, NOT, FACT, COMPARE)
- [x] Semantic checker (operator validation, type checking, existence, duplicates)
- [x] AST → bytecode compiler
- [x] Stack-machine VM for bytecode execution
- [x] Arena allocator (mmap-backed, thread-safe)
- [x] Action registry with retroactive binding
- [x] JSON parser via yyjson
- [x] Binary bytecode format (.velabc) and loader
- [x] Vela DSL: TypeScript lexer, parser, bytecode compiler
- [x] 27-rule test suite with pass/fail assertions

### In Progress 🔄
- [ ] **Bitmask boolean facts** — encode bools as bitfields instead of hash table entries
- [ ] **Compile comparisons to derived booleans** — `age > 18` → derived `_age_gt_18` fact bit
- [ ] **Rule-level dependency tracking** — only re-evaluate rules whose facts changed

### Planned 📋
- [ ] `BETWEEN` and `IN` operators
- [ ] Runtime fact mutation without engine rebuild
- [ ] Hot-reload rules from file
- [ ] Windows port (VirtualAlloc instead of mmap)
- [ ] C++ wrapper header
- [ ] Rule conflict detection (two rules with contradictory conditions/actions)
- [ ] Profile-guided optimization of bytecode layout

---

## ❓ FAQ

**Q: Why not just use Lua/Python/etc. as a rule engine?**  
A: Scripting languages pull in an entire runtime (VM, GC, standard library). Vela is a single header/source library with no runtime beyond libc+pthreads. A Lua state is ~50KB; Vela's entire compiled engine is smaller.

**Q: Can I update facts at runtime?**  
A: Yes. `setBoolFact()` and `setNumFact()` are public API. Call them anytime, then `runEngine()` re-evaluates all rules against the new state.

**Q: What happens if no action handler is registered?**  
A: The rule evaluates but no callback fires. A warning is printed (can be disabled). This is useful for debugging — you can inspect which rules would fire before wiring actions.

**Q: How do I add custom comparison logic?**  
A: The `CompareOp` enum supports `<`, `<=`, `>`, `>=`, `==`, `!=`. For custom logic, write a fact that your other code sets, then reference that fact in rules.

**Q: Is there a C++ API?**  
A: The C API is directly usable from C++. A more idiomatic C++ wrapper with `std::function` support is planned.

**Q: Can I use this on an embedded system?**  
A: Yes, if your system has `mmap` (Linux) or `VirtualAlloc` (Windows — planned). The arena can be backed by a static buffer instead of mmap with minor changes.

---

## 📄 License

Apache 2.0 — see `LICENSE`.

The vendored `uthash.h` is copyright Troy D. Hanson and released under BSD-revised.

---

*Built with C, ambition, and way too much time thinking about bits.*
