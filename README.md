# Shortest Path Routing Algorithm

### Get Started

1. Compile the program
```
$ make
```

2. On the host __host1__:
```
$ ./nse-linux386 <host2> <port0>
```

3. On the host __host2__:
```
$ ./router 1 <host1> <port0> <port1>
$ ./router 2 <host1> <port0> <port2>
$ ./router 3 <host1> <port0> <port3>
$ ./router 4 <host1> <port0> <port4>
$ ./router 5 <host1> <port0> <port5>
```
4. Check output files `router(n).log`

### Tested Machines

- host1 `ubuntu1604-002.student.cs.uwaterloo.ca`
- host2 `ubuntu1604-006.student.cs.uwaterloo.ca`

### Versions

- Dev Environment: `macOS High Sierra 10.13.6`
- C++ Version: `C++14`
- Compiler version: `gcc version 5.5.0 20171010`
