# **Record MANAGER**

## Assignment 3 
## ADO CS525 Summer 2023 : Group 15


## Team  and respective contribution
```
1. Name : Jie Kang (A20528112)jkang34@hawk.iit.edu 
   Work : Table and manager 

2. Kanakam Sai Dimple (A20516770) dkanakamsai@hawk.iit.edu
   Work : handling records in a table, scans

3. Marwan Omar (A20522313) momar3@iit.edu
   Work : Dealing with schemas, Dealing with records and attribute values
```

## How to execute

Firstly enter the directory in the terminal where all the files are present and  run

1. Execute 'make -f Makefile1' command: This will execute some test cases, and clens automatically in the end

2. We confirm the code works when we get the 'OK: finished test' 

3. Execute 'make -f Makefile2' : We execute soem more testcases

4. We confirm the code works when we get the 'OK: finished test' 

## Record Manager Functionality 

The following gives the explanation for the functionality of the record manager that we have implemented. There was one change at line 21 of rm_serializer.c file that we made as it was leading to segmentation fault: core dump.

We have created a custom structure which stores all necesarry meta data of the record manager, table, schema, bufferpool, page handle in order to easily perform required tasks.

----

RC initRecordManager (void *mgmtData)

- we have initialized required values

RC shutdownRecordManager ()

- we shut down other services that are not necessary anymore like the bufferpool

RC createTable (char *name, Schema *schema)

- we first use createpagefile function in storage manager, to create a new pagefile that will store our table
- then we allocate space for metainfo in order to store all the meta data
- once we get all these values we store on the first page of the file
- we also store the schema data in our custom stucture value for further use

RC openTable (RM_TableData *rel, char *name)

- we get the required schema from our custom structure and add it to the rel passed
- we then update some attributes in our structure

RC closeTable (RM_TableData *rel) 

- we basically update all the metadata stored in the first page
-then we shut down the buffer pool as we may not need it anymore

RC deleteTable (char *name)

- since we need to delete the table and its data, we simple invoke destroy page function implemented in the storage manager to get remove the file itself

int getNumTuples (RM_TableData *rel)

-we return the total number of records, this data we keep track of in our custom record manager data stucture.

----

RC insertRecord (RM_TableData *rel, Record *record)

- we first navigate the the page and slot where new record can be inserted
- then we pin the page
- we copy record to the location, and mark page as dirty
- then we update RID of new record in file
- then we update all meta data info in custome structure like incrementing record counter
- we unpin page

RC deleteRecord (RM_TableData *rel, RID id) 

- we first pin the required page
- we navigate to the record slot, and replace with empty characters, and mark dirty
- we decrement record count in our custom structure
- we unpin page

RC updateRecord (RM_TableData *rel, Record *record)

- similar to previous functions, we navigate to required place and replace record with new record vals passed

RC getRecord (RM_TableData *rel, RID id, Record *record)

-again similar to previous functions, we navigate to required record and read the vals 

----

startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)

- we initialize the custom structure created to manage scan data

RC next (RM_ScanHandle *scan, Record *record)

- if there are records that can be scanned
- we iterate through records, 
- we pin each page as necessary and unpin when not required anymore while iterating
- we compare with the expression passed to function and if it passed condition we scan it and update necessary vals

RC closeScan (RM_ScanHandle *scan)

- we just reset all vals of custom scan manager
- we then free space allocated to it

----

int getRecordSize (Schema *schema)

- we iterate through attributes 
- according to type of each we keep adding the sizes
- we return this size

Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)

- we basically just assign all the passed values to each corresponding value of a new schema structure

RC freeSchema (Schema *schema)

- we free the memory allocated to the schema structure

----

RC createRecord (Record **record, Schema *schema) 

- we allocate space for one record
- set space for its data

RC freeRecord (Record *record)

- we free the space allocated for a record

RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)

- we naviage to start position of attribute in the record
- we copy data from that location, size of the attribute
- we then use the inbuilt function stringToValue to get the actual value of the attribute

RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)

- similar to above we move to start pos of attribute
- we convert the data to be copied into string or char * type 
- then we set the value of the attribute

