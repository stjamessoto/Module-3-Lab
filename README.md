# Module 3 Lab 4: Client-Server Using Fork

This repository contains the C and C++ solutions for Lab 4, plus the extra
files needed for the Exercise 3 challenge questions and the Exercise 4 DoS
simulation. Every program in this repository has been compiled and run
end to end while building this repo, so the steps below are known to work.

## What is in each exercise

| Exercise | What it asks for | Where the solution lives |
|---|---|---|
| Exercise 1 | Basic C fork server and client | `c/server_basic.c` (unmodified starter, used to demonstrate the zombie bug) and `c/client.c` |
| Exercise 2 | Same architecture in C++ | `cpp/server.cpp`, `cpp/client.cpp` |
| Exercise 3 | Five challenge questions | Fixed for good in `c/server.c` and `cpp/server.cpp`, exercised with `c/client.c` and `scripts/stress_test.sh` |
| Exercise 4 | Safe, localhost only DoS simulation | `dos_sim/dos_sim_local.c` |

## Directory structure

```
Module-3-Lab/
├── README.md                 this file
├── Makefile                  optional shortcut to build everything at once
├── .gitignore                keeps compiled binaries out of git
├── c/
│   ├── server_basic.c         Exercise 1 starter server, no fixes (used to show zombies)
│   ├── server.c               Exercise 1 server with all Exercise 3 fixes applied
│   └── client.c                client used against both C servers
├── cpp/
│   ├── server.cpp             Exercise 2 server, with the same fixes as c/server.c
│   └── client.cpp              client used against the C++ server
├── scripts/
│   └── stress_test.sh          Exercise 3 Q2, fires many simultaneous clients
└── dos_sim/
    └── dos_sim_local.c         Exercise 4, localhost only connection storm
```

Only `.c`, `.cpp`, `.sh` and config files are committed. Compiled binaries
are left out of git on purpose (see `.gitignore`) since they are trivial to
rebuild and would just add noise to the repository, they are what I will
actually run and screenshot on my own machine.

## Setup

Install the compiler toolchain once, exactly as the lab instructions say:

```bash
sudo apt update && sudo apt install build-essential
```

## Building everything

The easiest way is the Makefile:

```bash
cd Module-3-Lab
make
```

That produces `c/server`, `c/server_basic`, `c/client`, `cpp/server_cpp`,
`cpp/client_cpp` and `dos_sim/dos_sim_local`. Run `make clean` to remove
them again.

I can also just type the commands from the lab sheet directly, they
work the same way:

```bash
gcc c/server_basic.c -o c/server_basic
gcc c/server.c -o c/server
gcc c/client.c -o c/client
g++ cpp/server.cpp -o cpp/server_cpp
g++ cpp/client.cpp -o cpp/client_cpp
gcc dos_sim/dos_sim_local.c -o dos_sim/dos_sim_local
```

## Running Exercise 1 and Exercise 2

Open two terminals. In the first, start a server. In the second, run the
matching client one or more times.

```bash
# Terminal 1
./c/server_basic

# Terminal 2
./c/client
```

For the C++ pair:

```bash
# Terminal 1
./cpp/server_cpp

# Terminal 2
./cpp/client_cpp
```

`c/client.c` and `cpp/client.cpp` both accept an optional message on the
command line, for example `./c/client "custom text"`. With no argument they
send a default greeting.

## What each Exercise 3 fix looks like in the code

`c/server.c` and `cpp/server.cpp` are the finished, robust servers. Every
fix is labelled with a `Q1` to `Q5` comment in the source so I can point
to the exact lines in my report.

1. **Zombie processes.** `server_basic.c` forks a child for every client but
   never calls `wait()` or `waitpid()`, so each child stays in the process
   table as a `<defunct>` zombie after it exits. `server.c` installs a
   `SIGCHLD` handler that calls `waitpid(-1, NULL, WNOHANG)` in a loop, so
   the kernel notifies the parent the instant a child finishes and it is
   reaped immediately.
2. **Concurrent clients stress test.** `scripts/stress_test.sh` uses
   `xargs -P` to launch many copies of the client at the same time instead
   of one after another, which is what actually exercises concurrency.
3. **Port reuse.** `server.c` calls `setsockopt()` with `SO_REUSEADDR`
   before `bind()`, which lets the server restart on the same port right
   away instead of failing with "Address already in use".
