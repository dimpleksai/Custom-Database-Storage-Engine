# **Storage Manager**

## Assignment 1 
## ADO CS525 Summer 2023 : Group 15


## Team  and respective contribution
```
1. Name : Jie Kang (A20528112)jkang34@hawk.iit.edu 
   Work : File manipulation interface

2. Kanakam Sai Dimple (A20516770) dkanakamsai@hawk.iit.edu
   Work : Read block interface

3. Marwan Omar (A20522313) momar3@iit.edu
   Work : Write block interface
```

## How to execute

Firstly enter the directory in the terminal where all the files are present and  run

1. Execute 'make clean' command: This will clear out all the output files

2. Execute the 'make' command : This will execute everything in the make file in order to build the code. 

3. Execute './test_assign1' : The above command creates a binary file for test_assign1 which can be executed this way to run all the tests

4. We confirm the code works when we get the 'OK: finished test' upon executing './test_assign1' 

## Storage Manager Functionality 

The following gives the explanation for the functionality of the storage manager that we have implemented. The storage manager consists of three parts which mainly aid in manipulating the files, reading blocks of memory from disk, and writing blocks to a page file.

## 1. File manipulation

**void initStorageManager(void)** 
- The initStorageMAnager function initiates the golbal file pointer by setting it to NULL.

**RC createPageFile(char \*fileName)**
- The creatPageFile function creates a file in disk by fileName, which is a char point to the beginning address of a const string. It uses mainly the function fopen() in C language, under the mode of 'w', if the file to be created exists, fopen() return file pointer, if not, creat a file by the fileName,then return file pointer.

**RC openPageFile(char \*fileName, SM_FileHandle \*fHandle)**

- The openPageFile function open a file already exists, by fopen() in 'r' mode, if the file doesn't exist, it won't creat it under this mode. Then calculate the parameter of the file and records them into fHandle'elements, include fileName, curPagePos, totalNumPages, and the file beginning pointer.There are some things that need to be clarified. The curPagePos is set 0 for the file is newdly opened. totalNumPages need to be calculated by the size of file and one page, it is the result that the quotient of these two numbers add one, because any dissatisfied page counts as one page.

**RC closePageFile(SM_FileHandle \*fHandle)**
- closePageFile will check the fHandle'mgmtInfo, which if it is Null, show there is not a openning file, otherwise, it will call fclose() in C language, close the file.

**RC destroyPageFile(char \*fileName)**
- destroyPageFile remove the file by the fileName, then return the resulet in integer. if rerurn 0, the deletion is successful, otherwise, return other number, at this time, the destroyPageFile function return rc error.

## 2. Read blocks from disk

**RC readBlock(int pageNm, SM_FileHandle \*storageMgrFileHndl, SM_PageHandle storageMgrPgHndl)**

- The readBlock function reads the block from the disk given the page number at which the block is present. We start by making sure the storage manager file handle passed to the function has been initialized. We then check that the page number passed is within the bounds which is 0-totalNumPages. Once we confirm the page number is in bound we open the file read mode. Then we make use of fseek function to set the position of the file pointer to the reuired page. We then use fread to read the page into the storage manager page handle. We next make sure the number of characters read which is nothing but the return value of the fread function is equal to the pagesize. If they are euqal we know that the page was read successfully hence we print the message conveying the same. We update current page position in the storage manager file handle and close the file.

**int getBlockPos(SM_FileHandle \*storageMgrFileHndl)**

- The getBlockPos returns the position of the current block which is stored in the storage manager file handle in curPagePos variable. 

**RC readFirstBlock(SM_FileHandle \*storageMgrFileHndl, SM_PageHandle storageMgrPgHndl)**

- The readFirstBlock function reads the first block by invoking the readBlock function by passing page number to it as 0.

**RC readCurrentBlock (SM_FileHandle \*storageMgrFileHndl, SM_PageHandle storageMgrPgHndl)**

- The readCurrentBlock function reads the block at the current pointer position which is retrieved using the getBlockPos function and we send this value as pagenumber to the readBlock function

**RC readPreviousBlock (SM_FileHandle \*storageMgrFileHndl, SM_PageHandle storageMgrPgHndl)**

- The readPreviousBlock function reads block at location of page present exactly before the current page position. Just as readCurrentBlock function we get the current page position using getBlockPos function then we decrement the value by 1 and pass this value as page number to the readBlock function

**RC readNextBlock (SM_FileHandle \*storageMgrFileHndl, SM_PageHandle storageMgrPgHndl)**

- Similar to the readPreviousBlock function this function reads the page  present just after the current page position. We follow same steps as above except we increment the value of the current page position value returned by the getBlockPos position and then send it to readBlock function

**RC readLastBlock (SM_FileHandle \*storageMgrFileHndl, SM_PageHandle storageMgrPgHndl)**

- This function reads the last block. We can do so by getting the totalNumPages stored in the storage file handle. We check that if the value to it is 0 that means theres no page to be read. When thats not the case we just pass the value of total pages to readBlock function after decrementing it by 1 which is nothing but the page number of the last page.

## 3. Write blocks to page file

**RC writeBlock(int pageNm, SM_FileHandle \*storageMgrFileHndl, SM_PageHandle storageMgrPgHndl)**

- The writeBlock function is used to write a page block to the disk given the page number at which the block is present. We the open the file firstly. We make sure that the page number passed ot the function is within bound. Then we make use of the fseek function to get to the required page number. Then we use the fwrite function to write at the required position, we make sure all characters are written successfully by comapring the return value of the fwrite function with the page size. If they are equal it means the whole page was written successfully, and we display the same. We then update the current page position in the storage manager file handle and then close the file.

**RC writeCurrentBlock(SM_FileHandle \*storageMgrFileHndl, SM_PageHandle storageMgrPgHndl)**

- The writeCurrentBlock function is to write a page block to disk from current position. We retrieve the current block position using the getBlockPos function and pass this value as the page number to the writeBlock function above. 

**RC appendEmptyBlock(SM_FileHandle \*storageMgrFileHndl)**

- The appendEmptyBlock function does as the name suggests it appends an epmty block to the file. We use calloc function to allocate memory the size of pagesize. We use calloc instead of malloc as calloc initiates the allocated space to 0 implicitly. We now open the file in the append mode hence the file pointer is moved to the end of the file. We then use fwrite to write the created empty block of memory to the end of the file. We check that it was written successfully by making sure return value of the fwrite function is equal to pagesize. We then increment the total number of pages in the storage manager file handle. We close the file and also free the allocated memory as we have finished writing the block.

**RC ensureCapacity (int numPages, SM_FileHandle \*storageMgrFileHndl)**

- The ensureCapacity function is used to manage the total pages in the file handle. We retrieve the total number of pages in the storage manager file handle and compare it to the number of pages passed to the ensureCapacity function. If the total pages is less that value passed, we find the count of the pages to be appended. We then run a loop on this value and execute the appendEmptyBlock function. We now update the totalNumPages variable in the storage manager file handle to the new total.