**Build:** `cmake --build build --target unit_tests`
**Run unit tests:** `./build/unit_tests` (or `cmake --build build --target run_unit_tests`)
**Run unit tests under valgrind:** `cmake --build build --target valgrind_unit_tests`
**Run with verbose output:** `./build/unit_tests -v`
**Run specific suite:** `./build/unit_tests -s suite_name`
**Run specific test:** `./build/unit_tests -t test_name`
**Build & run integration tests:** `cmake --build build --target engine && ./build/engine`
**Integration tests under valgrind:** `cmake --build build --target valgrind`
**Rebuild everything:** `cmake --build build`
