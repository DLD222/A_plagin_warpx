# A0 API catalog — Runtime

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> Generated; 1 classes/structs. Signatures are heuristic.

## `Runtime`

- file: `Source/Runtime/Runtime.H`
- kind: `class`
- public methods: 10; public members: 0

```cpp
Runtime (int argc, char* argv[])
~Runtime ()
Runtime (Runtime const &) = delete
Runtime& operator= (Runtime const &) = delete
Runtime (Runtime &&) = delete
Runtime& operator= (Runtime &&) = delete
void initialize ()
void evolve (int numsteps = -1)
void finalize ()
[[nodiscard]] bool verbose () const
```

