# 🧑‍💻 Chapter 13 - IPC Namespace
## 🧩 Brief explanation

In the Linux operating system, each process possesses its own **separate address space** and is unaware of the memory belonging to other processes. For this reason, Linux introduces a mechanism known as **IPC (Inter-Process Communication)** to address the following three primary needs:
- **Inter-process data transfer**
- **Shared memory**
- **Synchronization and notification**

Linux provides several IPC mechanisms, including : 

| IPC Mechanism               | Description                                                                                                                              |
| --------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| **Pipes**                   | Unidirectional byte stream used for communication between related processes, typically a parent and child.                               |
| **FIFOs (Named Pipes)**     | Pipe-like IPC mechanism represented by a filesystem path, allowing unrelated processes to communicate.                                   |
| **UNIX Domain Sockets**     | Local socket-based communication between processes on the same host; supports stream and datagram communication.                         |
| **Signals**                 | Asynchronous notifications delivered to processes to indicate events such as termination, interruption, or configuration changes.        |
| **POSIX Message Queues**    | Kernel-managed queues that allow processes to exchange discrete, prioritized messages.                                                   |
| **System V Message Queues** | Kernel-managed message queues using the System V IPC API for exchanging typed messages between processes.                                |
| **System V Semaphores**     | Kernel-managed synchronization primitives used to coordinate access to shared resources between processes.                               |
| **System V Shared Memory**  | Shared memory regions that allow multiple processes to directly access the same physical memory for high-performance data exchange.      |
| **POSIX Shared Memory**     | Shared-memory objects exposed through the POSIX API, commonly used with `shm_open()` and memory mapping via `mmap()`.                    |
| **eventfd**                 | Lightweight kernel event/counter mechanism used for efficient event notification between processes and threads.                          |
| **signalfd**                | Provides signals through a file descriptor, allowing applications to handle signals using normal file-descriptor-based event mechanisms. |
| **memfd**                   | Creates anonymous, RAM-backed file descriptors that can be used for sharing memory or passing memory-backed objects between processes.   |
| **Futexes**                 | Fast user-space synchronization primitives backed by the kernel when contention requires blocking or waking threads/processes.           |

If two processes in different namespaces attempt to communicate via IPC, that communication is restricted. With the advent of containers, it became necessary to limit and isolate these interactions so that processes within one container could not access the shared resources of another.

**❗ Two processes can communicate via IPC only if they share the same IPC namespace.**



### 🔎 Kernel Implementation
#### 🧩 net_namespace.h
The purpose of this file is to define the data structure and associated functions for the IPC namespace in the Linux kernel. Each IPC namespace within the kernel is represented by an object of type `struct ipc_namespace`. This file specifies the data contained within each namespace (such as message queues, semaphores, and shared memory) and provides functions for creating, acquiring, freeing, and managing its reference count.


To view the list of IPC namespaces, we use the following command : 
```bash
root@rootium:~# lsns -t ipc
        NS TYPE NPROCS   PID USER COMMAND
4026531839 ipc     357     1 root /usr/lib/systemd/systemd --switched-root --system --deserialize=52 splash
4026532905 ipc       1  4540 root registry serve /etc/distribution/config.yml

```
Next, we **create a System V IPC shared memory segment** using the following command. This command—short for "IPC Make"—is a tool for creating Inter-Process Communication (IPC) objects such as `message queues`, `shared memory`, and `semaphores`. The `-m` flag specifies that a shared memory segment is to be created, while `10` represents the size of the memory to be allocated, in bytes. Upon executing this command, the kernel creates a new shared memory segment with a size of 10 bytes.
```bash
root@rootium:~# ipcmk -M 10
Shared memory id: 1
```
- The returned value is the identifier assigned by the kernel to this memory segment, and other processes can access it using this identifier (via shmat, shmctl, ipcrm, or ipcs).