4. **Buffer overflow edge case.** Instead of one fixed size `read()` into a
   1024 byte array, `read_full_message()` reads in a loop and grows the
   buffer with `realloc()` (C) or appends to a `std::string` (C++), so a
   10,000 character message is received safely instead of being truncated.
5. **Graceful shutdown.** Sending the message `"shutdown"` makes the child
   handling that client send `SIGTERM` to its parent, which closes the
   listening socket and exits cleanly, unlike `Ctrl+C`, which just kills the
   process outright without a chance to clean up.

## Running the Exercise 3 demonstrations

Before every demonstration below, make sure nothing from a previous test is
still running. Only one server should ever be alive at a time, otherwise a
leftover instance quietly answers my client instead of the one I meant
to test, and every later screenshot in the sequence goes wrong. Run this
cleanup first, every time:

```bash
pkill -x server_basic; pkill -x server; pkill -x server_cpp; pkill -x dos_sim_local
sleep 1
```

`-x` matches the exact process name, so this cannot accidentally match the
command I just typed the way a plain `pkill -f server` would.

**Zombie processes, before the fix:**

```bash
./c/server_basic &
./c/client one
./c/client two
./c/client three
ps -eo pid,ppid,stat,cmd | grep defunct
kill %1
```

**Zombie processes, after the fix (no defunct entries appear):**

```bash
./c/server &
./c/client one
./c/client two
./c/client three
ps -eo pid,ppid,stat,cmd | grep server
./c/client shutdown
```

**Concurrent clients stress test, run against the fixed server:**

```bash
./c/server &
./scripts/stress_test.sh c/client 50
./c/client shutdown
```

**Port reuse problem:** I try the literal steps from the lab sheet first on
my own machine, kill the server while a client is still connected, then
restart it immediately.

```bash
./c/server_basic
# in another terminal: ./c/client one
# press Ctrl+C in the server terminal, then immediately:
./c/server_basic
```

Whether this actually produces `bind: Address already in use` depends on
my kernel's TCP settings. Many current Linux systems, including this
one, enable `tcp_tw_reuse` for loopback traffic by default, which quietly
lets a fresh bind reuse a lingering `TIME_WAIT` socket even without
`SO_REUSEADDR`, so the classic error will not always appear. I check my
own setting with `cat /proc/sys/net/ipv4/tcp_tw_reuse` and mention it in
my report either way, it is a relevant part of the answer.

For a demonstration that is guaranteed to work on any machine, start two
instances at the same time instead of restarting one:

```bash
./c/server_basic &
./c/server_basic
```

`server_basic.c` never checks the return value of `bind()`, so it prints
"listening on port 8080" a second time even though the second instance's
bind actually failed and it is not really listening anywhere useful. Now
compare that with the fixed server, which does check:

```bash
./c/server &
./c/server
```

`server.c` correctly detects the conflict and prints
`bind: Address already in use` through `perror()`. That contrast, checked
versus unchecked error handling, is real and 100% reproducible regardless
of kernel settings, and is worth including alongside whichever result I
get from the literal restart test above.

**Large message / buffer overflow edge case:**

```bash
./c/server &
./c/client --big 10000
./c/client shutdown
```

**Graceful shutdown versus Ctrl+C:**

```bash
./c/server &
./c/client shutdown
# compare with starting it again and pressing Ctrl+C instead
```

## Running Exercise 4: the safe DoS simulation

`dos_sim_local.c` only ever connects to `127.0.0.1`, the target address is
hard coded and is not a command line option, so it cannot be pointed at
another machine. I use it only against my own server on my own computer.

```bash
./c/server &
ps --ppid $(pgrep -f "\./c/server$") | wc -l      # baseline: 0 children
./dos_sim/dos_sim_local 8080 100 10 &
sleep 2
ps --ppid $(pgrep -f "\./c/server$") | wc -l      # ~100 children while held open
wait
ps --ppid $(pgrep -f "\./c/server$") | wc -l      # back to 0, no zombies left
```

Arguments are `<port> <max_connections> <hold_seconds>`. I start with small
numbers such as `50` connections and `10` seconds, then increase gradually
while watching CPU, memory and the connection count.

Mitigations worth discussing in my report: capping the number of
simultaneous accepted clients, closing sockets that stay idle too long
with `SO_RCVTIMEO`, moving away from one process per connection toward a
thread pool or an event loop, enabling SYN cookies, and rate limiting new
connections per source IP.

