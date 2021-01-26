# Thread-Safe-Malloc
```
Implement two different thread-safe versions (i.e. safe for concurrent access by different threads
of a process) of the malloc() and free() functions. Both of the thread-safe malloc and free functions
use the best fit allocation policy.
```
### Function Description:
```
There is a single linked list with node containing the metadata pre, next, and size to track the free memory.

To begin with, the user mallocs a chunk of memory, my function will call sbrk to get some memory on heap.

Then after several mallocs, when the user first calles free, this chunk of free memory will be tracked by the 
head pointer which points to the head of the free list.

When later the user calles malloc, the program will firstly check that if there is enough free memory insite 
the linked list according to best-fit strategy. If there is free block with enough space, that node will be 
removed from the linked list (As it is malloced), and if there is still enough space remaining in the current
node, split it into two parts, the remaining parts will become a smaller node in the linked list. If there is
no such large free block, the program will call sbrk again and return the newly malloced space.

Later if the user calles free, the freed memory block will be connected into the linked list according to its
position in the memory. And if there is consecutive free block, they will be merged in order to decrease the 
overhead of free memory segmentations and solve the problem of poor region selection during malloc.

```
