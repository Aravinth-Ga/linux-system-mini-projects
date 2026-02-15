# SmartLog

SmartLog is a small Linux logging project in C.
It provides a reusable core API and a CLI tool (`mini_log`) to append timestamped log lines to a file.

## Current Status

- Core logging code is implemented in `src/`.
- Public headers are in `include/smartlog/`.
- `examples/` and `test/` are placeholders right now.
- `CMakeLists.txt` builds both the library and CLI.

## Features (Current)

- Writes log lines with timestamp (nanoseconds) and PID.
- Creates the log file if it does not exist.
- Appends safely using retry logic for interrupted writes (`EINTR`).
- Optional durable mode (`--durable`) using `fdatasync` + parent dir `fsync`.
- Optional single-backup rotation (`--max-bytes <N>`) to `file.1`.
- Message size limit is 256 bytes (long messages are truncated with `...`).

## CLI Usage

```bash
./mini_log <file_path> "<message>" [--durable] [--max-bytes <size>]
```

Examples:

```bash
./mini_log app.log "server started"
./mini_log app.log "payment done" --durable
./mini_log app.log "worker heartbeat" --max-bytes 1048576
```

## Build

Using CMake:

```bash
cmake -S . -B build
cmake --build build
```

Direct `gcc` build:

```bash
gcc -Wall -Wextra -std=c11 \
  -Iinclude \
  src/mini_log.c src/smartlog_core.c src/utils.c \
  -o mini_log
```

## C API

Header:

- `include/smartlog/smartlog_core.h`

Main function:

```c
int smartlog_write_log_entry(
    const char* file_path,
    const char* msg,
    feature_state_t durable,
    feature_state_t max_bytes_config,
    unsigned long max_byte_val
);
```

## Repo Layout

- `src/mini_log.c`: CLI entry point and option parsing
- `src/smartlog_core.c`: core logging logic
- `src/utils.c`: time, write-all, and directory sync helpers
- `include/smartlog/config.h`: limits and feature flags
- `include/smartlog/smartlog_core.h`: public API

## CLI and Reusable API

- CLI mode: use `mini_log` for command-line logging.
- Library mode: call `smartlog_write_log_entry(...)` from your C code.
- The core API is stdout-clean for embedding; on failure it returns non-zero and sets `errno`.

## Not Included Yet

- Multiple backup generations for rotation
- Log levels and formatting options
- Thread-safe shared logger state
- Completed tests and examples
