# Retry Forever

Can't stop won't stop!

This is a simple "wrapper" process that restarts the child when the child exits
for any reason.

To compile:

    $ git clone https://github.com/Calvin-L/retry-forever.git
    $ cd retry-forever
    $ mkdir build
    $ cd build
    $ cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..
    $ cmake --build .

Usage example:

    $ ./retry-forever echo hello

The executable has no configuration or options.  It spawns a new process using
the given arguments.

## Q&A

**Why?**

Sometimes you just really need something to keep running.  See also:
[crash-only software](https://en.wikipedia.org/wiki/Crash-only_software).

**Why `MinSizeRel`?**

Like most software, this program will use your disk space 100% of the time and
your CPU almost never.  Also it doesn't do much and probably won't benefit from
more optimization.

**Why not a library?**

Using a wrapper process enables stronger protection against segfaults, the
nefarious OOM killer, and all manner of unexpected shenanigans.  Also it is
language-agnostic.

**How do I stop it?**

You caught me: can stop will stop (for interrupts).  To stop it, first kill the
parent process using your favorite signal (SIGINT, SIGTERM, and SIGKILL all
work, but my favorite is SIGUSR2).  Then kill the child.  In Bash (and other
shells?), Ctrl+C delivers a SIGINT to both processes, neatly stopping
the whole party.

**What else should I know?**

You should know how to summon unstoppable daemons:

    $ nohup ./retry-forever MY_PROGRAM >log.txt 2>&1 &
