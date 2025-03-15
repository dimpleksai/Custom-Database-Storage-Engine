#include <stdlib.h>
#include <string.h>

#include "record_mgr.h"
#include "tables.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

//one small change has been made to rm_serializer.c, line21

MetaInfoTableData metaMngr;


/* Table and manager 
Jie Kang
*/
SM_FileHandle filehandler;
RC initRecordManager (void *mgmtData)
{    printf("Initializing\n");
    initStorageManager(); 
    return RC_OK;
}

//shut the record manager and freeing all allocated memory
RC shutdownRecordManager (){
    //execute shutdown of buffer pool
    shutdownBufferPool(&metaMngr.bufferPool);
    printf("Record manager shut down!\n");
    return RC_OK;
}

//function creates a new table given its name
RC createTable (char *name, Schema *schema){

    //we start by already implemented function in storage manager 
	createPageFile(name);
    //schema data meta data
    char * metaInfo = (char *)calloc(1, PAGE_SIZE); 
    //we can open the file 
    openPageFile(name, &filehandler);
    //we start by storing name of file
    strcpy(metaInfo, name);
    strcat(metaInfo, "|");
    metaMngr.free_space_lctn.page =1;
    //we add the schema values
    //starting with total count of attributes
    int totAttri = schema->numAttr;
    sprintf(metaInfo + strlen(metaInfo), "%d", totAttri);
    metaMngr.free_space_lctn.slot =0;
    strcat(metaInfo, "[");
    for(int i=0; i< totAttri;i++){
        strcat(metaInfo, "(");
        char * attrnm = schema->attrNames[i];
        strcat(metaInfo, attrnm);
        strcat(metaInfo, ":");
        sprintf(metaInfo + strlen(metaInfo), "%d", schema->dataTypes[i]);
        strcat(metaInfo, "~");
        sprintf(metaInfo + strlen(metaInfo), "%d", schema->typeLength[i]);
        strcat(metaInfo, ")");
    }
    strcat(metaInfo, "]");
    //we add other schema data
    int sizeKey = schema->keySize;
    metaMngr.totalRecords =0;
    sprintf(metaInfo+ strlen(metaInfo),"%d",sizeKey);
    strcat(metaInfo, "{");
    for(int i=0; i< sizeKey;i++)
        sprintf(metaInfo + strlen(metaInfo), "%d:", schema->keyAttrs[i]);
    sprintf(metaInfo + strlen(metaInfo) - 1, "%s", "}");
    strcat(metaInfo, "$1:0$?0?");
	metaMngr.rm_tbl_data = (RM_TableData *)calloc(1, sizeof(RM_TableData));
    metaMngr.rm_tbl_data->schema = (Schema *)calloc(1, sizeof(Schema));
    memcpy(metaMngr.rm_tbl_data->schema, schema, sizeof(Schema));
    metaMngr.rm_tbl_data->name = name;
    
    //finally we can add this meta information to the start of the file on first page
    writeBlock(0, &filehandler, metaInfo);
    return RC_OK; 
  }


//function to open table
RC openTable (RM_TableData *rel, char *name)
{
    initBufferPool(&metaMngr.bufferPool, name, 3, RS_FIFO, NULL);
    //we first pin the first page which contains the metainfo
    pinPage(&metaMngr.bufferPool, &metaMngr.pageHandle, 0);
    //we retrieve the schema and add it to rel schema
    rel->schema = metaMngr.rm_tbl_data->schema;
    rel->name = metaMngr.rm_tbl_data->name;
    //we then update the size of record and records per page info in our custom table data manager
    metaMngr.recordsize =  getRecordSize(metaMngr.rm_tbl_data->schema) + 1;  
    metaMngr.numRecPerPage = (PAGE_SIZE / metaMngr.recordsize);
    unpinPage(&metaMngr.bufferPool,&metaMngr.pageHandle); 
}

