**Build:** `cmake --build . --target unit_tests`
**Run unit tests:** `./unit_tests (or cmake --build . --target run_unit_tests)`
**Run unit tests under valgrind:** `cmake --build . --target valgrind_unit_tests`
**Run with verbose output:** `./unit_tests -v`
**Run specific suite:** `./unit_tests -s suite_name`
**Run specific test:** `./unit_tests -t test_name`
**Build & run integration tests:** `cmake --build . --target engine && ./engine`
**Integration tests under valgrind:** `cmake --build . --target valgrind`
**Rebuild everything:** `cmake --build .`
