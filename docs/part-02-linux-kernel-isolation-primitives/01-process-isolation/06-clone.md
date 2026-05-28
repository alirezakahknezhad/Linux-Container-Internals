# 🧑‍💻 PART 4 - clone() - System Call
## clone()
In the Linux operating system, a process or thread is created by a syscall called clone.<br>
🟥 Both `fork` and `pthread _create` use `clone()` in their implementation.<br>
Its manual shows us the following prototype : 
```
       /* Prototype for the glibc wrapper function */

       #define _GNU_SOURCE
       #include <sched.h>

       int clone(int (*fn)(void *_Nullable), void *stack, int flags,
                 void *_Nullable arg, ...  /* pid_t *_Nullable parent_tid,
                                              void *_Nullable tls,
                                              pid_t *_Nullable child_tid */ );

```
#### The following inputs are very important in this function : 
- `int (*fn)(void)` : This is a function pointer to a function that is executed by the child process.
- `*stack` : A downward-growing stack on which the child performs operations.
- `flags` : An integer value representing all flags used, configured using an OR-conjugation of all flags.
- `arg` : Additional arguments

##### 🔎 The most important parameter for us is the flag. So below we will review the most important flags.
