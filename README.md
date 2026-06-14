# anchor
Anchor- A simple and highly optimized rule engine in C that can be directly added to projects and used for speedy rule calculations




## FURTHER OPTIMIZATIONS:

1. Bitmask engine
2. Compiled Bitecode engine + VM
3. Fact and Rule look up using hashmaps [ COMPLETED ]
4. Only recalculate particular rules if facts affect it (dependency tracking)
5. Bytecode compiler


## Other features yet to implement (apart from optimizations):

1. integration possibilies with C/C++ (directly use functions of this engine and actually give function ptrs that get executed)
2. Way to update the rules within the program and json file doesnt get re-written
3. add function pointers to actions so they are directly called
4. semantic checker [ COMPLETED ]
5. Operators: BETWEEN, IN
6. Runtime rule changes
7. HIGH OPTIMIZATIONSS
