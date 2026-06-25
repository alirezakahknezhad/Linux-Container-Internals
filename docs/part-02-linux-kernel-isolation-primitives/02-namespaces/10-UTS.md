# 🧑‍💻 Chapter 10 - UTS Namespace
## 🧩 Brief explanation
UTS stands for **Unix Timesharing System** Namespace and is one of the simplest but most basic types of Namespaces in Linux. This Namespace allows each process to have its own `hostname` and `NIS (Network Information Service)` domain name, separate from the one set in the main system (global namespace). This type of Namespace allows a process to see a different hostname than the one in the global namespace. In other words, each container or group of processes can be configured to see a different system name. As a result, multiple environments can run on the same Linux kernel, each seeing itself as a separate system with its own hostname.
### 🔎 How UTS works : 
1. When a UTS Namespace is created, it starts with a copy of the hostname and NIS Domain from its parent namespace. This means that at the time of creation, the new namespace shares the same identifiers as its parent. However, any subsequent changes to the hostname or NIS domain name in the namespace will not affect other namespaces.
2. Processes within a UTS Namespace can change the hostname and NIS Domain Name using the **`setdomainname`** and **`sethostname`** syscalls. These changes are local to the namespace and do not affect other namespaces or the host system.
3. Processes can switch between namespaces using the **`setns`** syscall or create a new namespace using the **`unshare or clone syscalls`** with the **`CLONE_NEWUTS`** flag. When a process moves to a new namespace or creates one, it starts using the hostname and NIS domain name associated with that namespace.