Next, we want to create a `System V IPC Message Queue`. This command creates a new message queue in the kernel, allowing processes to send and receive messages through it. Upon execution, the kernel creates a new message queue and returns a numeric identifier (**msqid**) for it.
```bash
root@rootium:~# ipcmk -Q
Message queue id: 0
```
Next, we have the **`ipcs`** command. In Linux, `ipcs` is used to display the status and details of Inter-Process Communication (IPC) objects. When executed without any options, the kernel displays a list of all IPC resources present on the system (or within the current namespace). This command shows which message queues, shared memory segments, and semaphores are currently created on the system, along with details regarding their owner, ID, and status.
```bash
root@rootium:~# ipcs

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages    
0xb7c58b78 0          root       644        0            0           

------ Shared Memory Segments --------
key        shmid      owner      perms      bytes      nattch     status      
0xbf491931 1          root       644        10         0                       

------ Semaphore Arrays --------
key        semid      owner      perms      nsems  
```
## 🛡️ Explanation of IPC at the system level and analysis of the Cross-Container Shared Memory Snooping attack.
Suppose we have two containers: the first contains an application that writes a secret key to memory, while the second runs a suspicious process attempting to read that secret key.

Our first code snippet is as follows : 
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

int main(void)
{
    printf("[1] Starting writer...\n");

    /* Generate System V IPC key */
    key_t key = ftok("/tmp", 65);

    if (key == (key_t)-1) {
        perror("[ERROR] ftok");
        return EXIT_FAILURE;
    }

    printf("[2] ftok() succeeded\n");
    printf("    key = %d\n", key);

    /* Create or get shared memory segment */
    int shmid = shmget(key, 1024, 0666 | IPC_CREAT);

    if (shmid == -1) {
        perror("[ERROR] shmget");
        return EXIT_FAILURE;
    }

    printf("[3] shmget() succeeded\n");
    printf("    shmid = %d\n", shmid);

    /* Attach shared memory to process address space */
    char *data = shmat(shmid, NULL, 0);

    if (data == (char *)-1) {
        perror("[ERROR] shmat");
        
        /* Cleanup */
        shmctl(shmid, IPC_RMID, NULL);

        return EXIT_FAILURE;
    }

    printf("[4] shmat() succeeded\n");
    printf("    address = %p\n", (void *)data);

    /* Write data */
    const char *secret = "SECRET_API_KEY=Secret : $3cretKeY!";

    strcpy(data, secret);

    printf("[5] Data written successfully\n");
    printf("    data = %s\n", data);

    printf("[6] Shared memory is ready\n");
    printf("    Sleeping for 60 seconds...\n");

    sleep(60);

    /* Detach */
    if (shmdt(data) == -1) {
        perror("[ERROR] shmdt");
    } else {
        printf("[7] shmdt() succeeded\n");
    }

    /* Remove shared memory segment */
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("[ERROR] shmctl(IPC_RMID)");
    } else {
        printf("[8] Shared memory removed\n");
    }

    printf("[9] Writer finished\n");

    return EXIT_SUCCESS;
}
```
This program is the writer side of a System V shared-memory IPC example. Its purpose is to create a shared-memory segment, attach it to its address space, write data into it, keep it alive for 60 seconds so another process can access it, and finally clean it up.

### 🔎 Code flow
| Step | Code                                   | What it does                                                                                                                                                       |
| ---- | -------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 1    | `ftok("/tmp", 65)`                     | Generates a System V IPC key from `/tmp` and the project identifier `65`. Both processes need to generate the same key to locate the same IPC object.              |
| 2    | `shmget(key, 1024, 0666 \| IPC_CREAT)` | Creates a **1 KB System V shared-memory segment**, or obtains the existing segment associated with `key`. `0666` sets permissions; `IPC_CREAT` allows creation.    |
| 3    | `shmat(shmid, NULL, 0)`                | Attaches the shared-memory segment to the process's virtual address space. It returns the address where the shared memory is mapped.                               |
| 4    | `strcpy(data, secret)`                 | Writes the string into the shared-memory region. This data can subsequently be read by another process that attaches to the same segment.                          |
| 5    | `sleep(60)`                            | Keeps the shared-memory segment available for 60 seconds, giving the second program time to attach and read the data.                                              |
| 6    | `shmdt(data)`                          | Detaches the shared-memory mapping from this process's address space.                                                                                              |
| 7    | `shmctl(shmid, IPC_RMID, NULL)`        | Marks the System V shared-memory segment for removal. Since this program has already detached, it will normally disappear after the relevant attachments are gone. |

The crucial point is that the two processes do not directly share their normal address spaces. Instead, the kernel maps the same System V shared-memory object into both processes' virtual address spaces.

Now, let's examine the second code : 
```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main(void)
{
    printf("[1] Starting snooper...\n");

    key_t key = ftok("/tmp", 65);

    if (key == (key_t)-1) {
        perror("[ERROR] ftok");
        return EXIT_FAILURE;
    }

    printf("[2] ftok() succeeded\n");
    printf("    key = %d\n", key);

    int shmid = shmget(key, 1024, 0666);

    if (shmid == -1) {
        perror("[ERROR] shmget");
        return EXIT_FAILURE;
    }

    printf("[3] shmget() succeeded\n");
    printf("    shmid = %d\n", shmid);

    char *data = shmat(shmid, NULL, SHM_RDONLY);

    if (data == (char *)-1) {
        perror("[ERROR] shmat");
        return EXIT_FAILURE;
    }

    printf("[4] shmat() succeeded\n");
    printf("    address = %p\n", (void *)data);

    printf("[5] Reading shared memory...\n");
    printf("    data = %s\n", data);

    if (shmdt(data) == -1) {
        perror("[ERROR] shmdt");
        return EXIT_FAILURE;
    }

    printf("[6] shmdt() succeeded\n");

    return EXIT_SUCCESS;
}
```
It is a System V shared memory reader. It locates an existing segment created by another process, attaches it read-only, prints its contents, and detaches.
### 🔎 Code flow
| Step | Code                                   | What it does                                                                                                                                                       |
| ---- | -------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 1    | `ftok("/tmp", 65)`                     | Generates the same System V IPC key as the first program, because the path (/tmp) and project identifier (65) match. This shared key is how the two unrelated processes find the same IPC object.              |
| 2    | `shmget(key, 1024, 0666)` | Looks up the existing shared-memory segment associated with `key`. There is no `IPC_CREAT`, so nothing is created. If the first program hasn't created the segment yet (or has already removed it), the call fails with `ENOENT`. Access is checked against the segment's real permissions.    |
| 3    | `shmat(shmid, NULL, SHM_RDONLY)`                | Attaches the segment to this process's virtual address space and returns the mapped address. `SHM_RDONLY` makes the mapping read-only, so a write through `data` would crash the process with `SIGSEGV`.                               |
| 4    | `printf("%s", data)`                 | Reads the string the first program wrote with `strcpy`. This is a direct memory read with no copy through the kernel. It relies on the writer having stored a NUL terminator.                          |
| 5    | `shmdt(data)`                            | Detaches the mapping from this process only. The segment itself is untouched, and its removal is up to the first program's `IPC_RMID.`                                              |
| 6    | `return EXIT_SUCCESS`                          | The process exits cleanly. Any error in steps 1 to 5 prints a message with perror() and returns `EXIT_FAILURE` instead.                                                                                              |

In the default state and without regard to whether this code resides within a container or a specific IPC namespace it will execute on the host machine as follows : 

**Terminal - 1 :**
```bash
root@rootium:~# gcc ipc-1.c -o ipc-1
root@rootium:~# ./ipc-1 
[1] Starting writer...
[2] ftok() succeeded
    key = 1093664769
