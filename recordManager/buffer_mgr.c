#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "buffer_mgr_stat.h"
#include "dberror.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


typedef struct nodeDLL{
	int fixCount;
	char *bufferData;
	int isDirty;

	int pageNum;
	struct nodeDLL *prevNodePtr;
	int frameNum;
	struct nodeDLL *nextNodePtr;
	
} nodeDLL;
	
	
//Doubly Linked List
typedef struct DLL {
	nodeDLL *head; //first
	int totalNumOfFrames;
	nodeDLL *tail; //last
	int filledframes;
	
} DLL;

DLL *dll;
SM_FileHandle *fileHndl;
BM_PageHandle *pgHndl;

//initilalizing the buffer pool with doubly linked list data structure
void newDoublyLinkedList(BM_BufferPool *const bm);

int numReads;
int numWrites;

/* Functions to handle pool of buffer manager interface 
Jie Kang
*/
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
    pgHndl = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
	bm->numPages = numPages;

	char* buffersize = (char *)calloc(numPages,sizeof(char)*PAGE_SIZE);
	bm->mgmtData = buffersize;

	//File used to read write pages
	bm->pageFile = (char *)pageFileName;
	fileHndl = (SM_FileHandle *)malloc(sizeof(SM_FileHandle));
	openPageFile(bm->pageFile,fileHndl);

	//Page replacement strategy
	bm->strategy = strategy;

	//IO write reads counters
	numReads = 0;
	numWrites = 0;
	//Creating doubly linked list to store buffer pool
	dll = (DLL *)malloc(sizeof(DLL));
	newDoublyLinkedList(bm);

	return RC_OK;

}

//This function destroys buffer pool
RC shutdownBufferPool(BM_BufferPool *const bm)
{
	if (dll->head == NULL) {
        printf("Already Shut!\n");
		closePageFile(fileHndl);
        return RC_OK;
    }
	//if page pinned, fix count of any frame not 0, return error
	nodeDLL *iter = dll->head;
	while(iter){
		if(iter->fixCount!=0){
			printf("The pool still has pinned pages\n");
			return RC_POOL_SHUTDOWN_FAILED;
		}
		iter = iter->nextNodePtr;
	}
	//first flush pool
	forceFlushPool(bm);
	//iterate over pool and free each node
	while(iter){
		nodeDLL *freeNode = iter;
		iter = iter->nextNodePtr;
		dll->head = iter;
		free(freeNode);
	}
	closePageFile(fileHndl);
	return RC_OK;

}

//this function focefully writes all dirty pages in pool to disk
RC forceFlushPool(BM_BufferPool *const bm)
{
	//printf("here line 116\n");
	BM_PageHandle *pgHndl2 = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
	nodeDLL *iter = dll->head;
	//printPoolContent(bm);
	//iterate over the pool
	while(iter){
		//check if dirty
		if(iter->isDirty == 1){
			
			//page details in page handle 
			pgHndl2->data = iter->bufferData;
			pgHndl2->pageNum = iter->pageNum;
			//invoke forcepage to forecfully write page to disk
			forcePage(bm, pgHndl2);
		}
		iter = iter->nextNodePtr;
	}

	return RC_OK;
}



/* Functions to access pages of buffer manager interface 
Dimple Kanakam Sai(A20516770)
*/

//This function is to mark a page as dirty, page to be marked dirty is sent via the BM page handle
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    //We create a pointer to iterate frames in the pool, we make it point to first frame of pool
    nodeDLL *iter = dll->head;
    //iterate over the pool to find page, once found set it to dirty and return
    for(int i=0; i<dll->filledframes; i++){
        
        if(iter->pageNum == page->pageNum){
            //if page is not already dirty we set it dirty
            if(iter->isDirty==0)
                iter->isDirty++;
            return RC_OK;
        }
        if((iter->pageNum != page->pageNum) && (iter->nextNodePtr != NULL)){
            iter = iter->nextNodePtr;
        }
    }

    return RC_READ_NON_EXISTING_PAGE;

}

