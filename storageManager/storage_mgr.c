#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include "storage_mgr.h"
#include<stdbool.h>
#include "dberror.h"

//can be accessed by all functions
FILE * globalFilePtr;
RC return_Value;
/* Functions to mainpulate files in page
Jie Kang(A20528112)
*/

void initStorageManager(void) {
  printf("Storage Manager Initialized!\n");
  globalFilePtr = NULL;
  return;
}

RC createPageFile(char *fileName) {
  
  //open the file in write mode
  globalFilePtr = fopen(fileName, "w+");
  //if the file is not opened successfully, return rc error, which is definded in dberror.h
  if (globalFilePtr == NULL) {
    return RC_FILE_HANDLE_NOT_INIT;
  //if the file is opened successfully, return success rc
  } else {
    return RC_OK;
  }
}


RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
  // size is the size of the file to be opened, it calculated by bytes
  int size;
  //open the file in read mode
  globalFilePtr = fopen(fileName, "r+");
  //if the file is not opened successfully
  if (globalFilePtr == NULL) {
    return RC_FILE_NOT_FOUND;
  //if the file is opened successfully, calculate file handle
  } else {
    //set filename
    fHandle->fileName = fileName;
    //set file Pagepos,new file's pagepos is 0
    fHandle->curPagePos = 0;
    //calculate total Number of Pages, first move the file pointer to the end of the file
    fseek(globalFilePtr, 0, SEEK_END);
    //get the size of the file
    size = ftell(globalFilePtr);
    //calculate total Number of Pages, divide the size of the file by the size of each page
    fHandle->totalNumPages = (size+1) / PAGE_SIZE + 1;
    //move the file pointer to the beginning of the file
    fseek(globalFilePtr, 0, SEEK_SET);
    //set the file beginning pointer to the file handle
    fHandle->mgmtInfo = globalFilePtr;
    //return success rc
    return RC_OK;
  }
}

RC closePageFile(SM_FileHandle *fHandle) {
  //if there is not file handle, return rc error
  if (fHandle->mgmtInfo == NULL) {
    return RC_FILE_HANDLE_NOT_INIT;
  //if there is file handle, a openning file exists, then close the file, and set the file handle to NULL
  } else {
    fclose(fHandle->mgmtInfo);
    fHandle->mgmtInfo = NULL;
    //return success rc
    return RC_OK;
  }
}

RC destroyPageFile(char *fileName) {
  //delete the file by the file's name, result record the result of remove function
  int result = remove(fileName);
//if the file is deleted successfully, return success rc
  if (result == 0) {
    return RC_OK;
//if the file is not deleted successfully, return rc error
  } else {
    return RC_FILE_NOT_FOUND;
  }
}



/* Functions to read bocks of memory from disk 
Dimple Kanakam Sai(A20516770)
*/

// function to read block given the pagenumber
RC readBlock(int pageNm, SM_FileHandle *storageMgrFileHndl, SM_PageHandle storageMgrPgHndl){
    printf("\n ---------- Function to read block invoked!------------\n");
    int returnOfSeek;
    int bytesReadSuccessfully;
    int returnCode;
    
    if(storageMgrFileHndl == NULL){
        printf("Storage manager file handle not initialized\n");
        return RC_READ_NON_EXISTING_PAGE;
    }
    else{
        // We first make sure given pagenumber is within bound, if its out of bound we throw error
        if(pageNm < 0 || pageNm > storageMgrFileHndl->totalNumPages){
            printf("Page number out of bound, please check pagenumber");
            returnCode = RC_READ_NON_EXISTING_PAGE;
        }
        else{
            //move pointer to required position i.e from beignning of file to (page number * size of page)
            //globalFilePtr = fopen(storageMgrFileHndl->fileName, "r");
            returnOfSeek = fseek(globalFilePtr, (pageNm * PAGE_SIZE), SEEK_SET);
            if(returnOfSeek != 0 ){
                printf("Error seeking position of page to be read\n");
                returnCode = RC_READ_NON_EXISTING_PAGE;
            }

            //read page at the pointer position 
            bytesReadSuccessfully = fread(storageMgrPgHndl, sizeof(char), PAGE_SIZE, globalFilePtr);
            //check if whole page content was read sucessfully or not

            if(bytesReadSuccessfully != PAGE_SIZE){
                printf("Error reading page, please recheck\n ");
                returnCode = RC_READ_NON_EXISTING_PAGE;
            }
            printf("---Page %d was read successfully!---\n", pageNm);
            storageMgrFileHndl->curPagePos = pageNm;
            returnCode = RC_OK;
            //fclose(globalFilePtr);

        }
        return returnCode;
    }
}


