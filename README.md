# uKernel
uKernel is a compact and very fast real-time kernel for the embedded 32 bits microprocessors. It performs a preemptive priority-based scheduling and a round-robin scheduling for the tasks with identical priority.

uKernel was born as a thorough review and re-implementation of TNKernel v2.7.

Currently it is available for the following architectures:

- ARM Cortex-M cores: Cortex-M0/M0+/M1/M3/M4 *(supported toolchains: ARM Compiler 4/5)*
- ARM 7/9 cores *(supported toolchains: ARM Compiler 4/5)*

The current version of uKernel includes semaphores, mutexes, data queues, event flags, fixed-sized memory pools and program timers.
The system functions calls in the interrupts are supported.

## License
uKernel is distributed in the source code form free of charge under the Apache-2.0 license.

## Contributions and Pull Requests
Contributions are accepted under Apache-2.0. Only submit contributions where you have authored all of the code.
