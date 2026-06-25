# 🧑‍💻 Chapter 10 - UTS Namespace
## 🧩 Brief explanation
UTS stands for **Unix Timesharing System** Namespace and is one of the simplest but most basic types of Namespaces in Linux. This Namespace allows each process to have its own `hostname` and `NIS (Network Information Service)` domain name, separate from the one set in the main system (global namespace). This type of Namespace allows a process to see a different hostname than the one in the global namespace. In other words, each container or group of processes can be configured to see a different system name. As a result, multiple environments can run on the same Linux kernel, each seeing itself as a separate system with its own hostname.
### 🔎 How UTS works : 
1. When a UTS Namespace is created, it starts with a copy of the hostname and NIS Domain from its parent namespace. This means that at the time of creation, the new namespace shares the same identifiers as its parent. However, any subsequent changes to the hostname or NIS domain name in the namespace will not affect other namespaces.
2. Processes within a UTS Namespace can change the hostname and NIS Domain Name using the **`setdomainname`** and **`sethostname`** syscalls. These changes are local to the namespace and do not affect other namespaces or the host system.
3. Processes can switch between namespaces using the **`setns`** syscall or create a new namespace using the **`unshare or clone syscalls`** with the **`CLONE_NEWUTS`** flag. When a process moves to a new namespace or creates one, it starts using the hostname and NIS domain name associated with that namespace.


### 🔎 Kernel Implementation
To see the implementation of this namespace, the following files exist at the kernel level : 
#### 🧩 uts.h
This file is located in the **`include/linux/`** directory and is one of the main kernel header files. Its purpose is to define default values ​​and structures related to the **UTS - Unix Time-Sharing System**. This is the subsystem that stores information such as: operating system name (sysname), host name (nodename), domain name (domain name). This information is what is displayed when you run the following command in the terminal :
```bash
root@rootium:~#  uname -a
Linux rootium 7.0.0-22-generic #22-Ubuntu SMP PREEMPT_DYNAMIC Mon May 25 15:54:34 UTC 2026 x86_64 GNU/Linux
```
This section is defined at the kernel level as follows : 
```c
GNU nano 8.7.1        /usr/src/linux-headers-7.0.0-22-generic/include/linux/uts.h                                                           
/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_UTS_H
#define _LINUX_UTS_H

/*
 * Defines for what uname() should return 
 */
#ifndef UTS_SYSNAME
#define UTS_SYSNAME "Linux"
#endif

#ifndef UTS_NODENAME
#define UTS_NODENAME CONFIG_DEFAULT_HOSTNAME /* set by sethostname() */
#endif

#ifndef UTS_DOMAINNAME
#define UTS_DOMAINNAME "(none)" /* set by setdomainname() */
#endif

#endif
```
#### 🧩 utsname.h
This file defines data and functions related to **`system information (uname, hostname, domainname)`**. This file is actually the main header related to the UTS Namespace in the Linux kernel and includes:
- Data structures related to system information (new_utsname)
- Pointers related to the UTS Namespace in each process
- Inline functions for quick access to system information.

This file is used in various parts of the kernel such as `sys_uname()` (for the `uname()` system call), `sethostname()`, `setdomainname()`, and the `/proc/sys/kernel/{hostname, domainname}` mechanism.
```bash
root@rootium:~# cat /proc/sys/kernel/hostname 
rootium
root@rootium:~# cat /proc/sys/kernel/domainname 
(none)
```
Now take a look at the different parts of its source code : 
```c
enum uts_proc {
        UTS_PROC_ARCH,
        UTS_PROC_OSTYPE,
        UTS_PROC_OSRELEASE,
        UTS_PROC_VERSION,
        UTS_PROC_HOSTNAME,
        UTS_PROC_DOMAINNAME,
};
```
This enumeration defines a set of constants that represent the types of fields that may change in **/proc/sys/kernel/**.
For example :
- `UTS_PROC_HOSTNAME` → refers to **/proc/sys/kernel/hostname**
- `UTS_PROC_DOMAINNAME` → refers to **/proc/sys/kernel/domainname**
- `UTS_PROC_VERSION` → refers to **/proc/sys/kernel/version**

The kernel uses this enum in the **`uts_proc_notify()`** function to notify other processes when one of these values ​​changes : 
```c
#ifdef CONFIG_PROC_SYSCTL
extern void uts_proc_notify(enum uts_proc proc);
#else
static inline void uts_proc_notify(enum uts_proc proc)
{
}
#endif
```
> The uts_proc_notify() function is only defined if the ‍`/proc/sysctl` feature is enabled in the kernel (`CONFIG_PROC_SYSCTL` option). Otherwise, an empty function is defined to prevent errors.
```bash
root@rootium:~# grep CONFIG_PROC_SYSCTL /boot/config-7.0.0-22-generic 
CONFIG_PROC_SYSCTL=y
```
The `new_utsname` function is called when one of the UTS fields changes (for example, when the hostname changes), so that the system can update `/proc/sys/kernel/hostname`.
```c
static inline struct new_utsname *utsname(void)
{
        return &current->nsproxy->uts_ns->name;
}
```
Every time the kernel wants to read or change the current hostname or domainname value, it uses this function to access the structure corresponding to that namespace.

