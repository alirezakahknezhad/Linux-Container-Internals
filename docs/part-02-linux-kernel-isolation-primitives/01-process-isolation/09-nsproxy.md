# 🧑‍💻 Chapter 9 - nsproxy
Before we discuss the main concepts of this chapter, namely the **`nsproxy`** structure, and considering that we will examine namespaces in detail in the next chapter, we will first examine the concept of **`Namespace`** in the Linux world system.

## 🧩 Namespace
Namespaces are one of the main mechanisms in the Linux kernel that allow the operating system to provide **`isolation`** between processes. Simply put, Namespaces limit the visibility of each process to **system resources**; that is, each group of processes sees only that part of the system that is defined in their own namespace.

In fact, Namespace in Linux is a kind of **logical isolation** within the kernel. In other words, Namespace controls the visibility of a process to the resources available in the kernel. All these controls are done at the process level, meaning each Namespace determines which parts of the system resources a process can see or access.
## 🔗 nsproxy — The Namespace Proxy
While examining `task_struct`, one of the most important fields I saw was nsproxy, which was used as follows. Every task_struct has exactly one pointer to an nsproxy structure:
```c
/* include/linux/sched.h */
struct task_struct {
    ...
    struct nsproxy *nsproxy;
    ...
};
```
When the kernel needs to answer questions like:
- "What hostname does this process see?"
- "What network interfaces does this process have access to?"
- "What does PID 1 mean to this process?"

## 🔎 Kernel Definition
```c
/* include/linux/nsproxy.h */
struct nsproxy {
    refcount_t count;
    struct uts_namespace    *uts_ns;   /* hostname, domainname            */
    struct ipc_namespace    *ipc_ns;   /* SysV IPC, POSIX message queues  */
    struct mnt_namespace    *mnt_ns;   /* filesystem mount points         */
    struct pid_namespace    *pid_ns_for_children; /* PID number space     */
    struct net              *net_ns;   /* network stack                   */
    struct time_namespace   *time_ns;  /* clock offsets (CLOCK_MONOTONIC) */
    struct time_namespace   *time_ns_for_children;
    struct cgroup_namespace *cgroup_ns;/* cgroup hierarchy view           */
```
At kernel boot, a single global nsproxy is created. Every process on a standard Linux system ,before any containers exist , shares this one : 
```c
struct nsproxy init_nsproxy = {
	.count			= REFCOUNT_INIT(1),
	.uts_ns			= &init_uts_ns,
#if defined(CONFIG_POSIX_MQUEUE) || defined(CONFIG_SYSVIPC)
	.ipc_ns			= &init_ipc_ns,
#endif
	.mnt_ns			= NULL,
	.pid_ns_for_children	= &init_pid_ns,
#ifdef CONFIG_NET
	.net_ns			= &init_net,
#endif
#ifdef CONFIG_CGROUPS
	.cgroup_ns		= &init_cgroup_ns,
#endif
#ifdef CONFIG_TIME_NS
	.time_ns		= &init_time_ns,
	.time_ns_for_children	= &init_time_ns,
#endif
};
```
### ✅ How nsproxy is Copied "`copy_namespaces()`"
When `fork()` or `clone()` is called without any `CLONE_NEW*` flags, the kernel does not create a new nsproxy. It simply increments the reference count and hands the same pointer to the child:
```c
/* kernel/nsproxy.c */
int copy_namespaces(u64 flags, struct task_struct *tsk)
{
	struct nsproxy *old_ns = tsk->nsproxy;
	struct user_namespace *user_ns = task_cred_xxx(tsk, user_ns);
	struct nsproxy *new_ns;

	if (likely(!(flags & (CLONE_NS_ALL & ~CLONE_NEWUSER)))) {
		if ((flags & CLONE_VM) ||
		    likely(old_ns->time_ns_for_children == old_ns->time_ns)) {
			get_nsproxy(old_ns);
			return 0;
		}
	} else if (!ns_capable(user_ns, CAP_SYS_ADMIN))
		return -EPERM;

	/*
	 * CLONE_NEWIPC must detach from the undolist: after switching
	 * to a new ipc namespace, the semaphore arrays from the old
	 * namespace are unreachable.  In clone parlance, CLONE_SYSVSEM
	 * means share undolist with parent, so we must forbid using
	 * it along with CLONE_NEWIPC.
	 */
	if ((flags & (CLONE_NEWIPC | CLONE_SYSVSEM)) ==
		(CLONE_NEWIPC | CLONE_SYSVSEM))
		return -EINVAL;

	new_ns = create_new_namespaces(flags, tsk, user_ns, tsk->fs);
	if (IS_ERR(new_ns))
		return  PTR_ERR(new_ns);

	if ((flags & CLONE_VM) == 0)
		timens_on_fork(new_ns, tsk);

	nsproxy_ns_active_get(new_ns);
	tsk->nsproxy = new_ns;
	return 0;
}
```
The key function underneath is : 
```c
/* kernel/nsproxy.c */
static struct nsproxy *create_new_namespaces(u64 flags,
	struct task_struct *tsk, struct user_namespace *user_ns,
	struct fs_struct *new_fs)
{
	struct nsproxy *new_nsp;
	int err;
    ...
	new_nsp = create_nsproxy();
	...
	new_nsp->mnt_ns = copy_mnt_ns(flags, tsk->nsproxy->mnt_ns,
				      user_ns, new_fs);
	...
	new_nsp->uts_ns = copy_utsname(flags, user_ns, tsk->nsproxy->uts_ns);
	...
	new_nsp->ipc_ns = copy_ipcs(flags, user_ns, tsk->nsproxy->ipc_ns);
	...
	new_nsp->pid_ns_for_children =
		copy_pid_ns(flags, user_ns, tsk->nsproxy->pid_ns_for_children);
	...
	new_nsp->cgroup_ns = copy_cgroup_ns(flags, user_ns,
	...
	new_nsp->net_ns = copy_net_ns(flags, user_ns, tsk->nsproxy->net_ns);
	...
	new_nsp->time_ns_for_children = copy_time_ns(flags, user_ns,
					tsk->nsproxy->time_ns_for_children);
	...
	new_nsp->time_ns = get_time_ns(tsk->nsproxy->time_ns);

	return new_nsp;
    ...
```
This is the moment container isolation is born. Each **`copy_*_ns()`** function checks whether its corresponding flag is present. If `CLONE_NEWNET` is set, `copy_net_ns()` allocates a completely fresh network stack. If `CLONE_NEWNET` is absent, it just copies the pointer from the parent — the child shares the same network namespace.