[3] shmget() succeeded
    shmid = 2
[4] shmat() succeeded
    address = 0x76b19a516000
[5] Data written successfully
    data = SECRET_API_KEY=P@ss$ec&3et
[6] Shared memory is ready
    Sleeping for 60 seconds...
```
**Terminal - 2 :**
```bash
root@rootium:~# gcc ipc-2.c -o ipc-2
root@rootium:~# ./ipc-2 
[1] Starting snooper...
[2] ftok() succeeded
    key = 1093664769
[3] shmget() succeeded
    shmid = 2
[4] shmat() succeeded
    address = 0x74fd2680c000
[5] Reading shared memory...
    data = SECRET_API_KEY=P@ss$ec&3et
[6] shmdt() succeeded
```
Now, let's see what happens if we run the second program in a different IPC namespace.

**❗ Now, let's see what happens if we run the second program in a different IPC namespace.**

**Terminal - 1 :**
```bash
root@rootium:~# unshare --ipc /bin/bash 
root@rootium:~# ./ipc-1 
[1] Starting writer...
[2] ftok() succeeded
    key = 1093664769
[3] shmget() succeeded
    shmid = 3
[4] shmat() succeeded
    address = 0x79ce0b5b8000
[5] Data written successfully
    data = SECRET_API_KEY=P@ss$ec&3et
