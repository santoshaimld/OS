#Single producer single consumer(SPSC)

SPSC demostrates a simple case of IPC with out any syncronization.
Kernel module is the single slot mailbox.

On purpose SPSC app/kernel module doesnt use any mutex/semaphore, to demostrate
the behaviour of single vs multiple prducers/consumers.

Just increasing the count of producers and consumers will break the sync.


SimpleIPC - Kernel Module
Single buffer is used to produce and consume.
wait_queue semantics `wait_event_interruptible` and `wake_up_interruptible`
are used wait/signal for full and empty buffer.
