
# gl-radix-sort

Ready to use RadixSort and other parallel programming primitives, implemented with OpenGL 4.6.

Includes:
- Parallel Reduce
- Parallel BlellochScan
- Parallel RadixSort

Such modules are grouped together under the name "GLU" (OpenGL Utilities).

## Requirements

- C++17

## How to include it

### Copy-paste the utility file

- Look into the `dist/` directory
- Take a utility (e.g. `dist/Reduce.hpp`)
- Copy the file in your codebase
- Include it where you need it
  - **Important: OpenGL 4.6 symbols must be defined prior the utility file!** E.g.:

```cpp
#include <glad/glad.h>  // Must be placed beforehand! 
#include "Reduce.hpp"
```

### git submodule + CMake

- Git submodule this project into your codebase
- Add CMake subdirectory and link to the `glu` target

```cmake
add_subdirectory(path/to/glu)

target_link_libraries(your_project PUBLIC glu)
```

## How to use it

### Reduce

```cpp
#include "Reduce.hpp"

using namespace glu;

size_t N;
GLuint buffer;  // SSBO containing N GLuint (of size N * sizeof(GLuint))

Reduce reduce(DataType_Uint, ReduceOperator_Sum);
reduce(buffer, N);
```

### BlellochScan

```cpp
#include "Blelloch.hpp"

using namespace glu;

size_t N;       // Important: N must be a power of 2
GLuint buffer;  // SSBO containing N GLuint (of size N * sizeof(GLuint))

BlellochScan blelloch_scan(DataType_Uint);
blelloch_scan(buffer, N);
```

### RadixSort

```cpp
#include "RadixSort.hpp"

using namespace glu;

size_t N;
GLuint key_buffer;  // SSBO containing N GLuint (of size N * sizeof(GLuint))
GLuint val_buffer;  // SSBO containing N GLuint (of size N * sizeof(GLuint))

RadixSort radix_sort;
radix_sort(buffer, N);
```

Note: currently `val_buffer` is **required** and its type is `GLuint`. If you have a keys array you would have to
allocate a dummy values array!

## Performance

- OS: Ubuntu 22.04
- CPU: AMD Ryzen 7 3700X 8-Core Processor
- Device: NVIDIA GeForce RTX 2060 SUPER/PCIe/SS
- NVIDIA Driver Version: 545.23.08
- RAM: 16 GB

```
Reduce; Num elements: 1024, Elapsed: 0.069 ms
Reduce; Num elements: 16384, Elapsed: 0.012 ms
Reduce; Num elements: 65536, Elapsed: 0.016 ms
Reduce; Num elements: 131072, Elapsed: 0.020 ms
Reduce; Num elements: 524288, Elapsed: 0.029 ms
Reduce; Num elements: 1048576, Elapsed: 0.049 ms
Reduce; Num elements: 16777216, Elapsed: 0.620 ms
Reduce; Num elements: 67108864, Elapsed: 2.514 ms
Reduce; Num elements: 134217728, Elapsed: 5.030 ms
Reduce; Num elements: 268435456, Elapsed: 10.044 ms
BlellochScan; Num elements: 1024, Elapsed: 1.108 ms
BlellochScan; Num elements: 16384, Elapsed: 0.081 ms
BlellochScan; Num elements: 65536, Elapsed: 0.101 ms
BlellochScan; Num elements: 131072, Elapsed: 0.115 ms
BlellochScan; Num elements: 524288, Elapsed: 0.168 ms
BlellochScan; Num elements: 1048576, Elapsed: 0.360 ms
BlellochScan; Num elements: 16777216, Elapsed: 4.368 ms
BlellochScan; Num elements: 67108864, Elapsed: 18.407 ms
BlellochScan; Num elements: 134217728, Elapsed: 37.167 ms
BlellochScan; Num elements: 268435456, Elapsed: 86.493 ms
Radix sort; Num elements: 1024, Elapsed: 0.663 ms
Radix sort; Num elements: 16384, Elapsed: 1.004 ms
Radix sort; Num elements: 65536, Elapsed: 1.761 ms
Radix sort; Num elements: 131072, Elapsed: 3.074 ms
Radix sort; Num elements: 524288, Elapsed: 10.633 ms
Radix sort; Num elements: 1048576, Elapsed: 20.457 ms
Radix sort; Num elements: 2097152, Elapsed: 39.688 ms
Radix sort; Num elements: 4194304, Elapsed: 78.594 ms
Radix sort; Num elements: 8388608, Elapsed: 0.156 s
Radix sort; Num elements: 16777216, Elapsed: 0.311 s
Radix sort; Num elements: 33554432, Elapsed: 0.626 s
Radix sort; Num elements: 67108864, Elapsed: 1.252 s
Radix sort; Num elements: 134217728, Elapsed: 2.518 s
Radix sort; Num elements: 268435456, Elapsed: 5.022 s
```

## Useful resources
- http://www.heterogeneouscompute.org/wordpress/wp-content/uploads/2011/06/RadixSort.pdf
- https://vgc.poly.edu/~csilva/papers/cgf.pdf
- Nabla (radix sort implementation): https://github.com/Devsh-Graphics-Programming/Nabla
- Udacity parallel programming guide: https://www.youtube.com/playlist?list=PLAwxTw4SYaPnFKojVQrmyOGFCqHTxfdv2