[6] Shared memory is ready
    Sleeping for 60 seconds...
```

**Terminal - 2 :**
```bash
root@rootium:~# ./ipc-2 
[1] Starting snooper...
[2] ftok() succeeded
    key = 1093664769
[ERROR] shmget: No such file or directory
```

***As you can see, this code snippet is attempting to access shared memory that does not exist within it.***

### 🐳 IPC Namespace in Docker
When we create a container using Docker, a dedicated IPC namespace is created for that container.

**Terminal - 1 :**
```bash
root@rootium:~# docker run --rm -it --name container-1 ubuntu bash
```
**Terminal - 2 :**
```bash
root@rootium:~# docker run --rm -it --name container-2 ubuntu bash
```
**Terminal - 3 :**
```bash
root@rootium:~# docker inspect --format '{{.State.Pid}}' container-1
150468
root@rootium:~# readlink /proc/150468/ns/ipc 
ipc:[4026532960]
root@rootium:~# docker inspect --format '{{.State.Pid}}' container-2
159184
root@rootium:~# readlink /proc/159184/ns/ipc 
ipc:[4026533387]
```
**❗Given the distinct IPC namespaces, as expected, the second module component within one container cannot access the secret key inside the other container.**

**❗ The key question now is how a container can be vulnerable to cross-container shared memory snooping.**

In a Docker container, the following flag can be used to allow the container to access an IPC object from another container : 

**Terminal - 1 :**
```bash
root@rootium:~# docker run --rm -it -v ./:/tmp/ --ipc=shareable --name container-1 ubuntu bash
root@9593333ef0f5:/# 
```
**Terminal - 2 :**
```bash
root@rootium:~# docker run --rm -it -v ./:/tmp/ --ipc=container:container-1 --name container-2 ubuntu bash
root@ed29468b4499:/# 
```
**Terminal - 3 :**
```bash
root@rootium:~# docker inspect --format '{{.State.Pid}}' container-1
166132
root@rootium:~# readlink /proc/166132/ns/ipc 
ipc:[4026532960]
root@rootium:~# docker inspect --format '{{.State.Pid}}' container-2
166540
root@rootium:~# readlink /proc/166540/ns/ipc 
ipc:[4026532960]
```
### 🛡️ Cross-Container Shared Memory Snooping Attack.
As you can see, these containers now share the same IPC namespace, making it possible to carry out the attack :

**Terminal - 1 :**
```bash
root@9593333ef0f5:/# echo "Container-1"
Container-1
root@9593333ef0f5:/# /tmp/ipc-1
[1] Starting writer...
[2] ftok() succeeded
    key = 1090670609
[3] shmget() succeeded
    shmid = 1
[4] shmat() succeeded
    address = 0x70f651d6f000
[5] Data written successfully
    data = SECRET_API_KEY=P@ss$ec&3et
[6] Shared memory is ready
    Sleeping for 60 seconds...
```

**Terminal - 2 :**
```bash
root@043ee94f103a:/# echo "Container-2"
Container-2
root@043ee94f103a:/# /tmp/ipc-2 
[1] Starting snooper...
[2] ftok() succeeded
    key = 1090670609
[3] shmget() succeeded
    shmid = 1
[4] shmat() succeeded
    address = 0x7fa87d948000
[5] Reading shared memory...
    data = SECRET_API_KEY=P@ss$ec&3et
[6] shmdt() succeeded
```