The `init_utsname` function is similar to the previous one, but instead of the current Namespace, it points to the system's initial UTS Namespace (global namespace). `init_uts_ns` is the global namespace that is created when the system boots and other Namespaces (e.g. for containers) are copied from it.
```c
static inline struct new_utsname *init_utsname(void)
{
        return &init_uts_ns.name;
}
```
#### 🧩 uts_namespace.h
```c
GNU nano 8.7.1        /usr/src/linux-headers-7.0.0-22-generic/include/linux/uts_namespace.h    
/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_UTS_NAMESPACE_H
#define _LINUX_UTS_NAMESPACE_H

#include <linux/ns_common.h>
#include <uapi/linux/utsname.h>

struct user_namespace;
extern struct user_namespace init_user_ns;

struct uts_namespace {
	struct new_utsname name;
	struct user_namespace *user_ns;
	struct ucounts *ucounts;
	struct ns_common ns;
} __randomize_layout;

extern struct uts_namespace init_uts_ns;

#ifdef CONFIG_UTS_NS
static inline struct uts_namespace *to_uts_ns(struct ns_common *ns)
{
	return container_of(ns, struct uts_namespace, ns);
}

static inline void get_uts_ns(struct uts_namespace *ns)
{
	ns_ref_inc(ns);
}

extern struct uts_namespace *copy_utsname(u64 flags,
	struct user_namespace *user_ns, struct uts_namespace *old_ns);
extern void free_uts_ns(struct uts_namespace *ns);

static inline void put_uts_ns(struct uts_namespace *ns)
{
	if (ns_ref_put(ns))
		free_uts_ns(ns);
}

void uts_ns_init(void);
#else
static inline void get_uts_ns(struct uts_namespace *ns)
{
}

static inline void put_uts_ns(struct uts_namespace *ns)
{
}

static inline struct uts_namespace *copy_utsname(u64 flags,
	struct user_namespace *user_ns, struct uts_namespace *old_ns)
{
	if (flags & CLONE_NEWUTS)
		return ERR_PTR(-EINVAL);

	return old_ns;
}

static inline void uts_ns_init(void)
{
}
#endif

#endif /* _LINUX_UTS_NAMESPACE_H */
```
This file is the brain of the UTS Namespace implementation in the Linux kernel. This file contains the main definition of the UTS Namespace data structure in the Linux kernel. The UTS Namespace is what allows each container or isolated process to have its own hostname and set its own domainname, without affecting other processes in the system. In simple terms, this file is responsible for managing the UTS Namespace Life Cycle in the Kernel.

#### 🔎 To view the list of UTS Namespaces we have : 
```bash
root@rootium:~# lsns -t uts 
        NS TYPE NPROCS   PID USER    COMMAND
4026531838 uts     255     1 root    /usr/lib/systemd/systemd --switched-root --system --deserialize=51
4026532582 uts       1  1018 root    |-/usr/lib/systemd/systemd-udevd
4026532649 uts       1  1904 syslog  |-/usr/sbin/rsyslogd -n -iNONE
4026532650 uts       1  1843 root    |-/usr/lib/systemd/systemd-logind
4026532653 uts       3  1818 root    |-/bin/sh /usr/lib/systemd/scripts/chronyd-starter.sh -n -F 1
4026532654 uts       1  1836 polkitd |-/usr/lib/polkit-1/polkitd --no-debug --log-level=notice
4026532846 uts       1  7408 root    `-/usr/libexec/fwupd/fwupd
4026532658 uts      30  3045 root    /sbin/init
4026532730 uts       1  5057 65535   /pause
4026532735 uts       1  5108 65535   /pause
4026532740 uts       1  5134 65535   /pause
4026532745 uts       1  5178 65535   /pause
4026532762 uts       1  5560 65535   /pause
4026532825 uts       1  5719 65535   /pause
4026532830 uts       1  5742 65532   /coredns -conf /etc/coredns/Corefile
4026532834 uts       1  5750 65535   /pause
4026532845 uts       1 30110 65535   /pause
4026532911 uts       5 30397 root    nginx: master process nginx -g daemon off;
```
Now we want to create a UTS Namespace and assign the **`/bin/bash`** process to it : 
```bash
root@rootium:~# unshare --uts /bin/bash
```
```bash
root@rootium:~# lsns -t uts | grep bash
4026532914 uts       1 70737 root    /bin/bash
```
#### 🐳 Container engines like Docker also use the same mechanism : 
```bash
root@rootium:~# docker run -it --name ubuntu-test --hostname test ubuntu
root@test:/# 
```
If we test at the same time, we see that a new uts namespace has also been created : 
```bash
root@rootium:~# lsns -t uts | grep bash
4026532915 uts       1 73402 root    /bin/bash
```