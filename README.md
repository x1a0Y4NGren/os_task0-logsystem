# Smart Log Processing System Skeleton

This project is a small C language skeleton for an operating systems course assignment.
The topic is the producer-consumer problem in a "smart log processing system" setting.

## Directory Structure

- `src/main.c`: program entry, basic initialization, and placeholder threads
- `src/buffer.c`: circular buffer with mutex and condition variables
- `src/log_ai.c`: placeholder log classification and scoring module
- `src/output.c`: placeholder file output module
- `include/common.h`: shared constants and `LogEntry`
- `include/buffer.h`: buffer declarations
- `include/log_ai.h`: analysis declarations
- `include/output.h`: output declarations
- `Makefile`: build rules for Linux with `gcc -pthread`

## Build

```bash
make
```

## Run

```bash
./logsystem
```

## Current Status

- Creates 2 producer threads and 1 consumer thread as placeholders
- Prepares a circular buffer based on pthread mutexes and condition variables
- Keeps analysis and output logic simple for later expansion
- Creates `analysis.csv` and `alerts.log` during the demo run

## Suggested Next Steps

1. Connect producer threads to `buffer_push`
2. Connect consumer threads to `buffer_pop`
3. Add real log generation rules
4. Improve classification, scoring, and alert logic
