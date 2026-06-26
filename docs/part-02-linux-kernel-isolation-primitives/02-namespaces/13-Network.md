# 🧑‍💻 Chapter 13 - Network Namespace
## 🧩 Brief explanation
Net Namespace is one of the most vital and widely used isolation features in Linux. This type of Namespace allows each container or process to see and use a completely separate set of network resources. Such as **network interfaces, IP addresses, routing tables, and iptables rules**. In essence, Network Namespace is what allows each container to act like an independent system with its own network.
In normal mode (without namespaces), a Linux system has only one shared Net Namespace. All processes have access to the same set of interfaces, firewall rules, routes, etc.
But when a new Network Namespace is created for a process : 
- that process has an independent network stack
- sees its own addresses
- routes, and interfaces
- and is communicationally isolated from other namespaces.

### 🔎 Kernel Implementation
#### 🧩 net_namespace.h
This file is the core of the Linux Network Namespace implementation. In fact, this file defines the main data structure associated with each network namespace (`struct net`), provides helper functions for creating, getting, freeing, and managing the reference count of the network space, and provides generic interfaces to other kernel networking components (IPv4, IPv6, Netfilter, XFRM, BPF, etc.).

Each Network Namespace in Linux is represented by an object of type `struct net`, and this file defines all the components of that structure and its management functions.
This file is located in the path `/usr/src/linux-headers-$(uname -r)/include/net/net_namespace.h`
Its most important functions are as follows : 
| function | Explanation |
| -------- | ----------- |
| `copy_net_ns(u64 flags, struct user_namespace *user_ns, struct net *old_net)` | Create a new Network Namespace (during clone(CLONE_NEWNET) or unshare --net) |
| `get_net(struct net *net)` | Increment reference count to prevent premature namespace deletion |
`put_net(struct net *net)` | Decrement the reference counter; when it reaches zero, the namespace is freed.
`get_net_ns_by_pid(pid_t pid)` | Get network namespace based on PID of a process
| `get_net_ns_by_fd(int fd)` | Getting namespace via file descriptor /`proc/[pid]/ns/net` |
| `register_pernet_subsys(struct pernet_operations *)` | Registering a network subsystem (such as IPv6 or Netfilter) to be initialized in each namespace separately | 
| `unregister_pernet_subsys(...)` | Unregister network subsystem |
| `net_initialized(struct net *net)` | Check whether the network namespace has been successfully initialized |

There are various examples and network scenarios for creating and using network namespaces. You can see some interesting examples in the link below : 

