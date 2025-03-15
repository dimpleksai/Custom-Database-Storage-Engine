# **BUFFER MANAGER**

## Assignment 1 
## ADO CS525 Summer 2023 : Group 15


## Team  and respective contribution
```
1. Name : Jie Kang (A20528112)jkang34@hawk.iit.edu 
   Work : handle pool of buffer manager interface

2. Kanakam Sai Dimple (A20516770) dkanakamsai@hawk.iit.edu
   Work : access pages of buffer manager interface, page replacement strategies

3. Marwan Omar (A20522313) momar3@iit.edu
   Work : Statistics Interfaces
```

## How to execute

Firstly enter the directory in the terminal where all the files are present and  run

1. Execute 'make clean' command: This will clear out all the output files

2. Execute the 'make' command : This will execute everything in the make file in order to build the code. 

3. Execute './test_assign2_1' : The above command creates a binary file for test_assign1 which can be executed this way to run all the tests

4. We confirm the code works when we get the 'OK: finished test' upon executing './test_assign2_1' 

## Buffer Manager Functionality 

The following gives the explanation for the functionality of the buffer manager that we have implemented. There were some minor changes we had to initially make to our storage manager file, in order to pass all testcases.


First of all we have made use of a doubly linked list to implement the buffer pool so that we can easily manipulate the frame order as needed in the page replacement strategies.

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)

- We have initialized all values here, basically the file manager, file page handle, buffermanager, buffer page handle, the doubly linked list

RC shutdownBufferPool(BM_BufferPool *const bm)

- We first check if its already shut down
- then we make sure the pool doesnt have pinne dpages
- then we invoke foreflush to get rid of dirty pages
- we iterate over the linked list and free memory of each node

RC forceFlushPool(BM_BufferPool *const bm)

- we iterate pool to find dirty pages
- we invoke forcepage to write it to disk

----

RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)

- we iterate pool to find given page
- if we find then we increment dirty counter to mark dirty
- if page not found we throw error

RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)

- we iterate pool to find given page
- if we find then we decrement fix count counter to unpin
- if page not found we throw error

RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)

- - we iterate pool to find given page
- if we find then we set dirty counter to not dirty
- write to disk/ file using write block function
- if page not found we throw error


RC pinPage(BM_BufferPool * const bm, BM_PageHandle * const page,const PageNumber pageNum)

- we iterate over pool to find given page
- if yes we update position of page according to replacement strategy
- increment fix count
- if not in pool we see if there is place in pool and if yes we read page and add page 
- else we follow each page replacement strategy to delete one page
- and then we add new page

----

PageNumber *getFrameContents (BM_BufferPool *const bm)

- we iterate over pool and get page number of each frame
- we store it in array of int

bool *getDirtyFlags (BM_BufferPool *const bm)

- - we iterate over pool and get dirty bit value for each frmae
- we store it in array of bool

int *getFixCounts (BM_BufferPool *const bm)

- we iterate over pool and get fix count of each frame
- we store it in array of int

getNumReadIO and getNumWriteIO gives number of IO reads and write respectively