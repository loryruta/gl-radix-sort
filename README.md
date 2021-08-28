
# GPU radix sort

GPU radix sort implemented with OpenGL 4.6 and made a header-only library.

## Include it in your project
- Copy `generated/radix_sort.hpp` in your project.
- Replace the line `#include <glad/glad.h>`  with your own OpenGL 4.6 loading library include header.

Now you can use the radix sort through the following code:
```c++  
GLuint key_buf; // The GL_SHADER_STORAGE_BUFFER of keys to sort.  
GLuint val_buf; // The GL_SHADER_STORAGE_BUFFER of values that will be sorted along with the keys.  
size_t arr_len; // The length of both buffers (that should be the same).  
  
rgc::radix_sort::sorter sorter(arr_len);
sorter.sort(GLuint key_buf, GLuint val_buf, size_t arr_len);  
```  

## Benchmark
Number of elements | Elapsed time (µs) | Elapsed time (ms)
--- | --- | ---
100 | 10119 | 10
1000 | 5248 | 5
10000 | 5739 | 5
100000 | 10332 | 10
1000000 | 41106 | 41
10000000 | 352137 | 352
100000000 | 104114 | 104

Computed with the following hardware:
- AMD Ryzen 7 3700X
- NVIDIA GeForce RTX 2060 SUPER

## References
- http://www.heterogeneouscompute.org/wordpress/wp-content/uploads/2011/06/RadixSort.pdf
- https://vgc.poly.edu/~csilva/papers/cgf.pdf
- https://github.com/Devsh-Graphics-Programming/Nabla/tree/radix_sort_tmp