//function is to close table and update meta info in the file
RC closeTable (RM_TableData *rel) 
{
    //schema data meta data
    char * metaInfo = (char *)calloc(1, PAGE_SIZE); 
    //we can open the file 
    openPageFile(rel->name, &filehandler);
    //we start by storing name of file
    strcpy(metaInfo, rel->name);
    strcat(metaInfo, "|");
    //we add the schema values
    //starting with total count of attributes
    int totAttri = rel->schema->numAttr;
    sprintf(metaInfo + strlen(metaInfo), "%d", totAttri);
    strcat(metaInfo, "[");
    for(int i=0; i< totAttri;i++){
        strcat(metaInfo, "(");
        char * attrnm = rel->schema->attrNames[i];
        strcat(metaInfo, attrnm);
        strcat(metaInfo, ":");
        sprintf(metaInfo + strlen(metaInfo), "%d", rel->schema->dataTypes[i]);
        strcat(metaInfo, "~");
        sprintf(metaInfo + strlen(metaInfo), "%d", rel->schema->typeLength[i]);
        strcat(metaInfo, ")");
    }
    strcat(metaInfo, "]");
    //we add other schema data
    int sizeKey = rel->schema->keySize;
    sprintf(metaInfo+ strlen(metaInfo),"%d",sizeKey);
    strcat(metaInfo, "{");
    for(int i=0; i< sizeKey;i++)
        sprintf(metaInfo + strlen(metaInfo), "%d:", rel->schema->keyAttrs[i]);
    sprintf(metaInfo + strlen(metaInfo) - 1, "%s", "}$");
    int pg =metaMngr.free_space_lctn.page;
    sprintf(metaInfo+ strlen(metaInfo),"%d:",pg);
    int slot = metaMngr.free_space_lctn.slot;
    sprintf(metaInfo+ strlen(metaInfo),"%d",slot);
    strcat(metaInfo, "$?");
    sprintf(metaInfo+ strlen(metaInfo),"%d",metaMngr.totalRecords);
    strcat(metaInfo, "?");

    //we update the new meta info to the page file
    writeBlock(0, &filehandler, metaInfo);
    //we can now shut the buffer pool
    shutdownBufferPool(&metaMngr.bufferPool);

    return RC_OK;
}

//This function deletes the table also its data
RC deleteTable (char *name)
{
    destroyPageFile(name);
    return RC_OK;
}

//returns the total numbers of records in table
int getNumTuples (RM_TableData *rel)
{
    int returnCode = metaMngr.totalRecords;
    return returnCode;
}



/*handling records in a table
Dimple Kanakam Sai(A20516770)
*/

//This function is to insert a new record passed to it at the next empty space in table
RC insertRecord (RM_TableData *rel, Record *record)
{
    //find page with empty space for inserting record
    int emptyPageNum = metaMngr.free_space_lctn.page;
    int emptySlotNum = metaMngr.free_space_lctn.slot;
    //pin the page
    pinPage(&metaMngr.bufferPool, &metaMngr.pageHandle, emptyPageNum);
    //append $ to end of record
    strcat(record->data, "$");
    strcpy((&metaMngr.pageHandle)->data + (emptySlotNum * metaMngr.recordsize), record->data);
    //mark dirty the page
    markDirty(&metaMngr.bufferPool, &metaMngr.pageHandle);
    //update record ID
    record->id.page = emptyPageNum;
    record->id.slot = emptySlotNum;
    //update table information
    metaMngr.totalRecords++;
    int updated_empty =0;
    int lastPageIndex = metaMngr.numRecPerPage-1;
    while(updated_empty++ == 0){
        //check if theres space at end of page
        if(lastPageIndex != emptySlotNum && emptySlotNum >= 0)
            metaMngr.free_space_lctn.slot++;
        else {
            //reset free slot to first slot of next page
            metaMngr.free_space_lctn.slot=0;
            metaMngr.free_space_lctn.page++;
        }
    }
    //finally unpin page
    unpinPage(&metaMngr.bufferPool, &metaMngr.pageHandle);

    return RC_OK;
}

