# Linux System Mini Projects

This repository is a place for **small but complete Linux system programming projects**
that are built incrementally while learning and experimenting.

Each project brings together multiple Linux concepts and turns them into something
usable, reusable, and easy to extend over time.

These are not labs or one-off examples.  
They are mini systems meant to sit between syscall practice and larger projects.

## Why this repo exists

I already keep syscall-level practice in a separate labs repository.  
This repo is where those ideas start coming together in the form of **real, working components**.

At the moment, this repository is still taking shape, and projects are added and
refined as I move forward.

The focus here is on:
- understanding how Linux pieces work together
- writing code that can grow
- keeping things simple, readable, and portable

## How projects are written

Some ground rules I follow:
- no single-file demos
- no throwaway code
- clear separation between interfaces and implementation
- designs that can be reused in bigger systems later

## Projects

Work in progress.  
Projects will be added here as they take shape.

| Project   | What it explores |
|----------|------------------|
| smartlog | File I/O, permissions, atomic writes, log structure |
| (planned) | Signals, IPC, threads, polling, process control |

## Related repositories

- **linux-system-programming-labs**  
  Focused, syscall-by-syscall practice and experiments.

- **TelemetrySoc**  
  A larger system where ideas from these mini projects are reused and extended.