//This function unpins given page after client has finished read/ write to it
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{	
	//We create a pointer to iterate frames in the pool, we make it point to first frame of pool
    nodeDLL *iter = dll->head;
    for(int i=0; i<dll->filledframes; i++){
        
        if(iter->pageNum == page->pageNum){
            //decrement fix count by 1, to indicate one client has unpinned
            iter->fixCount--;
            return RC_OK;
        }
        if((iter->pageNum != page->pageNum) && (iter->nextNodePtr != NULL)){
            iter = iter->nextNodePtr;
        }
    }
    return RC_READ_NON_EXISTING_PAGE;
}

//This function forcefully writes given page to disk
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    //We create a pointer to iterate frames in the pool, we make it point to first frame of pool
    nodeDLL *iter = dll->head;
    //We iterate over the pool till page is found
    for(int i=0; i<dll->filledframes; i++){
        
        if(iter->pageNum == page->pageNum){
            //write page to disk using write function implemented in storage mngr
            int writeFunctStatus = writeBlock(page->pageNum, fileHndl,iter->bufferData);
            if(writeFunctStatus != 0)
                return RC_WRITE_FAILED;
            else{
                numWrites++; 
				iter->isDirty = 0;
                return RC_OK;
            }
        }
        if((iter->pageNum != page->pageNum) && (iter->nextNodePtr != NULL)){
            iter = iter->nextNodePtr;
        }
    }
    return RC_READ_NON_EXISTING_PAGE;
}

//This function pins the page required by the client, in case its not in the buffer pool
//then a page replacement strategy function is invoked 

// FIFO Replacement Strategy 
RC FIFO_Strategy(const PageNumber pageNum, BM_BufferPool *const bm, BM_PageHandle *const page); 