**[🔗 Network Namespace](https://github.com/rezafarhadur/network-namespace)**

In the following example, we will configure a Net Namespace as Point-to-Point. This configuration is similar to using a crossover cable when connecting two network interface controllers.

To do this, we'll set up a few Environment Variables to make it easier to work with and the code more repeatable. You can create multiple variables at once by putting the required variables in a simple text file and then importing them into your shell using the source command (or a dot):
```bash
root@rootium:~# cat << EOF >> vars
> namespace1=client
> namespace2=server
> command='python3 -m http.server'
> ip_address1="10.10.10.10/24"
> ip_address2="10.10.10.20/24"
> interface1=veth-client
> interface2=veth-server
> EOF
root@rootium:~# source vars
```
For this example, the server is running a basic Python 3 web server. We just need to verify that the connection is up.
The first thing we need to do is create the namespace. Unlike other commands, we can access the net namespace directly from the ip command. We create the namespaces:
```bash
root@rootium:~# ip netns add $namespace1
root@rootium:~# ip netns add $namespace2
root@rootium:~# ip netns list
server
client
```
If you run the command in one of the new namespaces, you will see that only the `loopback` address is present and marked as `DOWN` :
```bash
root@rootium:~# ip netns exec $namespace2 ip ad
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
```
The next step is to create a virtual "Ethernet cable" by creating a link between the two namespaces, like this : 
```bash
root@rootium:~# ip link add \ 
> ptp-$interface1 \
> type veth \
> peer name ptp-$interface2
```
If we run the `ip link` command on the host, we will see two additional links created by this command:
```bash
root@rootium:~# ip link
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: ens33: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP mode DEFAULT group default qlen 1000
    link/ether 00:0c:29:c2:2e:e9 brd ff:ff:ff:ff:ff:ff
    altname enp2s1
    altname enx000c29c22ee9
3: docker0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state DOWN mode DEFAULT group default 
    link/ether ba:b2:8e:26:37:42 brd ff:ff:ff:ff:ff:ff
4: br-e240251c399a: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state DOWN mode DEFAULT group default 
    link/ether 52:33:40:5a:28:5c brd ff:ff:ff:ff:ff:ff
5: ptp-veth-server@ptp-veth-client: <BROADCAST,MULTICAST,M-DOWN> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether a6:0d:59:15:7e:b2 brd ff:ff:ff:ff:ff:ff
6: ptp-veth-client@ptp-veth-server: <BROADCAST,MULTICAST,M-DOWN> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether 6e:f8:f9:06:79:a1 brd ff:ff:ff:ff:ff:ff
```
At this point, we have created the links but not assigned them anywhere. Let's go ahead and assign the interfaces to their respective namespaces.
```bash
root@rootium:~# ip link set ptp-$interface1 netns $namespace1
root@rootium:~# ip link set ptp-$interface2 netns $namespace2
```
After running this command, the Host no longer has access to these links because they are assigned to a different Network Namespace. If we run the ip netns exec command again, we can see that our new namespaces have devices, but they are still marked as `DOWN` and do not have IP addresses to communicate with : 
```bash
root@rootium:~# ip link
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: ens33: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP mode DEFAULT group default qlen 1000
    link/ether 00:0c:29:c2:2e:e9 brd ff:ff:ff:ff:ff:ff
    altname enp2s1
    altname enx000c29c22ee9
3: docker0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state DOWN mode DEFAULT group default 
    link/ether ba:b2:8e:26:37:42 brd ff:ff:ff:ff:ff:ff
4: br-e240251c399a: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state DOWN mode DEFAULT group default 
    link/ether 52:33:40:5a:28:5c brd ff:ff:ff:ff:ff:ff
```
Now, we assign IPs and bring up the interfaces : 
```bash
root@rootium:~# ip netns exec $namespace1 ip addr add $ip_address1 dev ptp-$interface1
root@rootium:~# ip netns exec $namespace2 ip addr add $ip_address2 dev ptp-$interface2
root@rootium:~# ip netns exec $namespace1 ip link set dev ptp-$interface1 up
root@rootium:~# ip netns exec $namespace2 ip link set dev ptp-$interface2 up
```
Finally, we run the Python 3 web server in namespace 2 and test it : 
```bash
root@rootium:~# ip netns exec $namespace2 $command &
[1] 4502
root@rootium:~# Serving HTTP on :: port 8000 (http://[::]:8000/) ...
```
And finally we will have : 
```bash
root@rootium:~# ip netns exec $namespace1 curl 10.10.10.20:8000
<!DOCTYPE HTML>
<html lang="en">
<head>
<meta charset="utf-8">
<style type="text/css">
:root {
color-scheme: light dark;
}
</style>
<title>Directory listing for /</title>
</head>
<body>
<h1>Directory listing for /</h1>
<hr>
<ul>
<li><a href=".bash_history">.bash_history</a></li>
<li><a href=".bashrc">.bashrc</a></li>
<li><a href=".local/">.local/</a></li>
<li><a href=".profile">.profile</a></li>
<li><a href=".ssh/">.ssh/</a></li>
<li><a href=".wget-hsts">.wget-hsts</a></li>
<li><a href="snap/">snap/</a></li>
<li><a href="vars">vars</a></li>
</ul>
<hr>
</body>
</html>
```