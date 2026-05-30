# 🧑‍💻 PART 8 - struct `task_struct`
## 🧩 Brief explanation
A process in an operating system is actually an **abstraction that groups a resource** with the system and contains important information and things. All this information is put together under the name **`Process Control Group or PCB`**.
This information is in the Linux operating system in a structure called `task_struct`.

The Linux kernel stores the list of processes in a **circular doubly linked list** called the `task list`. Each element of this list is a process descriptor, which is defined in **`<linux/sched.h>`** in the Linux kernel. This list contains all the information about a particular process.

In Linux operating system, both `process` and `thread` are represented by a data structure called **`task_struct`** as a kernel data structure. This structure is one of the most basic objects in the entire kernel and contains all the metadata required for the following : 
- Manage processes
- Schedule execution
- Track memory usage
- Handle signals
- Maintain credentials
- Associate namespaces
- Control resources
- Manage process hierarchy
> If we ultimately consider Containers as a collection of Linux processes, then task_struct is very important in Container technology. Each containerized process is represented by its own `task_struct`.

In the data structure family, task_struct is a relatively large data structure, about 1.7KB in size on a 32-bit machine. However, this size can be small, considering that this data structure is intended to hold all the information needed by the kernel about a process.
The size of task_struct in bytes on each machine is as follows :
```
alireza@rootium:~/Linux-Container-Internals$ sudo cat /sys/kernel/slab/task_struct/object_size
10816
```
### ❗The actual structure of this file is very large, but its important points are as follows :
```c
struct task_struct {

    /* Process State */
    unsigned int __state;

    /* Scheduling */
    struct sched_entity se;

    /* Process Identification */
    pid_t pid;
    pid_t tgid;

    /* Parent/Child Relations */
    struct task_struct *parent;
    struct list_head children;

    /* Memory Management */
    struct mm_struct *mm;

    /* Filesystem Information */
    struct fs_struct *fs;

    /* Open File Descriptors */
    struct files_struct *files;

    /* Signal Handling */
    struct signal_struct *signal;
    struct sighand_struct *sighand;

    /* Credentials */
    const struct cred *cred;

    /* Namespaces */
    struct nsproxy *nsproxy;

    /* Cgroups */
    struct css_set *cgroups;

    /* Kernel Stack */
    void *stack;

};
``` 
## ✅ The important implementation of `task_struct` is as follows : 
### 1. Process State
The **`__state`** field indicates the current state of a process. The values ​​that this field can have are defined in this file under the macro heading : 
```c
#define TASK_RUNNING            0x00000000  /* on CPU or runqueue          */
#define TASK_INTERRUPTIBLE      0x00000001  /* sleeping, wakes on signal   */
#define TASK_UNINTERRUPTIBLE    0x00000002  /* sleeping, ignores signals   */
#define __TASK_STOPPED          0x00000004  /* stopped by signal (SIGSTOP) */
#define __TASK_TRACED           0x00000008  /* being traced by ptrace      */
#define TASK_DEAD               0x00000080  /* task is dying               */
#define TASK_ZOMBIE             EXIT_ZOMBIE /* exited, not yet reaped      */
```
- **`TASK_RUNNING`** : The process is either running or ready to run. The only possible state for a process running in user space.
- **`TASK_INTERRUPTIBLE `** : The process is in sleeping state but can be woken up by signals.
- **`TASK_UNINTERRUPTIBLE`** : The process is in sleeping state but cannot be woken up by signals.
- **`__TASK_STOPPED`** : The process has been stopped (e.g. via SIGSTOP).
- **`__TASK_TRACED `** : The process is in TRACED mode (e.g. by a debugger).

❗ When using, we will never change these directly. Instead, we will use Kernel Macros/Functions that are used to change the states of processes:

| API                                   | Exists now?   | Barrier? | Typical use                   |
| ------------------------------------- | ------------- | -------- | ----------------------------- |
| `set_current_state(state)`            | ✅             | Yes      | Before sleeping               |
| `__set_current_state(state)`          | ✅             | No       | When safe (after wake)        |
| `set_task_state(task, state)`         | ✅ (less used) | Yes      | Scheduler / freezer internals |

### 2. Process Identification
Its important fields are:
- **`pid_t pid;`** : the unique ID of THIS specific task (thread)
- **`pid_t tgid;`** : Thread Group ID — the PID of the thread group leader

### 3. Scheduling Information
This is used in the **`struct sched_entity se;`** field.

Contains scheduling metadata:
- Virtual runtime
- Priority
- CPU accounting
- Fair scheduling information

Linux Completely Fair Scheduler (CFS) uses this structure extensively. This allows Linux to:
- Balance CPU time
- Preempt tasks
- Handle priorities
- Implement fairness

### 4. Process Hierarchy and Family Relationships
This is implemented in the following fields : 
- **`struct task_struct  __rcu *real_parent;`**.
- **`struct task_struct  __rcu *parent;`**. 
- **`struct list_head  children;`**
- **`struct list_head  sibling;`** 
- **`struct task_struct  *group_leader;`**.

❗ The discussion of other topics is either redundant or will be covered in other chapters (for example, nsproxy, which is completely covered in the next chapter).