//function to return position of the current block
int getBlockPos(SM_FileHandle *storageMgrFileHndl){
    //current block position is the position of current page in storage manager file handle
    if(storageMgrFileHndl == NULL){
        printf("Storage mnager file handle not initialized\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    return storageMgrFileHndl->curPagePos;
}

//function to read the first block of disk or first page 
RC readFirstBlock(SM_FileHandle *storageMgrFileHndl, SM_PageHandle storageMgrPgHndl){
    //We can invoke already defined readBlock function above, passing page number as 0 to it
    RC status = readBlock(0, storageMgrFileHndl, storageMgrPgHndl);
    if(status == RC_OK)
        printf("-----Successfully read the first block-----\n");
    else 
        printf("***Error reading first block***\n");
    return status;
}

//function to read the current block/ page
RC readCurrentBlock (SM_FileHandle *storageMgrFileHndl, SM_PageHandle storageMgrPgHndl){
    //we can retrieve current block position from above function getBlockPos
    int currentBlockPos = getBlockPos(storageMgrFileHndl);
    // pass it to readBlock as page number
    RC status = readBlock(currentBlockPos, storageMgrFileHndl, storageMgrPgHndl);
    if(status == RC_OK )
        printf("-----Successfully read current block------\n");
    else
        printf("****Error reading current block****\n");
    return status;
}

//function to read previous block
RC readPreviousBlock (SM_FileHandle *storageMgrFileHndl, SM_PageHandle storageMgrPgHndl){
    //retrieve current block pos, subtract 1 to get previous block pos
    int currentBlockPos = getBlockPos(storageMgrFileHndl);
    int prevBlockPos = currentBlockPos--;
    // pass it to readBlock as page number
    RC status = readBlock(prevBlockPos, storageMgrFileHndl, storageMgrPgHndl);
    if(status == RC_OK )
        printf("-----Successfully read previous block------\n");
    else
        printf("****Error reading previous block****\n");
    return status;
}

//function to read subsequent block 
RC readNextBlock (SM_FileHandle *storageMgrFileHndl, SM_PageHandle storageMgrPgHndl){
    //get current block pos, add 1 to get next block pos
    int currentBlockPos = getBlockPos(storageMgrFileHndl);
    int nextBlockPos = currentBlockPos++;
    // pass it to readBlock as page number
    RC status = readBlock(nextBlockPos, storageMgrFileHndl, storageMgrPgHndl);
    if(status == RC_OK )
        printf("-----Successfully read subsequent block------\n");
    else
        printf("****Error reading subsequent block****\n");
    return status;
}

//function to read last block
RC readLastBlock (SM_FileHandle *storageMgrFileHndl, SM_PageHandle storageMgrPgHndl){
    //get total number of pages to find last page
    //if totalpages = 0 then no page to read 
    int totalPages = storageMgrFileHndl->totalNumPages;
    if(totalPages == 0){
        printf("No page to read\n");
        return RC_READ_NON_EXISTING_PAGE;
    }
    //to get position of last block, subtract 1 from total page count
    int lastBlockPos = totalPages--;
    // pass it to readBlock as page number
    RC status = readBlock(lastBlockPos, storageMgrFileHndl, storageMgrPgHndl);
    if(status == RC_OK )
        printf("-----Successfully read last block------\n");
    else
        printf("****Error reading last block****\n");
    return status;
}

/* Functions to write bocks to page file 
Marwan Omar (A20522313)
*/

//Function to write a page block to disk given the page number of the block
RC writeBlock(int pageNm, SM_FileHandle *storageMgrFileHndl, SM_PageHandle storageMgrPgHndl){
    printf("-----Function to write block invoked!-----");
    int returnOfSeek, bytesReturnedSuccessfully, returnCode;

    //globalFilePtr = fopen(storageMgrFileHndl->fileName, "w");
    //We need to ensure page number passed is within bounds
    if(pageNm>=0 && pageNm<=storageMgrFileHndl->totalNumPages){
        //Using fseek we move pointer to start of page number given
        returnOfSeek = fseek(globalFilePtr, pageNm * PAGE_SIZE, SEEK_SET);
        if(returnOfSeek != 0 ){
            printf("Error seeking position of page to be written\n");
            returnCode = RC_WRITE_FAILED;
        }
        else {
            //fwrite is used to write at required position
            bytesReturnedSuccessfully = fwrite(storageMgrPgHndl, sizeof(char), PAGE_SIZE, globalFilePtr);
            // make sure all characters are returned by comapring with page size 
            if(bytesReturnedSuccessfully != PAGE_SIZE){
                printf("Error writing content of page\n");
                returnCode = RC_WRITE_FAILED;
            }
            printf("---Page %d was written successfully!---\n", pageNm);
            //update current position of page
            storageMgrFileHndl->curPagePos = pageNm;
			//update totalNumpages;
			storageMgrFileHndl->totalNumPages++;
            returnCode = RC_OK;
            //fclose(globalFilePtr);
        }
    }
    else{
        printf("**********Error writing page, page number out of bound*********\n");
        returnCode = RC_WRITE_FAILED;
    }
    return returnCode;
}

//Function to write a page block to disk, from the current position 
RC writeCurrentBlock(SM_FileHandle *storageMgrFileHndl, SM_PageHandle storageMgrPgHndl){
    //Similar to read current block; we get current block position and pass to writeBlock function
    //we can retrieve current block position from function getBlockPos
    int currentBlockPos = getBlockPos(storageMgrFileHndl);
    // pass it to writeBlock as page number
    RC status = writeBlock(currentBlockPos, storageMgrFileHndl, storageMgrPgHndl);
    if(status == RC_OK )
        printf("-----Successfully written current block------\n");
    else
        printf("****Error writing current block****\n");
    return status;
}

//Function to append new page filled with zero bytes
RC appendEmptyBlock(SM_FileHandle *storageMgrFileHndl){
    //We use calloc to allocate memory for new page, as it initializes with 0
    SM_PageHandle pointerToNewBlock = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
    int returnCode;
    //we open the file in 'a' mode hence, file pointer is positioned at end of file
    globalFilePtr = fopen(storageMgrFileHndl->fileName, "a");

    if(globalFilePtr!=NULL){
        int bytesReturnedSuccessfully;
        //append the empty page to file
        bytesReturnedSuccessfully = fwrite(pointerToNewBlock, sizeof(char), PAGE_SIZE, globalFilePtr);
        //ensure all page successfully returned or not
        if(bytesReturnedSuccessfully != PAGE_SIZE){
            printf("Error appending empty block\n");
            returnCode = RC_WRITE_FAILED;
        }
        else{
            printf("-----Appended empty page sucessfully----\n");
            storageMgrFileHndl->totalNumPages++;
            returnCode = RC_OK;
        }
    }
    else{
        printf("Error opening file\n");
        returnCode = RC_FILE_NOT_FOUND;
    }
    fclose(globalFilePtr);
    free(pointerToNewBlock);
    return returnCode;

}

//Function to manage total pages and increase to given number of pages if its less
RC ensureCapacity (int numPages, SM_FileHandle *storageMgrFileHndl){
    printf("-----Function to ensure capacity invoked----------\n");
    int returnCode, count = 0;

    globalFilePtr = fopen(storageMgrFileHndl->fileName, "r");
    if(globalFilePtr != NULL){
        int totPagesInFile = storageMgrFileHndl->totalNumPages;
        if(totPagesInFile < numPages){
            //count of pages to append
            count = numPages - totPagesInFile;
            for(int i=0; i<count; i++){
                //we can invoke previous method to append empty blocks
                returnCode = appendEmptyBlock(storageMgrFileHndl);
            }
            //update total pages to numPages
            storageMgrFileHndl->totalNumPages = numPages;
        }
    }
    else{
        printf("Error opening file\n");
        returnCode = RC_FILE_NOT_FOUND;
    }
    fclose(globalFilePtr);
    return returnCode;
}