RC pinPage(BM_BufferPool * const bm, BM_PageHandle * const page,const PageNumber pageNum)
{
    //printf("pin page started");
    //We create a pointer to iterate frames in the pool, we make it point to first frame of pool
    nodeDLL *iter = dll->head;
    bool pageInPool = false;
    //We iterate over the pool till page is found
    for(int i=0; i<dll->filledframes; i++){
        if(iter->pageNum == pageNum){
            pageInPool = true;
            break;
        }
        else{
            iter = iter->nextNodePtr;
        }
    }
    
    int countPagesInPool = 0;
    //LRU stragtegy
    if(bm->strategy == RS_LRU){
        if(pageInPool){
            page->data = iter->bufferData;
             //page found so we increment page fix count
            iter->fixCount++;
            // set details of page handle
            page->pageNum = pageNum;

            //We update the order of nodes to match LRU   
                //check if its the last node
                if(iter->nextNodePtr==NULL){
                    
                    (iter->prevNodePtr)->nextNodePtr = NULL;
                }
                else             
                //node is in between
                if(iter->nextNodePtr!=NULL && iter!=dll->head){
                    printf("reached 501\n");
                    iter->prevNodePtr->nextNodePtr = iter->nextNodePtr;
                    iter->nextNodePtr->prevNodePtr = iter->prevNodePtr;
                }
                //make node as head
                iter->prevNodePtr = NULL;
                iter->nextNodePtr = dll->head;
                dll->head->prevNodePtr = iter;
                dll->head = iter;

                return RC_OK;

        }
        else{
				//page isnt in the pool 
				nodeDLL *temp, *iter2 = dll->head;
				int pageToBeDeletedFound = 0;
                
                //check if  pool is full
                if(dll->filledframes == dll->totalNumOfFrames){
                //delete the LRU node, find page with fix count 0, if dirty write it 
                //call LRU to delet node 
                //if no space in buffer pool return error
				//last node is LRU
				int totFrames = dll->totalNumOfFrames-1;
					while(totFrames){
						iter2=iter2->nextNodePtr;
						totFrames--;
					}
					
					for(int i=0; i < dll->filledframes; i++){
						if(iter2->fixCount!=0)
						{
							iter2=iter2->prevNodePtr;
						}
						else{
							//node to delete
							temp = iter2;
							//if its head node
							if(temp == dll->head){
								dll->head=temp->nextNodePtr;
								dll->head->prevNodePtr = NULL;
							}
							//if its last node
							else if(temp->nextNodePtr == NULL){
								temp->prevNodePtr->nextNodePtr = NULL;

							}
							//anywhere in between
							else{
								temp->nextNodePtr->prevNodePtr = temp->prevNodePtr;
								temp->prevNodePtr->nextNodePtr=temp->nextNodePtr;

							}
							pageToBeDeletedFound++;
							break;
						}
						
					}
					if(!pageToBeDeletedFound){
						return RC_NO_FREE_BUFFER_ERROR;
					}
					
					dll->filledframes--;
					//if dirty write back page and free memory of the page
					if(temp->isDirty){
						writeBlock(pageNum, fileHndl, temp->bufferData);
						//updating write IO count
						numWrites++;
					}
					
                }
				nodeDLL *newNode = (nodeDLL *)calloc(1, sizeof(nodeDLL));
				newNode->fixCount=1;
				newNode->pageNum = pageNum;
				page->pageNum = newNode->pageNum;
                //we read the new page into temporary new node, increment fix count
                readBlock(pageNum, fileHndl, newNode->bufferData);
				if(pageToBeDeletedFound)newNode->frameNum = temp->frameNum;
				else newNode->frameNum = dll->head->frameNum+1;
				page->data = newNode->bufferData;

				numReads++;
				//add the new node to head of dll.
                //if LinkedList empty
                if(dll->filledframes==0){
					newNode->nextNodePtr = dll->head;
					dll->head = newNode;
					newNode->nextNodePtr->prevNodePtr = dll->head;
					newNode->frameNum = newNode->nextNodePtr->frameNum;
				}else{printf("reached 512\n");
					newNode->nextNodePtr = dll->head;
					dll->head = newNode;
					newNode->nextNodePtr->prevNodePtr = newNode;
					
				}
				dll->filledframes++;
				if(pageToBeDeletedFound) free(temp);
                return RC_OK;
            }
    }
    //FIFO strategy
	if(bm->strategy == RS_FIFO){
        
        if(pageInPool){
            page->data = iter->bufferData;
             //page found so we increment page fix count
            iter->fixCount++;
            // set details of page handle
            page->pageNum = pageNum;
            return RC_OK;
        }
        else{
            //check if place in pool, if yes add new node to end of pool
            //iterate pool
            
            nodeDLL *tempNode ,*iter2 = dll->head;
            int returncode, itr =0;
            if (dll->filledframes < dll->totalNumOfFrames)
            {
				for(itr=0; itr<dll->filledframes; itr++)
                {
					//printf("itr = %d\n", itr);
                    iter2 = iter2->nextNodePtr;
                }
				tempNode = iter2;
                tempNode->fixCount = 1;
                tempNode->isDirty = 0;
                tempNode->pageNum = pageNum;
                page->pageNum= pageNum;     
                dll->filledframes = dll->filledframes + 1 ;
                readBlock(tempNode->pageNum,fileHndl,tempNode->bufferData);      
                page->data = tempNode->bufferData;
                numReads++;
                return RC_OK;
            }       
              
            return FIFO_Strategy(pageNum, bm, page);
            
        }
            
    }
}

void newDoublyLinkedList(BM_BufferPool *const bm)
{
	nodeDLL *firstNode; //first node
	
	firstNode = (nodeDLL*)calloc(1, sizeof(nodeDLL));
	firstNode->pageNum = -1;
	firstNode->bufferData = (char*) calloc(PAGE_SIZE, sizeof(char));
	dll->head = firstNode;
	
	dll->filledframes = 0;
	dll->totalNumOfFrames = bm->numPages;

	//rest of the list
	for(int loop=1; loop < bm->numPages; loop++){
		nodeDLL * newNode = (nodeDLL*)calloc(1, sizeof(nodeDLL));
		newNode->pageNum = -1;
		newNode->bufferData = (char*) calloc(PAGE_SIZE, sizeof(char));
		firstNode->nextNodePtr = newNode;
		newNode->prevNodePtr = firstNode;
		newNode->nextNodePtr = NULL;
		firstNode = firstNode->nextNodePtr;
		newNode->frameNum = loop;
	}
	dll->tail = firstNode->prevNodePtr;

}