//To delete record given its RID
RC deleteRecord (RM_TableData *rel, RID id) 
{
    //pin the page passed in RID
    pinPage(&metaMngr.bufferPool, &metaMngr.pageHandle, id.page);
    //reset values at slot to empty 
    memset((&metaMngr.pageHandle)->data + (id.slot * metaMngr.recordsize), '\0', metaMngr.recordsize);
    //mark the page dirty
    markDirty(&metaMngr.bufferPool, &metaMngr.pageHandle);
    //reduce totrecord count
    metaMngr.totalRecords-=1;  
    //finally unpin page
    unpinPage(&metaMngr.bufferPool, &metaMngr.pageHandle);
    return RC_OK;
}

//update record given its data and RID
RC updateRecord (RM_TableData *rel, Record *record)
{
    //pin the page passed in RID
    pinPage(&metaMngr.bufferPool, &metaMngr.pageHandle, record->id.page);
    //update record at location
    strcpy((&metaMngr.pageHandle)->data + (record->id.slot * metaMngr.recordsize), record->data);
    //mark the page dirty
    markDirty(&metaMngr.bufferPool, &metaMngr.pageHandle);  
    //finally unpin page
    unpinPage(&metaMngr.bufferPool, &metaMngr.pageHandle);
    return RC_OK;
}

//we get the required record data given its RID
RC getRecord (RM_TableData *rel, RID id, Record *record)
{
    //pin the page passed in RID
    pinPage(&metaMngr.bufferPool, &metaMngr.pageHandle, id.page);
    //copy memory into record from location
    memcpy(record->data, (&metaMngr.pageHandle)->data + (id.slot * metaMngr.recordsize), metaMngr.recordsize);
    //finally unpin page
    unpinPage(&metaMngr.bufferPool, &metaMngr.pageHandle);
    //update RID values of record
    record->id = id;
    return RC_OK;
}



/* scans
Dimple Kanakam Sai(A20516770)
*/

//initializes the scan handle to start scan
RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
    //allocate memory for custom struct to manage scanned data
    ManageScan * tempScanner;
    tempScanner = (ManageScan *)calloc(1,sizeof(ManageScan));
    (tempScanner->recID).page=1;
    //set expression
    tempScanner->cond =cond;
    scan->rel = rel;
	scan->mgmtData = tempScanner;
    return RC_OK;
					
}