## 🐳 Observing nsproxy Live with Docker
start a container : 
```bash
root@rootium:~# docker run -d --name ns_demo ubuntu:latest sleep infinity
```
get host-side PID
```bash
root@rootium:~# CPID=$(docker inspect --format '{{.State.Pid}}' ns_demo)
root@rootium:~# echo "Container PID: $CPID"
Container PID: 84146
```
every symlink in /proc/<pid>/ns/ is one pointer inside nsproxy :
```bash
root@rootium:~# ls -lai /proc/$CPID/ns/
total 0
436809 dr-x--x--x 2 root root 0 May 31 14:23 .
437318 dr-xr-xr-x 9 root root 0 May 31 14:23 ..
438619 lrwxrwxrwx 1 root root 0 May 31 14:23 cgroup -> 'cgroup:[4026532901]'
438615 lrwxrwxrwx 1 root root 0 May 31 14:23 ipc -> 'ipc:[4026532899]'
436810 lrwxrwxrwx 1 root root 0 May 31 14:23 mnt -> 'mnt:[4026532575]'
439312 lrwxrwxrwx 1 root root 0 May 31 14:23 net -> 'net:[4026532902]'
438616 lrwxrwxrwx 1 root root 0 May 31 14:23 pid -> 'pid:[4026532900]'
438617 lrwxrwxrwx 1 root root 0 May 31 14:23 pid_for_children -> 'pid:[4026532900]'
438620 lrwxrwxrwx 1 root root 0 May 31 14:23 time -> 'time:[4026531834]'
438621 lrwxrwxrwx 1 root root 0 May 31 14:23 time_for_children -> 'time:[4026531834]'
438618 lrwxrwxrwx 1 root root 0 May 31 14:23 user -> 'user:[4026531837]'
438614 lrwxrwxrwx 1 root root 0 May 31 14:23 uts -> 'uts:[4026532898]'
```
## 🐳 Proving nsproxy sharing between threads
start a container running two threads : 
```bash
root@rootium:~# docker run -d --name thread_ns ubuntu:22.04 \
    bash -c "sleep infinity & sleep infinity & wait"
62eac93f705496fd530b54d49bbd71093a669cf3569f822cc1c0e4833aa4e8b3
root@rootium:~# TPID=$(docker inspect --format '{{.State.Pid}}' thread_ns)
```
list all tasks (threads) in this process :
```bash
root@rootium:~# ls /proc/$TPID/task/
3500  3501  3502
```
all threads point to the SAME nsproxy — identical inode numbers : 
```bash
root@rootium:~# for TID in $(ls /proc/$TPID/task/); do
    echo "── TID $TID ──"
    ls -la /proc/$TPID/task/$TID/ns/ | awk '{print $9, $11}'
done
── TID 3500 ──
net -> net:[4026532585]
uts -> uts:[4026532579]
pid -> pid:[4026532581]

── TID 3501 ──
net -> net:[4026532585]
uts -> uts:[4026532579]
pid -> pid:[4026532581]

── TID 3502 ──
net -> net:[4026532585]
uts -> uts:[4026532579]
pid -> pid:[4026532581]
```
