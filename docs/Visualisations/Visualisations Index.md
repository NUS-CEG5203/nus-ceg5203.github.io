# Visualisations Index

Browser-based simulators for topics from the lectures. Each one runs a small example cycle by cycle, so you can see what the hardware or software is doing at every step, and change the parameters to see what that does to the result. Each has a short guide with suggestions for what to try.

| Simulator | What you can explore | Guide |
|---|---|---|
| [Cache](cache.html) | Run an access trace through a cache of your choosing. Compare direct-mapped, set-associative and fully associative mapping; LRU, FIFO and optimal replacement; write-back and write-through. Each miss is classified as compulsory, capacity, conflict or coherence. | [cache.md](cache.md) |
| [Interrupts](interrupt.html) | Raise an interrupt and follow it through the context switch, the ISR and the return, in vectored and non-vectored mode. | [interrupt.md](interrupt.md) |
| [Multithreading](multithread.html) | Two threads on one superscalar core under no multithreading, coarse-grained, fine-grained, barrel and SMT policies. See which issue slots go unused and why. | [multithread.md](multithread.md) |
| [Data parallelism](dataparallel.html) | The same loop on scalar, SIMD and SIMT hardware. Warp divergence, latency hiding through warp scheduling, and memory coalescing. | [dataparallel.md](dataparallel.md) |
| [Systolic array](systolic.html) | Matrix multiplication on an n × n array, output-stationary and weight-stationary (TPU-style). Fill and drain time, PE utilisation, operand reads. | [systolic.md](systolic.md) |
| [OpenCL](opencl.html) | One OpenCL host program from device discovery to read-back, with work-groups executing on compute units. | [opencl.md](opencl.md) |