//returns successive tuple which fulfills scan expression condition
RC next (RM_ScanHandle *scan, Record *record)
{
    //scanmanager temporary
    ManageScan * scanMngr = scan->mgmtData;
    if(scanMngr->count == metaMngr.totalRecords)
        return RC_RM_NO_MORE_TUPLES;
    //check there are records left to scan
	if(scanMngr->count < metaMngr.totalRecords){
    //pin the page we need
    pinPage(&metaMngr.bufferPool, &metaMngr.pageHandle, (scanMngr->recID).page);
    //iterate over all tuples starting from next RID value
    for(int slot = (scanMngr->recID).slot, pg = (scanMngr->recID).page; scanMngr->count <= metaMngr.totalRecords; scanMngr->count++){	
        //get record for RID
        //printf("line 492 pg : %d , slot : %d , scancount %d \n",pg ,slot, scanMngr->count);
        getRecord(scan->rel, scanMngr->recID, record);
        Value *queryExpResult = (Value *)calloc(1, sizeof(Value));
        //if condition is NULL set true as all tuples to be scanned
        if(scanMngr->cond == NULL){
            (queryExpResult->v).boolV = TRUE;
        }
        else
		    //invoke evalExpr to check if record satisfies condition
		    evalExpr(record, (scan->rel)->schema, scanMngr->cond, &queryExpResult);
		if((queryExpResult->v).boolV){
            scanMngr->count++;
            int updated_RID =0;
            int lastPageIndex = metaMngr.numRecPerPage-1;
            while(updated_RID++ == 0){
                //check if theres space at end of page
                if(lastPageIndex != slot)
                    slot++;
                else {
                    //unpin the old page
                    unpinPage(&metaMngr.bufferPool, &metaMngr.pageHandle);
                    slot=0;
                    pg++;
                    //and pin new page
                    pinPage(&metaMngr.bufferPool, &metaMngr.pageHandle, pg);
                }
                scanMngr->recID.slot = slot;
                scanMngr->recID.page = pg;
            }
            //unpin the page
            unpinPage(&metaMngr.bufferPool, &metaMngr.pageHandle);
            return RC_OK;
		}
							
		int updated_RID =0;
        int lastPageIndex = metaMngr.numRecPerPage-1;
        while(updated_RID++ == 0){
            //check if theres space at end of page
            if(lastPageIndex != slot)
                slot++;
            else {
                //unpin the old page
                unpinPage(&metaMngr.bufferPool, &metaMngr.pageHandle);
                //reset free slot to first slot of next page
                slot=0;
                pg++;
                //and pin new page
                pinPage(&metaMngr.bufferPool, &metaMngr.pageHandle, pg);
            }
            scanMngr->recID.slot = slot;
            scanMngr->recID.page = pg;
        }
        
	}
    }
    //unpin the page
    unpinPage(&metaMngr.bufferPool, &metaMngr.pageHandle);
    return  RC_RM_NO_MORE_TUPLES; 
}

//this is to close scan handle
RC closeScan (RM_ScanHandle *scan)
{
    //reset all values in the scan meta data
    int reset = 0;
    ((ManageScan *)scan->mgmtData)->count = reset;
    ((ManageScan *)scan->mgmtData)->recID.slot = reset;
    reset++;
    ((ManageScan *)scan->mgmtData)->recID.page=reset;
    //free the memory allocated for managing scanning
    if (scan->mgmtData) {
        free(scan->mgmtData);
        scan->mgmtData = NULL; 
        }
    return RC_OK;
}




/*Dealing with schemas
Marwan Omar*/

int getRecordSize (Schema *schema)
{
    int recordSize =0;
    //iterate over all attributes
    for(int i=0; i < schema->numAttr; i++){
        //if its not string type
        if(schema->dataTypes[i]!= 1){
            int dataType = schema->dataTypes[i];
            //add size of each attributes' datatype
            recordSize+= dataType == 0 ? sizeof(int) : dataType == 2 ? sizeof(float) : sizeof(bool);
        }
        //string type
        else if(schema->dataTypes[i]== 1){
            //get size stored in typelength using index
            recordSize+= schema->typeLength[i];
        }
    }
    return recordSize; 
}

//Given the values of schema, we create new schema type and assign them
Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys){

    //allocate memory for schema type
    Schema *newSchema = (Schema *)calloc(1, sizeof(Schema));
    //assign each value to respective attribute irrespective of order
    newSchema->numAttr = numAttr;
    newSchema->attrNames = attrNames;
    newSchema->dataTypes = dataTypes;
    newSchema->typeLength = typeLength;
    newSchema->keyAttrs = keys;
    newSchema->keySize = keySize;
    
    //update the table meta data information as well
    metaMngr.recordsize = getRecordSize(newSchema);
    //since its a new schema set total records count to 0
    metaMngr.totalRecords = 0;

    return newSchema;
}

// function to free memory allocated to schema
RC freeSchema (Schema *schema){
    //if its already not free, then we free memory allocated to it
    if(schema) free(schema);
    return RC_OK;
}


/*Dealing with records and attribute values
Marwan Omar*/