## 🔎 Why task_struct Matters for Container Internals
Understanding task_struct is essential because virtually every container isolation primitive eventually connects back to it.
| Container Feature | Related `task_struct` Field |
| ----------------- | --------------------------- |
| PID isolation     | `nsproxy`                   |
| Network isolation | `nsproxy`                   |
| Mount isolation   | `fs_struct`, `nsproxy`      |
| Resource limits   | `css_set`                   |
| Capabilities      | `cred`                      |
| Process lifecycle | `signal`, `parent`          |
| Scheduling        | `sched_entity`              |

## 🐳 View the view of checked items in a Docker container
🔺 First we run a container. : 
```bash
alireza@rootium:~/Linux-Container-Internals$ sudo docker run -it --name mycontainer ubuntu:latest sleep infinity
```

🔺 Then we extract the PID of the container : 
```bash
root@rootium:~# CONTAINER_PID=$(docker inspect --format '{{.State.Pid}}' mycontainer)
root@rootium:~# echo "Container PID on host: $CONTAINER_PID"
Container PID on host: 137467
```
🔺 **`/proc/<pid>/status`** exposes the most important identity fields:

```bash
root@rootium:~# cat /proc/$CONTAINER_PID/status
Name:	sleep
Umask:	0022
State:	S (sleeping)
Tgid:	137467
Ngid:	0
Pid:	137467
PPid:	137445
...
```
🔺 **`/proc/<pid>/maps`** shows the full mm_struct virtual memory layout : 
```bash
root@rootium:~# cat /proc/$CONTAINER_PID/maps
5670d814b000-5670d823d000 r--p 00000000 00:38 28056645                   /usr/lib/cargo/bin/coreutils/sleep
5670d823d000-5670d86f5000 r-xp 000f2000 00:38 28056645                   /usr/lib/cargo/bin/coreutils/sleep
5670d86f5000-5670d8ab4000 r--p 005aa000 00:38 28056645                   /usr/lib/cargo/bin/coreutils/sleep
5670d8ab4000-5670d8c19000 r--p 00968000 00:38 28056645                   /usr/lib/cargo/bin/coreutils/sleep
5670d8c19000-5670d8c1f000 rw-p 00acd000 00:38 28056645                   /usr/lib/cargo/bin/coreutils/sleep
...
73c9b7d30000-73c9b7d34000 r--p 00000000 00:00 0                          [vvar]
73c9b7d34000-73c9b7d36000 r--p 00000000 00:00 0                          [vvar_vclock]
73c9b7d36000-73c9b7d38000 r-xp 00000000 00:00 0                          [vdso]
...
7fffdc722000-7fffdc743000 rw-p 00000000 00:00 0                          [stack]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
```
🔺 Scheduler Fields : scheduling **`policy and priority`** :
```bash
root@rootium:~# chrt -p $CONTAINER_PID
pid 137467's current scheduling policy: SCHED_OTHER
pid 137467's current scheduling priority: 0
``` 
🔺 nice value = **`static_prio`** translated to userspace [-20..19]:
```bash
root@rootium:~# cat /proc/$CONTAINER_PID/stat | \
    awk '{print "nice:", $19, "  priority:", $18}'
nice: 0   priority: 20
```
🔺 CFS vruntime — how much CPU time this task has consumed : 
```bash
root@rootium:~# cat /proc/$CONTAINER_PID/schedstat
43254710 1888635 39
```
**output** : 
```
43254710  1888635  39
│           │      └─ number of timeslices run
│           └─ wait time on runqueue (nanoseconds)
└─ time spent on CPU (nanoseconds) = se.sum_exec_runtime
```
🔺 Namespaces — nsproxy : This is the most important section for containers. All namespaces of the container process : 
```bash
root@rootium:~# ls -la /proc/$CONTAINER_PID/ns/
total 0
dr-x--x--x 2 root root 0 May 30 15:03 .
dr-xr-xr-x 9 root root 0 May 30 15:03 ..
lrwxrwxrwx 1 root root 0 May 30 15:32 cgroup -> 'cgroup:[4026532852]'
lrwxrwxrwx 1 root root 0 May 30 15:32 ipc -> 'ipc:[4026532848]'
lrwxrwxrwx 1 root root 0 May 30 15:03 mnt -> 'mnt:[4026532773]'
lrwxrwxrwx 1 root root 0 May 30 15:03 net -> 'net:[4026532911]'
lrwxrwxrwx 1 root root 0 May 30 15:32 pid -> 'pid:[4026532850]'
lrwxrwxrwx 1 root root 0 May 30 15:32 pid_for_children -> 'pid:[4026532850]'
lrwxrwxrwx 1 root root 0 May 30 15:32 time -> 'time:[4026531834]'
lrwxrwxrwx 1 root root 0 May 30 15:32 time_for_children -> 'time:[4026531834]'
lrwxrwxrwx 1 root root 0 May 30 15:32 user -> 'user:[4026531837]'
lrwxrwxrwx 1 root root 0 May 30 15:32 uts -> 'uts:[4026532836]'
```
🔺 UID, GID, capabilities = task->cred : 
```bash
root@rootium:~# cat /proc/$CONTAINER_PID/status | grep -E "^(Uid|Gid|Cap)"
Uid:	0	0	0	0   ← real, effective, saved, filesystem UID
Gid:	0	0	0	0   ← all root inside container
CapInh:	0000000000000000    
CapPrm:	00000000a80425fb    ← permitted capabilities (reduced set)
CapEff:	00000000a80425fb    ← effective capabilities
CapBnd:	00000000a80425fb    ← bounding set
CapAmb:	0000000000000000
```
