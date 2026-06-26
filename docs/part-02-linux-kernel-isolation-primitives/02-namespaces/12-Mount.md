# 🧑‍💻 Chapter 12 - PID Namespace
## 🧩 Brief explanation
Mount is one of the most important and fundamental types of namespaces in Linux. This namespace determines which filesystems and mount points each process can see and access. In other words, this mechanism is what makes each container think it has its own filesystem, even though they are all actually on the same Linux kernel and physical disk.

When a new Mount Namespace is created for a process (e.g. with clone(CLONE_NEWNS)), that process:
- receives a separate copy of the mount table,
- can mount or unmount new filesystems,
- and has no effect on the mounts of the original (host) namespace.

**🟩 So two processes on the same kernel may have completely different views of the filesystem.**
Within the Linux kernel, each mount point is represented by a **data structure called `vfsmount`**. This structure contains information such as the filesystem path, a pointer to the superblock (the main filesystem information), and a pointer to the parent mount. All mounts together form a mount tree.

Containers work on the concept of **bind mounts**. So, when a volume is created for a container, it is actually a bind mount from a directory on the host to a mount point in the container's file system. Since the mount happens in the mount namespace, the vfsmount constructs are limited to the mount namespace. This means that by creating a bind mount from a directory, we can **expose a volume in the namespace that holds the container**.
### 🔎 Kernel Implementation
#### 🧩 mnt_namespace.h
The mnt_namespace.h file is the central interface between the VFS (mount management) subsystem and the kernel's Namespace subsystem.
This file provides declarations for functions used to create, copy, free, and view Mount Namespaces (e.g., via /proc). In simple terms, this file is the connection point between the namespace management module and the filesystem (VFS) subsystem.
```c
/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _NAMESPACE_H_
#define _NAMESPACE_H_
#ifdef __KERNEL__

#include <linux/cleanup.h>
#include <linux/err.h>

struct mnt_namespace;
struct fs_struct;
struct user_namespace;
struct ns_common;

extern struct mnt_namespace init_mnt_ns;

extern struct mnt_namespace *copy_mnt_ns(u64, struct mnt_namespace *,
                struct user_namespace *, struct fs_struct *);
extern void put_mnt_ns(struct mnt_namespace *ns);
DEFINE_FREE(put_mnt_ns, struct mnt_namespace *, if (!IS_ERR_OR_NULL(_T)) put_mnt_ns(_T))
extern struct ns_common *from_mnt_ns(struct mnt_namespace *);

extern const struct file_operations proc_mounts_operations;
extern const struct file_operations proc_mountinfo_operations;
extern const struct file_operations proc_mountstats_operations;

#endif
#endif
```
According to the Kernel documentation, there are five mount states:
1. Shared : A mount that belongs to a peer group. Any changes that occur are propagated to all members of the peer group.
2. Slave : One-way propagation. The master mount point forwards events to a slave, but the master does not see any events that the slave does.
3. Shared and Slave : Indicates that the mount point has a master server (Master), but also its own Peer Group. The master server will not be notified of changes to a mount point, but each member of the Peer Group downstream will be notified of these changes.
4. Private : Does not receive or send any broadcast events.
5. Unbindable : Does not receive or send any broadcast events and cannot bind.

🟨 Most Container Engines use private mount modes when mounting a volume inside a container.

#### 🟩 Creating a Mount Namespace :
The mount namespace does not behave as you might expect after creating a new user namespace. By default, if you create a new mount namespace with the `unshare -m` command, your system view remains largely unchanged and unrestricted. The reason for this is that whenever you create a new mount namespace, **a copy of the mount point from the parent namespace is created in the new mount namespace.** This means that any actions taken on files in a poorly configured mount namespace will affect the host.
To explain this namespace, we will use an alpine Linux on our host.

```bash
root@rootium:~# export CONTAINER_ROOT_FOLDER=/container_practice   
root@rootium:~# mkdir -p ${CONTAINER_ROOT_FOLDER}/fakeroot
root@rootium:~# cd ${CONTAINER_ROOT_FOLDER}
root@rootium:/container_practice# wget https://dl-cdn.alpinelinux.org/alpine/v3.13/releases/x86_64/alpine-minirootfs-3.13.1-x86_64.tar.gz
--2026-06-26 09:14:11--  https://dl-cdn.alpinelinux.org/alpine/v3.13/releases/x86_64/alpine-minirootfs-3.13.1-x86_64.tar.gz
Resolving dl-cdn.alpinelinux.org (dl-cdn.alpinelinux.org)... 146.75.118.132, 2a04:4e42:8d::644
Connecting to dl-cdn.alpinelinux.org (dl-cdn.alpinelinux.org)|146.75.118.132|:443... connected.
HTTP request sent, awaiting response... 200 OK
Length: 2728935 (2.6M) [application/octet-stream]
Saving to: 'alpine-minirootfs-3.13.1-x86_64.tar.gz'

alpine-minirootfs-3.13.1-x86_64.tar.gz        100%[===============================================================================================>]   2.60M  1.30MB/s    in 2.0s    

2026-06-26 09:14:14 (1.30 MB/s) - 'alpine-minirootfs-3.13.1-x86_64.tar.gz' saved [2728935/2728935]

root@rootium:/container_practice# tar xzvf alpine-minirootfs-3.13.1-x86_64.tar.gz 
...
root@rootium:/container_practice# chown hacker . -R ${CONTAINER_ROOT_FOLDER}/fakeroot
```
The fakeroot directory must be owned by the container-user user because as soon as a new namespace user is created, the root user in the new namespace is mapped to the container-user outside the namespace. This means that the process inside the new namespace thinks it has the necessary capabilities to modify its files. However, the permissions of the host file system prevent the container-user account from modifying the Alpine files of fakeroot (which are owned by root).

**❗ So what happens if you simply create a new mount namespace?**
```bash
root@rootium:/container_practice# PS1='\u@new-mnt$ ' unshare -Umr
root@new-mnt$ df -h
Filesystem                         Size  Used Avail Use% Mounted on
/dev/mapper/rl-root                37G   5.4G 32G   15%  /
tmpfs                              3.8G  0    3.8G  0%   /sys/fs/cgroup
...
root@new-mnt$ ls /
bin  boot  cdrom  container_practice  dev  etc  home  lib  lib64  lost+found  media  mnt  opt  proc  root  run  sbin  snap  srv  swap.img  sys  tmp  usr  var
```
Now that you've entered the mount namespace, you might not expect to see any of the host's mount points. However, you don't. This is because systemd by default shares mount points recursively with all new namespaces.
If you mounted a tmpfs filesystem somewhere, say /mnt, inside the new mount namespace, can the host see it?
```bash
root@new-mnt$ mount -t tmpfs /mnt
root@new-mnt$ findmnt | grep mnt
/mnt    tmpfs   tmpfs   rw,relatime,seclabel
```