//Function to create a new record
RC createRecord (Record **record, Schema *schema) 
{
    //allocate space for record
    *record = (Record *)calloc(1, sizeof(Record));
    //allocate space for data 
    (*record)->data = (char *)calloc(getRecordSize(schema), sizeof(char));
    //we can set memory to the data with zeros, its optional
    //memset((*record)->data, '\0', getRecordSize(schema));
    return RC_OK;
}

//Function is to free space allocated to a record
RC freeRecord (Record *record)
{
    //if record not already null, we free it
    if(record) free(record);
    return RC_OK;
}

//function to get required attribute
RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
    //iterate through record to find start of attribute position
    int attrBeg =0;
    for(int i=0; i < attrNum; i++){
        //if its not string type
        if(schema->dataTypes[i]!= 1){
            int dataType = schema->dataTypes[i];
            //add size of each attributes' datatype
            attrBeg+= dataType == 0 ? sizeof(int) : dataType == 2 ? sizeof(float) : sizeof(bool);
        }
        //string type
        else if(schema->dataTypes[i]== 1){
            //get size stored in typelength using index
            attrBeg+= schema->typeLength[i];
        }
    }

    //We find size of attribute to determine how many characters of the record should be copied
    int dataTypeAttr = schema->dataTypes[attrNum];
    int attrSize = dataTypeAttr == 1 ? schema->typeLength[attrNum] : dataTypeAttr == 0 ? sizeof(int) : dataTypeAttr == 2 ? sizeof(float) : sizeof(bool);

    //get pointer to start of record data
    char * itr = record->data;
    //move this to beginning of attribute
    itr += attrBeg;
    //copy data from this position to temporary string
    char * attrVal = (char *)calloc(1, attrSize+1);
    //in order to use inbuilt function stringToValue, we set first char indicating the datatype
    attrVal[0] = dataTypeAttr == 1 ? 's' : dataTypeAttr == 0 ? 'i' : dataTypeAttr == 2 ? 'f' : 'b';
    strncpy(attrVal+1, itr, attrSize);
    //we make use of inbuilt function to convert this stringtovalue
    Value *field = (Value*) malloc (sizeof (Value));
    field =  stringToValue(attrVal);
    *value = field;        
	
	return RC_OK;
	
}

//function to set required attribute
RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
     //iterate through record to find start of attribute position
    int attrBeg =0;
    for(int i=0; i < attrNum; i++){
        //if its not string type
        if(schema->dataTypes[i]!= 1){
            int dataType = schema->dataTypes[i];
            //add size of each attributes' datatype
            attrBeg+= dataType == 0 ? sizeof(int) : dataType == 2 ? sizeof(float) : sizeof(bool);
        }
        //string type
        else if(schema->dataTypes[i]== 1){
            //get size stored in typelength using index
            attrBeg+= schema->typeLength[i];
        }
    }
    //We find size of attribute to determine how many characters of the record should be copied
    int dataTypeAttr = schema->dataTypes[attrNum];
    int attrSize = dataTypeAttr == 1 ? schema->typeLength[attrNum] : dataTypeAttr == 0 ? sizeof(int) : dataTypeAttr == 2 ? sizeof(float) : sizeof(bool);

    //get pointer to start of record data
    char * itr = record->data;
    //move this to beginning of attribute
    itr += attrBeg;
    //copy value to the record
    //if string
    if(dataTypeAttr == 1)
        strcpy(itr, value->v.stringV);
    //float
    else if(dataTypeAttr == 2)
        sprintf(itr, "%f", value->v.floatV);
    //bool
    else if(dataTypeAttr == 3)
        sprintf(itr, "%s", value->v.boolV ? "true" : "false");
    //if int
    else if(dataTypeAttr == 0){
        int i=0, tempNum = value->v.intV;
        char str[attrSize+1];i=3;
        str[i] = '\0';
        tempNum = value->v.intV;
        while (i >= 0) {
        str[i--] = '0' + (tempNum % 10);
        tempNum /= 10;
    }
    sprintf(itr, "%s", str);

    }
    return RC_OK;
}


