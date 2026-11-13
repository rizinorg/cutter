# Cutter test suite

Tests cover what Cutter itself does as a frontend to rizin: turning rizin data
into views, notifying views when state changes, wiring user actions to the core,
and the state Cutter owns. They do not test the logic and functionality of rizin, it is extensively tested within rizin itself. See [rizin/test](https://github.com/rizinorg/rizin/tree/dev/test)

## Layout

Each directory is one layer of tests. Each `ctest` label matches its directory.

### `unit/`
- Tests Cutter's own algorithms and self-contained classes.
- Nothing of Cutter runs except the class under test.
- main is defined via `CUTTER_UNIT_TEST_MAIN`

### `session/`
- Tests core operations and the signals they emit.
- The core runs, but there is no `MainWindow` and no widget.
- Each test opens the binary it needs.
- main is defined via `CUTTER_CORE_TEST_MAIN`.

### `dialogs/`
- Tests one dialog at a time: input, validation, cancel and errors.
- The core runs, but there is no `MainWindow`. The test creates the dialog itself.
- Files end with `CUTTER_CORE_TEST_MAIN`.
- The startup dialogs (`test_OpenFile`, `test_CancelAnalysis`) are different. They run the whole
  app without a binary, so they can go through the New File dialog. 
- main is defined via 
  `CUTTER_APP_TEST_MAIN`.

### `workflows/`
- Tests user scenarios that go across several widgets.
- The whole app runs with a binary already loaded.
- main is defined via `CUTTER_APP_TEST_MAIN_WITH_FILE`.

### `common/`
- Shared helpers for all layers.
- It is the only code that creates the core or the app.

## Fixtures

`session/`, `dialogs/` and `workflows/` load a few binaries from
[rizin-testbins](https://github.com/rizinorg/rizin-testbins). CMake downloads
only those files, at the commit pinned in `common/Fixtures.cmake`, into
`build/test-fixtures/` and checks each one's SHA-256. Nothing to set up.

If you already have rizin-testbins cloned, point CMake at it and nothing is
downloaded:

```sh
cmake -B build -DCUTTER_TESTBINS_DIR=/path/to/rizin-testbins
```

Offline, or with `-DCUTTER_FETCH_TEST_FIXTURES=OFF`, tests that need a missing
fixture are reported as skipped. To add a fixture, add a line to
`common/Fixtures.cmake`. `Fixtures.h` is generated from it.

## Building

Tests are off by default. Turn them on once when you configure:

```sh
cmake -B build -DCUTTER_ENABLE_TEST=ON
```

Each test file is its own build target, named after the file. Each layer has a `tests_<layer>`
target, and `tests` builds them all:

```sh
cmake --build build                          # Cutter and every test
cmake --build build --target tests           # every test
cmake --build build --target tests_session   # one layer
cmake --build build --target test_Navigation # one test file
```

Add `--parallel <jobs>` to build with several jobs.

## Running

Build the tests first. `ctest` runs what was last built and doesn't rebuild anything.

```sh
ctest --test-dir build                    # everything
ctest --test-dir build -L session         # one layer
ctest --test-dir build -R test_Navigation # one test file
ctest --test-dir build -j 8               # 8 test files at the same time
```

`-j <jobs>` (or `--parallel <jobs>`) runs several test files at once. Each test file is its own
process with its own settings, so they don't get in each other's way.

Tests run headless (`QT_QPA_PLATFORM=offscreen`) with isolated settings, so they
never touch your own Cutter configuration.

### One test method

`ctest` runs a whole file. To run a single method, run the test binary itself and give the
method name. Layer tests are in `build/bin/tests/`. Unit tests are in `build/test/unit/`.
Set the same environment `ctest` does, so the test is headless and finds its binaries:

```sh
export QT_QPA_PLATFORM=offscreen														   # run in headless mode (no GUI)
export CUTTER_TESTBINS_DIR=$PWD/build/test-fixtures 									   # provide path to fixtures

build/bin/tests/test_Navigation backAndForwardMoveTheViews                                 # one method
build/bin/tests/test_Navigation backAndForwardMoveTheViews activatingXrefSeeksToReference  # several
build/test/unit/test_GraphLayouts layoutKeepsDrawingRules:if_else_diamond/grid_wide        # one data row
```

Some methods run once per row of data. To run one row, write `method:row`. 

For example, in the "one data row" command mentioned above, `layoutKeepsDrawingRules` checks every graph shape in every layout, and each row is named
`<shape>/<layout>`. So `layoutKeepsDrawingRules:if_else_diamond/grid_wide` only checks an
if/else graph laid out with the Grid wide layout. This helps when you only want to test a single row inside a testcase.

Any QtTest option works after the binary as well:

```sh
build/bin/tests/test_Navigation -functions                      # list the methods
build/test/unit/test_GraphLayouts -datatags                     # list the rows
build/bin/tests/test_Navigation -v2 backAndForwardMoveTheViews  # verbose output
```

## Adding a test

- Name it after the behavior or bug it guards (`test_<Area>.cpp`, one QtTest
  class per file).
- Add it to the `CMakeLists.txt` of its directory.
- If you can't name a Cutter bug it would catch, don't add it.