// replace page in pool using this
RC FIFO_Strategy(const PageNumber pageNum,BM_BufferPool *const bm, BM_PageHandle *const page)
{
   
    nodeDLL *iter= dll->head;
    int foundReplacementPage=0, i=0;
	page->pageNum= pageNum;
    //new node 
    nodeDLL *nn = (nodeDLL *) calloc (1,sizeof(nodeDLL));
        nn->fixCount = 1;
        nn->pageNum = pageNum;
        
        iter = dll->head;
        for( i=0;i<bm->numPages; i++){
            if(iter->fixCount != 0) 
            	iter = iter->nextNodePtr;
            else
            {
                if(iter->isDirty == 1){
                    writeBlock(iter->pageNum,fileHndl,iter->bufferData);
                    numWrites++;
                    
                }
                foundReplacementPage++;
            	break;            	
            }
        }
    if(foundReplacementPage==0)
    {
        return RC_NO_FREE_BUFFER_ERROR;
    }
    //we found page to delete, now we remove pointer to it and add new node
    //if its at the end
    if(iter->nextNodePtr==NULL){
        iter->prevNodePtr->nextNodePtr = nn;
        nn->prevNodePtr = iter->prevNodePtr;
       // free(temp);
       // return RC_OK;

    }
    //at beginning
    else if(iter == dll->head){
        dll->head = iter->nextNodePtr;
        iter->nextNodePtr->prevNodePtr = NULL;
    }

    //anywhere in the middle
    else{
        iter->prevNodePtr->nextNodePtr = iter->nextNodePtr;
        iter->nextNodePtr->prevNodePtr = iter->prevNodePtr;
    }
        //read page to be pinned and store data in the last node
        nn->bufferData = iter->bufferData;   
        readBlock(pageNum,fileHndl,nn->bufferData);
        page->data = nn->bufferData;
        nn->frameNum = iter->frameNum; 
        numReads++;   
        //add node to end
    while(iter->nextNodePtr!=NULL){
        iter=iter->nextNodePtr;
    }

    //iter is at last node
    iter->nextNodePtr = nn;
    nn->prevNodePtr = iter;   
    return RC_OK;

}


//Statistics Interfaces
//Marwan Omar

//This function gives an array of page numbers stored at respective frames
PageNumber *getFrameContents (BM_BufferPool *const bm)
{
	printf("content funct\n");
	//iterate over pool, get page number and update respective index of array
	int *pageNumArr = (int*)calloc(dll->totalNumOfFrames, sizeof(int)), frm=0;
	nodeDLL *iter = dll->head;
	while(frm<dll->totalNumOfFrames)
		pageNumArr[frm++]=NO_PAGE;
	frm=0;
	while(iter){
		//printf("\nfrm no : %d ,  pg num  : %d",iter->frameNum, iter->pageNum);
		if(frm++ < dll->filledframes)
			pageNumArr[iter->frameNum] = iter->pageNum; 
		iter = iter->nextNodePtr;
	}

	return pageNumArr;
}
	
//This function gives an array of dirty value of respective frames
bool *getDirtyFlags (BM_BufferPool *const bm)
{
	//iterate over pool, check if frame dirty and update respective index of array
	//printf("inside getfix cnt, \n totfilled frm :%d , tot frames: %d\n",dll->filledframes,dll->totalNumOfFrames);
	bool *dirtyArr = (bool*)calloc(dll->totalNumOfFrames, sizeof(bool));
	nodeDLL *iter = dll->head;
	while(iter){
		dirtyArr[iter->frameNum] = iter->isDirty == 1 ? true : false;
		iter = iter->nextNodePtr;
	}

	return dirtyArr;
}



//This function gives an array of fix counts of respective frames
int *getFixCounts (BM_BufferPool *const bm)
{
	//printf("inside getfix cnt, \n totfilled frm :%d , tot frames: %d\n",dll->filledframes,dll->totalNumOfFrames);
	int *fixCountArr = (int*)calloc(dll->totalNumOfFrames, sizeof(int));
	nodeDLL *iter = dll->head;
	while(iter){
		//printf("frm no : %d ,  fx cnt : %d",iter->frameNum, iter->fixCount);
		fixCountArr[iter->frameNum] = iter->fixCount;
		iter = iter->nextNodePtr;
	}

	return fixCountArr;
}


//This function gets the total number of read operations performed
int getNumReadIO (BM_BufferPool *const bm)
{
	return numReads;
}

//This function gets the total number of write operations performed
int getNumWriteIO (BM_BufferPool *const bm)
{
	return numWrites;
}
