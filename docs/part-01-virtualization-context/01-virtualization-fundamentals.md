# 🧑‍💻 PART 1 - Virtulization Context
## 🧩 What is Virtualization ? 
In general, and independent of the technology discussion, virtualization contains the concept of **abstraction**. That is, we can create an abstraction for our functions in some way. This abstraction can be in the form of a network or a virtual machine.
Virtualization essentially means creating an **abstraction layer** on top of real system resources; that is, resources that exist in reality (such as **CPU, memory, disk, and network card**) are made available to multiple environments or users in a virtual and divisible form. The level at which this abstraction layer is applied determines the type of virtualization. In other words, depending on the level of the system at which this layer is created (**hardware, operating system, or application**), the form and function of virtualization are different.
## 🔎 The Origins and History of Virtualization
Talking about the history and origins of virtualization would be very detailed, but here we will briefly talk about it to give you a fairly good overview.
Below we will examine 5 categories of concepts and the history of virtualization.
### 1️⃣ The Origins of Virtualization – IBM Mainframes
Virtualization began in the 1960s on IBM mainframe systems.

At **IBM’s Cambridge Scientific Center**, engineers worked on enabling multiple users to share extremely expensive mainframe hardware safely and efficiently. One of the programmers involved in early virtualization development was **Jim Rymarczyk**, who contributed to IBM’s VM systems evolution.

The key milestone was the development of :
- **CP-40 (1967)**
- **CP-67**
- Later commercialized as **VM/370**

These systems introduced the concept of a **Virtual Machine Monitor (VMM)** — what we now call a **hypervisor**.
more details : https://en.wikipedia.org/wiki/IBM_CP-40

### 2️⃣ Virtualization as Abstraction – The Java Virtual Machine
Virtualization does not require hardware emulation. 
In the 1990s, **Sun Microsystems** introduced a different form of virtualization through **Java**, which implemented the concept of **"Write Once, Run Anywhere."**.
This meant that a user could write a program in Java that would run on different hardware architectures. 
Java did this by introducing **bytecode** that was interpreted and executed on different hardware by the Java Runtime Environment. This was a turning point in the history of virtualization and was considered a form of process-level virtualization because the Java Runtime Environment virtually emulated the POSIX layer **(the standard interface between Unix-like operating systems).**

### 3️⃣ x86 Virtualization Challenges
Early x86 processors were not designed to be virtualizable under the classic **Popek and Goldberg virtualization requirements (1974).**
```
The Popek and Goldberg virtualization requirements are a set of conditions sufficient for a computer architecture to support system virtualization efficiently. They were introduced by Gerald J. Popek and Robert P. Goldberg in their 1974 article "Formal Requirements for Virtualizable Third Generation Architectures".
more deteils : https://en.wikipedia.org/wiki/Popek_and_Goldberg_virtualization_requirements
```
Certain privileged instructions did not trap properly when executed outside ring 0. This made pure hardware virtualization difficult on x86 until the early 2000s. 
Companies like **VMware** solved the problem using **dynamic binary translation.**

VMware’s early products:
- **VMware Workstation (1999)**
- **GSX** was a **Type 2 Hypervisor** that required a host operating system (such as Windows) to run guest operating systems.
- In contrast, **ESX** was a **Type 1 Hypervisor** that ran directly on the hardware and allowed guest operating systems to run directly on the Hypervisor.

### 4️⃣ Hardware-Assisted Virtualization (Intel VT-x / AMD-V)
Classical virtualization theory, formalized by Popek and Goldberg (1974), defines the requirements for a processor architecture to be virtualizable. One key requirement is that all sensitive (privileged) instructions must trap when executed outside the highest privilege level.Early x86 processors did not fully satisfy these requirements. Certain sensitive instructions behaved differently depending on privilege level but did not generate traps when executed in user mode. This made straightforward trap-and-emulate virtualization impossible.

To overcome this limitation, early virtualization platforms (notably VMware) relied on dynamic binary translation, rewriting problematic instruction sequences at runtime. While effective, this approach introduced complexity and overhead.

To address these architectural limitations, CPU vendors introduced hardware virtualization extensions in the mid-2000s:
- Intel VT-x
- AMD-V (SVM – Secure Virtual Machine)

These extensions added explicit architectural support for virtualization.



### 5️⃣ KVM – Virtualization in the Linux Kernel
KVM **(Kernel-based Virtual Machine)** is a virtualization infrastructure integrated directly into the Linux kernel.

It was merged into the mainline Linux kernel in version 2.6.20 (2007). KVM transforms the Linux kernel into a hypervisor by leveraging hardware virtualization extensions (VT-x / AMD-V).

### 