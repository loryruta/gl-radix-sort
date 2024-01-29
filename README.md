
# GLU: openGL Utilities

Ready to use primitives for parallel programming, implemented in OpenGL 4.6.

Currently, includes: Reduce, BlellochScan, RadixSort

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
#include <glad/glad.h>  // Must be placed earlier! 
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

Note: currently `val_buffer` is required and its type is `GLuint`.

## Performance

TODO

## References
- http://www.heterogeneouscompute.org/wordpress/wp-content/uploads/2011/06/RadixSort.pdf
- https://vgc.poly.edu/~csilva/papers/cgf.pdf
- https://github.com/Devsh-Graphics-Programming/Nabla/tree/radix_sort_tmp
