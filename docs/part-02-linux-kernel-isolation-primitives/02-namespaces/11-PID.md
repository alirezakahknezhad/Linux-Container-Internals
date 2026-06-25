# 🧑‍💻 Chapter 11 - PID Namespace
## 🧩 Brief explanation
In Linux, each process has a unique identifier called **PID** or **Process ID**. But when it comes to Container or Namespace, having just one **PID table** globally is not enough, because **`each container must think that only`** its own processes exist.
> PID Namespace is a type of namespace that allows each group of processes to have their own independent view of PIDs.

The first process in a namespace pid is assigned a PID of 1 in that namespace and will be the parent process. This parent process must persist for the lifetime of the namespace and act as the parent for any orphaned processes.

To provide a certain amount of protection against **accidental termination**, only signals that have a defined handler can be sent to the parent process of a PID Namespace.

When the initial process terminates, new processes are no longer allowed to enter the PID Namespace, and all processes within the PID Namespace receive a **SIGKILL** signal. If PID 1 terminates for any reason, the kernel sends a **SIGKILL** to all remaining processes in the Namespace, effectively shutting down that Namespace.

Unlike other supported namespace types, **`PID Namespaces are hierarchical`**. Processes within a child PID Namespace are visible in the parent PID Namespace and are assigned local process identifiers, meaning that a process may be identified by different PIDs depending on which namespace it is referenced in.
#### 🖼️ The following figure shows how local process identifiers are assigned in nested PID Namespaces : 
![images](/images/pid-namespace-1.png)
### 🔎 Kernel Implementation
The important parts of its source code are as follows : 
#### 🧩 pid_namespace.h
This file is the logical heart of the PID Namespace implementation; the mechanism that allows each container or sandbox in Linux to have its own process tree. The important lines of this file are as follows : 
```c
...
#define MAX_PID_NS_LEVEL 32
...
extern struct pid_namespace init_pid_ns;
...
extern struct pid_namespace *copy_pid_ns(u64 flags,
        struct user_namespace *user_ns, struct pid_namespace *ns);
...
extern void     zap_pid_ns_processes(struct pid_namespace *pid_ns);
...
extern int reboot_pid_ns(struct pid_namespace *pid_ns, int cmd);
...
```
1. The system allows up to **32** levels of nested PID Namespaces. **`This means that one container can be under another container, up to a maximum depth of 32.`**
2. Default System Namespace: This is the **zero-level namespace (global PID namespace)** that is created at system boot time. All processes are members of this namespace by default.
3. New Namespace Creation Function: This function is called when a process with the `CLONE_NEWPID` flag is executed by `clone()` or `unshare()`.
4. Clean up processes when the namespace ends: When the namespace is being cleaned up (e.g. its last process terminates), this function terminates all remaining processes.
5. Namespace-specific reboot function: Allows the container to be “rebooted” **without rebooting the main system**.
#### 🧩 pid_namespace.c
This file is where all the functions that were only declared (prototyped) in the header file linux/include/linux/pid_namespace.h are actually implemented here.

#### 🔎 In order to have the commands in the Host and Namespace next to each other and compare them, the commands are shown side by side in the image below:
![images](/images/pid-namespace-2.png)

First, on the main bash of the system, we see the list of PID Namespaces. 1️⃣ Then create a PID Namespace on another bash and give it a fork of bash. 2️⃣ Then we execute the sleep command in this namespace. 3️⃣ Now, on our main bash, we will see the PID Namespaces again. 4️⃣ And on this bash, we execute another sleep command. 5️⃣ As can be seen, in both bash these two commands can be seen in the list of processes. 6️⃣ Now, by going to /proc/$PID_Number/status and viewing the NSpid value, we can see the actual PID value of this process in the main bash and its PID value in that Namespace, from left to right. 7️⃣ If we try to kill this command with its original PID in our PID Namespace, we will encounter an error. 8️⃣ If we enter the same command in the main bash, this operation will be successful. 9️⃣ If we perform the same operation in the PID Namespace with the PID number in this Namespace, this operation will be successful. 🔟

If we want to see the last pid number assigned in our PID Namespace, we can use **`/proc/sys/kernel/ns_last_pid"`**.
```bash
root@rootium:~# cat /proc/sys/kernel/ns_last_pid 
38856
```
![images](/images/pid-namespace-3.png)