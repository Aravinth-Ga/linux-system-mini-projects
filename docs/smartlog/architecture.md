# SmartLog Architecture

## Scope

SmartLog is a small C logging component with two integration surfaces:
- library API (`smartlog_write_log_entry(...)`)
- CLI wrapper (`mini_log`)

The goal is predictable append behavior with optional durability and bounded file growth.

## Current Components

- `projects/smartlog/src/smartlog_core.c`
Core implementation for validation, timestamp formatting, write path, and rotation flow.

- `projects/smartlog/src/utils.c`
Low-level helpers (`write` retry loop, directory sync, and utility functions shared by core/CLI).

- `projects/smartlog/src/mini_log.c`
CLI argument parsing, option validation, signal handling, and delegation to core API.

- `projects/smartlog/include/smartlog/*.h`
Public header surface for callers and tests.

## Runtime Flow (Current)

1. Caller invokes API or CLI.
2. Inputs are validated (`file_path`, message, flags, optional max-bytes value).
3. Timestamp/PID metadata is assembled into one log line.
4. File is opened in append mode and write is executed with interruption-safe loop.
5. If durable mode is enabled, data is synced to disk (`fdatasync`) and directory metadata is synced where needed.
6. If max-bytes is enabled and threshold is crossed, single-backup rotation (`.1`) is applied.

## Error Model

- API returns `0` on success, non-zero on failure.
- `errno` is set by failing syscall/validation path for caller-level handling.
- CLI propagates non-zero exits and prints concise failures.

## Design Notes

- Header layout uses `include/smartlog/` namespace to avoid collisions for generic names like `config.h`.
- Core logic is kept outside CLI to keep embedding straightforward for future daemons/services.
- Rotation is intentionally simple today (single backup generation) to keep behavior deterministic.

## Known Limits

- No multi-generation rotation policy yet.
- No structured log levels/fields yet.
- No shared-state thread-safe logger object yet.
