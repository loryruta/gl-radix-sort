
# GPU radix sort

GPU radix sort implemented with OpenGL 4.6 and made a header-only library.

## Use it

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

## Performance analysis
Number of elements | Elapsed time (µs) | Elapsed time (ms)
--- | --- | ---  
256 | 4247 | 4
512 | 3350 | 3
1024 | 3146 | 3
2048 | 2885 | 2
4096 | 2988 | 2
8192 | 2938 | 2
16384 | 3027 | 3
32768 | 3946 | 3
65536 | 5277 | 5
131072 | 8408 | 8
262144 | 13442 | 13
524288 | 21333 | 21
1048576 | 40561 | 40
2097152 | 78820 | 78
4194304 | 148920 | 148
8388608 | 296441 | 296
16777216 | 197146 | 197
33554432 | 344191 | 344

## References
- http://www.heterogeneouscompute.org/wordpress/wp-content/uploads/2011/06/RadixSort.pdf
- https://vgc.poly.edu/~csilva/papers/cgf.pdf
- https://github.com/Devsh-Graphics-Programming/Nabla/tree/radix_sort_tmp
