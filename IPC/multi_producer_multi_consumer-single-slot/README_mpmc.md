#Multi producer multi consumer(MPMC)

MPMC demostrates a simple case of IPC.
Unline SPSC, it uses mutex to solve the critical section
problem.

Kernel module is the single slot mailbox.

SimpleIPC - Kernel Module
Multi buffer is used to produce and consume.
wait_queue semantics `wait_event_interruptible` and `wake_up_interruptible`
are used wait/signal for full and empty buffer.
