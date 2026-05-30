# RTOS_lite_cpp

Lite RTOS for small, resource limited devices and projects
<<<<<<< Updated upstream
=======

# Build

```sh
mkdir build
cd build
cmake ..
make
```

OR

```sh
mkdir build
cd build
cmake -G Ninja ..
ninja
```

## Documentation

## Explanation

### Mutex

A **mutex** (mutual exclusion lock) is a synchronization object that allows only **one task/thread** at a time to access a shared resource.

### What it is responsible for

- Preventing **race conditions** on shared data/peripherals.
- Protecting **critical sections** so concurrent tasks do not corrupt state.
- Enforcing **ownership**: only the task that locked it can unlock it.
- Optionally supporting **priority inheritance** (in RTOS) to reduce priority inversion.

In the RTOS-lite, we use a mutex when multiple tasks access the same global variable, buffer, or hardware interface.

> > > > > > > Stashed changes
