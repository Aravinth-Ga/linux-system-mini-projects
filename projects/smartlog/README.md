# SmartLog

A simple logging tool written in C for Linux. It saves what your program does to a file with the exact time it happened.

## What Is This?

SmartLog is a minimal way to log messages. You write a message, it gets saved to a file with a timestamp. That's it. Nothing fancy, just the basics that work.

## Features

Fast and lightweight. Adds timestamps automatically. Works in any directory you want. Handles interruptions safely. Easy to use from C code.

## How to Use

Build it with CMake:

```
mkdir build
cd build
cmake ..
make
```

Then in your C code, call the logger:

```
log_message("Application started");
log_message("Processing file");
log_message("Done");
```

Each message gets written to your log file with a timestamp. No setup needed.

## What's Inside

src/ - The logging code
include/ - Headers to use it
examples/ - Sample code
test/ - Tests
CMakeLists.txt - Build config

## How It Works

SmartLog uses standard Linux calls to write messages to a file. It takes care of creating the file if needed, adding messages to the end, and making sure everything gets saved properly. If something interrupts a write, it tries again.

## What It Doesn't Do

No log levels like DEBUG or ERROR. No rotating log files when they get big. Not safe for multiple threads. No custom formatting.

If you need those things, use a bigger logging library. If you want something simple that just works, this is it.

## Author

Aravinthraj Ganesan
