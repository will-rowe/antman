#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "bigsi.h"

/*****************************************************************************
 * bigsInit will initiate a BIGSI.
 * 
 * arguments:
 *      numBits                   - the number of bits per bloom filter
 *      numHashes                 - the number of hashes used per bloom filter
 * 
 * returns:
 *      pointer to an initiated BIGSI
 * 
 * note:
 *      the user must free the returned BIGSI
 *      the user must check the returned BIGSI for NULL (failed to init)
 *      the index is not ready until the bigsIndex function is called
 */
bigsi_t *bigsInit(int numBits, int numHashes)
{
    assert(numBits > 0);
    assert(numHashes > 0);

    bigsi_t *newIndex;
    if ((newIndex = malloc(sizeof *newIndex)) != NULL)
    {
        newIndex->bvArray = NULL;
        map_init(&newIndex->id2colour);
        newIndex->colourArray = NULL;
        newIndex->numBits = numBits;
        newIndex->numHashes = numHashes;
        newIndex->colourIterator = 0;
        newIndex->index = NULL;
    }
    return newIndex;
}

/*****************************************************************************
 * bigsAdd will assign colours to seqIDs and add corresponding bloom filters
 * to the BIGSI.
 * 
 * arguments:
 *      bigsi                     - the BIGSI data structure
 *      id2bf                     - a map of the bloom filters to insert (seqID:bloomfilter)
 *      numEntries                - the number of sequences to be inserted (the number of bloom filters in id2bf)
 * 
 * returns:
 *      0 on success, -1 on error
 * 
 * note:
 *      the index is not ready until the bigsIndex function is called
 */
int bigsAdd(bigsi_t *bigsi, map_bloomfilter_t id2bf, int numEntries)
{

    // if this is the first insert, init the colour array and bit vector array
    if (!bigsi->colourArray)
    {
        if ((bigsi->colourArray = malloc(numEntries * sizeof(char **))) == NULL)
        {
            fprintf(stderr, "could not allocate colour array\n");
            return -1;
        }
        if ((bigsi->bvArray = malloc(numEntries * sizeof(bitvector_t **))) == NULL)
        {
            fprintf(stderr, "could not allocate bit vector array\n");
            return -1;
        }
    }

    // otherwise resize the existing arrays
    else
    {
        char **tmp = realloc(bigsi->colourArray, (bigsi->colourIterator + numEntries) * sizeof(char **));
        if (tmp)
        {
            bigsi->colourArray = tmp;
        }
        else
        {
            fprintf(stderr, "could not re-allocate colour array\n");
            return -1;
        }
        bitvector_t **tmp2 = realloc(bigsi->bvArray, (bigsi->colourIterator + numEntries) * sizeof(bitvector_t **));
        if (tmp2)
        {
            bigsi->bvArray = tmp2;
        }
        else
        {
            fprintf(stderr, "could not re-allocate bit vector array\n");
            return -1;
        }
    }

    // iterate over the input bloomfilter map, use the colourIterator to assign colours
    int inputMapCheck = 0;
    const char *seqID;
    map_iter_t iter = map_iter(&id2bf);
    while ((seqID = map_next(&id2bf, &iter)))
    {

        // if the seqID is already in BIGSI, return error
        if (map_get(&bigsi->id2colour, seqID) != NULL)
        {
            fprintf(stderr, "duplicate sequence ID can't be added to BIGSI: %s\n", seqID);
            return -1;
        }

        // check the bloom filter is compatible and not empty
        bloomfilter_t *newBF = map_get(&id2bf, seqID);
        if ((newBF->numHashes != bigsi->numHashes) || (newBF->bitvector->capacity != bigsi->numBits))
        {
            fprintf(stderr, "bloom filter incompatible with BIGSI for sequence: %s\n", seqID);
            return -1;
        }
        if (bvCount(newBF->bitvector) == 0)
        {
            fprintf(stderr, "empty bloom filter supplied to BIGSI for sequence: %s\n", seqID);
            return -1;
        }

        // ADD 1 - get the bit vector and add it to the BIGSI tmp array
        bigsi->bvArray[bigsi->colourIterator] = bvClone(newBF->bitvector);

        // ADD 2 - store the sequence ID to colour lookup for this bit vector
        map_set(&bigsi->id2colour, seqID, bigsi->colourIterator);

        // ADD 3 - store the colour to sequence ID lookup as well
        if ((bigsi->colourArray[bigsi->colourIterator] = malloc(strlen(seqID) + 1)) == NULL)
        {
            fprintf(stderr, "could not allocate colour memory for sequence: %s\n", seqID);
            return -1;
        }
        strcpy(bigsi->colourArray[bigsi->colourIterator], seqID);

        // increment the colour iterator
        bigsi->colourIterator++;
        if (bigsi->colourIterator == MAX_COLOURS)
        {
            fprintf(stderr, "maximum number of colours reached\n");
            return -1;
        }

        // increment the map checker
        inputMapCheck++;
    }

    // check the number of input bloom filters matched the number expected by the user, otherwise we'll have memory issues
    if (inputMapCheck != numEntries)
    {
        fprintf(stderr, "number bloom filters read did not match expected number: %u vs %u\n", inputMapCheck, numEntries);
        return -1;
    }
    return 0;
}

/*****************************************************************************
 * bigsIndex will transform the BIGSI bit vectors in the index.
 * 
 * arguments:
 *      bigsi                     - the BIGSI data structure
 * 
 * returns:
 *      0 on success, -1 on error
 */
int bigsIndex(bigsi_t *bigsi)
{

    // check there are some bit vectors inserted into the data structure
    if (bigsi->colourIterator < 1)
    {
        fprintf(stderr, "no bit vectors have been inserted into the BIGSI, nothing to index\n");
        return -1;
    }

    // check it hasn't already been indexed
    if (bigsi->index != NULL)
    {
        fprintf(stderr, "indexing has already been run on this BIGSI\n");
        return -1;
    }

    // allocate the memory to store the array of BIGSI bit vectors
    if ((bigsi->index = malloc(bigsi->numBits * sizeof(bigsi->index))) == NULL)
    {
        fprintf(stderr, "could not assign memory during BIGSI indexing\n");
        return -1;
    }

    // iterate over all the BIGSI bit vectors by bit index (equates to rows in matrix)
    // TODO: this is super inefficient - do better
    for (int i = 0; i < bigsi->numBits; i++)
    {

        // create a new bit vector for this row
        bigsi->index[i] = bvInit(bigsi->colourIterator);
        if (bigsi->index[i] == NULL)
        {
            fprintf(stderr, "could not assign memory for new bit vector during BIGSI indexing\n");
            return -1;
        }

        // iterate over the bit vectors and check the bit index for the current row
        for (int colour = 0; colour < bigsi->colourIterator; colour++)
        {

            // check the bit vector
            if (bigsi->bvArray[colour] == NULL)
            {
                fprintf(stderr, "lost bit vector from BIGSI\n");
                return -1;
            }

            // check the bit at the required position (current BIGSI row) - skip if 0
            uint8_t bit = 0;
            if (bvGet(bigsi->bvArray[colour], i, &bit) != 0)
            {
                fprintf(stderr, "could not access bit at index position %u in bit vector for colour: %u\n", i, colour);
                return -1;
            }
            if (bit == 0)
            {
                continue;
            }
            // TODO: this check is unecessary as it shouldn't happen...
            bit = 0;
            if (bvGet(bigsi->index[i], colour, &bit) != 0)
            {
                fprintf(stderr, "could not access bit at index position %u in bigsi index bit vector %u\n", colour, i);
                return -1;
            }
            if (bit == 1)
            {
                fprintf(stderr, "trying to set same bit twice in BIGSI bitvector %u for colour %u\n", i, colour);
                return -1;
            }

            // update the BIGSI index with this colour
            if (bvSet(bigsi->index[i], colour, 1) != 0)
            {
                fprintf(stderr, "could not set bit\n");
                return -1;
            }
        }
    }

    // wipe the input bit vectors and the seqID map as they're not needed anymore (TODO: could keep if wanting to re-index)
    for (int i = 0; i < bigsi->colourIterator; i++)
    {
        bvDestroy(bigsi->bvArray[i]);
    }
    map_deinit(&bigsi->id2colour);
    return 0;
}

/*****************************************************************************
 * bigsQuery will determine if any sequences in the BIGSI contain a query
 * k-mer.
 * 
 * arguments:
 *      bigsi                     - the BIGSI data structure
 *      buffer                    - the query k-mer
 *      len                       - the length of the buffer
 *      result                    - an initialised bit vector to return the colours containing the query k-mer
 * 
 * returns:
 *      0 on success, -1 on error
 * 
 * note:
 *      it is the caller's responsibility to check and free the result
 */
int bigsQuery(bigsi_t *bigsi, const void *buffer, int len, bitvector_t *result)
{
    // check the BIGSI is ready for querying
    if (bigsi->index == NULL)
    {
        fprintf(stderr, "need to run the bigsIndex function first\n");
        return -1;
    }

    // check a k-mer has been provided
    if (!buffer)
    {
        fprintf(stderr, "no k-mer provided\n");
        return -1;
    }

    // check we can send the result
    if (!result)
    {
        fprintf(stderr, "no pointer provided for returning query results\n");
        return -1;
    }
    if (result->capacity != bigsi->colourIterator)
    {
        fprintf(stderr, "result bit vector capacity does not match number of colours in BIGSI\n");
        return -1;
    }
    if (result->count != 0)
    {
        fprintf(stderr, "result bit vector isn't empty\n");
        return -1;
    }

    // hash the query k-mer n times using the same hash func as for the BIGSI bloom filters
    register unsigned int hv;
    for (int i = 0; i < bigsi->numHashes; i++)
    {
        hv = getHashVal(buffer, len, i, bigsi->numBits);

        // get the corresponding bit vector in the bigsi index
        if (!bigsi->index[hv])
        {
            fprintf(stderr, "missing row in BIGSI for hash value: %d\n", hv);
            return -1;
        }

        // quick check to see if it's empty
        if (bvCount(bigsi->index[hv]) == 0)
        {
            return 0;
        }

        // update the result with this bit vector
        if (i == 0)
        {
            // first BIGSI hit can be the basis of the result, so just OR
            if (bvBOR(result, bigsi->index[hv], result) != 0)
            {
                fprintf(stderr, "could not bitwise OR during BIGSI query\n");
                return -1;
            }
        }
        else
        {

            // bitwise AND the current result with the new BIGSI hit
            if (bvBANDupdate(result, bigsi->index[hv]) != 0)
            {
                fprintf(stderr, "could not bitwise AND during BIGSI query\n");
                return -1;
            }

            // if the current result is now empty, we can leave early
            if (bvCount(result) == 0)
            {
                return 0;
            }
        }
    }
    return 0;
}

/*****************************************************************************
 * bigsDestroy will clear a BIGSI and release the memory.
 * 
 * arguments:
 *      bigsi                     - the BIGSI data structure
 * 
 * returns:
 *      0 on success, -1 on error
 */
int bigsDestroy(bigsi_t *bigsi)
{
    if (!bigsi)
    {
        fprintf(stderr, "no BIGSI was provided to bigsDestroy\n");
        return -1;
    }

    // if there has been a call to bigsAdd, there will be some freeing to do
    if (bigsi->colourIterator != 0)
    {
        // there will be a colourArray to free
        for (int i = 0; i < bigsi->colourIterator; i++)
        {
            free(bigsi->colourArray[i]);
        }
        free(bigsi->colourArray);

        // if there has been a call to bigsIndex, free the index
        if (bigsi->index)
        {
            for (int i = 0; i < bigsi->numBits; i++)
            {
                free(bigsi->index[i]);
            }
            free(bigsi->index);
        }

        // otherwise there will be input bloom filters and a lookup map that haven't been freed yet
        else
        {
            for (int i = 0; i < bigsi->colourIterator; i++)
            {
                bvDestroy(bigsi->bvArray[i]);
            }
            map_deinit(&bigsi->id2colour);
        }
    }

    bigsi->numBits = 0;
    bigsi->numHashes = 0;
    bigsi->colourIterator = 0;
    free(bigsi);
    return 0;
}

/*****************************************************************************
 * bigsDump will dump an indexed BIGSI to disk.
 * 
 * arguments:
 *      bigsi                     - the BIGSI data structure
 *      filepath                  - the filepath to write the BIGSI to
 * 
 * returns:
 *      0 on success, -1 on error
 */
int bigsDump(bigsi_t *bigsi, const char *filepath)
{
    // check the index is okay
    if (!bigsi)
    {
        fprintf(stderr, "no BIGSI was provided to bigsDump\n");
        return -1;
    }
    if (!bigsi->index)
    {
        fprintf(stderr, "must index BIGSI before running bigsDump\n");
        return -1;
    }

    // initialize the STOCK_DBS struct
    int ret;
    u_int32_t keyFlags;
    BIGSI_DB_t my_stock;
    initialize_stockdbs(&my_stock, filepath);

    // identify the files that will hold our databases
    set_db_filenames(&my_stock);

    // open all databases
    u_int32_t openFlags;
    openFlags = DB_CREATE | DB_EXCL;
    ret = databases_setup(&my_stock, PROG_NAME, stderr, openFlags);
    if (ret)
    {
        fprintf(stderr, "could not open up berkeley databases (%u)\n", ret);
        return -1;
    }

    // add the colours to the database
    DBT key, data;
    keyFlags = DB_APPEND;
    for (int i = 0; i < bigsi->colourIterator; i++)
    {
        // zero out the DBTs
        memset(&key, 0, sizeof(DBT));
        memset(&data, 0, sizeof(DBT));

        // set up the database record's key
        key.data = &i;
        key.size = sizeof(i);

        // set up the database record's data
        data.data = bigsi->colourArray[i];
        data.size = (u_int32_t)strlen(bigsi->colourArray[i]) + 1;

        // add it
        if (my_stock.colours_dbp->put(my_stock.colours_dbp, NULL, &key, &data, keyFlags))
        {
            fprintf(stderr, "could not add colour to database: %u -> %s\n", *(int *)key.data, (char *)data.data);
            return -1;
        }
    }

    // add the bit vectors to the database
    for (int i = 0; i < bigsi->numBits; i++)
    {
        // zero out the DBTs
        memset(&key, 0, sizeof(DBT));
        memset(&data, 0, sizeof(DBT));

        // set up the database record's key
        key.data = &i;
        key.size = sizeof(i);

        // set up the database record's data
        data.data = bigsi->index[i];
        data.size = (sizeof(bitvector_t) + (sizeof(uint8_t) * bigsi->index[i]->bufSize));

        // add it
        if (my_stock.bitvectors_dbp->put(my_stock.bitvectors_dbp, NULL, &key, &data, keyFlags))
        {
            fprintf(stderr, "could not add bit vector to database: %u\n", *(int *)key.data);
            return -1;
        }
    }

    // close our environment and databases
    databases_close(&my_stock);
    return 0;
}

/*****************************************************************************
 * bigsLoad will load an indexed BIGSI from disk.
 * 
 * arguments:
 *      filepath                  - the filepath to write the BIGSI to
 * 
 * returns:
 *      pointer to the loaded BIGSI
 * 
 * note:
 *      the user must free the returned BIGSI
 *      the user must check the returned BIGSI for NULL (failed to init)
 */
bigsi_t *bigsLoad(const char *filepath)
{
    // initialize the STOCK_DBS struct
    int ret;
    u_int32_t keyFlags;
    BIGSI_DB_t my_stock;
    initialize_stockdbs(&my_stock, filepath);

    // identify the files that will hold our databases
    set_db_filenames(&my_stock);

    // open all databases
    u_int32_t openFlags;
    openFlags = DB_RDONLY;
    ret = databases_setup(&my_stock, PROG_NAME, stderr, openFlags);
    if (ret)
    {
        fprintf(stderr, "could not open up berkeley databases (%u)\n", ret);
        return NULL;
    }

    DBT key, data;
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    keyFlags = 0;

    /* Set up our DBTs */
    int testSearch = 1;
    key.data = &testSearch;
    key.size = sizeof(testSearch);
    data.data = NULL;
    data.size = 0;

    if ((ret = my_stock.colours_dbp->get(my_stock.colours_dbp, NULL, &key, &data, 0)) == 0)
    {
        fprintf(stderr, "found it mofo\n");
        fprintf(stderr, "result = %s\n", (char *)data.data);
    }
    else
    {
        fprintf(stderr, "doh\n");
    }

    databases_close(&my_stock);

    // dont want to return this - need to decide how to use the db
    bigsi_t *loadedBIGSI = NULL;
    return loadedBIGSI;
}

//////////////////
/////////
//////////
////////

/* Initializes the STOCK_DBS struct.*/
void initialize_stockdbs(BIGSI_DB_t *my_stock, const char *filepath)
{
    my_stock->db_home_dir = filepath;
    my_stock->bitvectors_dbp = NULL;
    my_stock->colours_dbp = NULL;
    my_stock->bitvectors_db_name = NULL;
    my_stock->colours_db_name = NULL;
}

/* Identify all the files that will hold our databases. */
void set_db_filenames(BIGSI_DB_t *my_stock)
{
    size_t size;

    /* Create the Inventory DB file name */
    size = strlen(my_stock->db_home_dir) + strlen(BITVECTORS_DB_FILENAME) + 2;
    my_stock->bitvectors_db_name = malloc(size);
    snprintf(my_stock->bitvectors_db_name, size, "%s/%s", my_stock->db_home_dir, BITVECTORS_DB_FILENAME);

    /* Create the Vendor DB file name */
    size = strlen(my_stock->db_home_dir) + strlen(COLOURS_DB_FILENAME) + 2;
    my_stock->colours_db_name = malloc(size);
    snprintf(my_stock->colours_db_name, size, "%s/%s", my_stock->db_home_dir, COLOURS_DB_FILENAME);
}

/* open_database is a generic function to open a berkeley db database */
int open_database(DB **dbpp,                /* The DB handle that we are opening */
                  const char *filename,     /* The file in which the db lives */
                  const char *program_name, /* Name of the program calling this function */
                  FILE *error_file_pointer, /* File where we want error messages sent */
                  u_int32_t openFlags)      /* The flags to open the DB with */
{
    DB *dbp;
    int ret;

    // initialize the DB handle
    ret = db_create(&dbp, NULL, 0);
    if (ret != 0)
    {
        fprintf(error_file_pointer, "%s: %s\n", program_name, db_strerror(ret));
        return (ret);
    }

    // point to the memory malloc'd by db_create()
    *dbpp = dbp;

    // set up error handling for this database
    dbp->set_errfile(dbp, error_file_pointer);
    dbp->set_errpfx(dbp, program_name);

    // open the database
    ret = dbp->open(dbp,       /* Pointer to the database */
                    NULL,      /* Txn pointer */
                    filename,  /* File name */
                    NULL,      /* Logical db name (unneeded) */
                    DB_RECNO,  /* Database type (using queue) */
                    openFlags, /* Open flags */
                    0);        /* File mode. Using defaults */
    if (ret != 0)
    {
        dbp->err(dbp, ret, "failed to open '%s'", filename);
        return (ret);
    }
    return 0;
}

/* opens all bigsi databases */
int databases_setup(BIGSI_DB_t *my_stock, const char *program_name, FILE *error_file_pointer, u_int32_t openFlags)
{
    int ret;

    // open the colours database
    ret = open_database(&(my_stock->colours_dbp), my_stock->colours_db_name, program_name, error_file_pointer, openFlags);

    // error reporting is handled in open_database() so just return the return code here
    if (ret != 0)
    {
        return (ret);
    }

    // open the bit vectors database
    ret = open_database(&(my_stock->bitvectors_dbp), my_stock->bitvectors_db_name, program_name, error_file_pointer, openFlags);

    // error reporting is handled in open_database() so just return the return code here
    if (ret != 0)
    {
        return (ret);
    }
    return 0;
}

/* closes all the bigsi databases */
int databases_close(BIGSI_DB_t *my_stock)
{
    int ret;

    // Note: closing a database automatically flushes its cached data to disk, so no sync is required here
    if (my_stock->colours_dbp != NULL)
    {
        ret = my_stock->colours_dbp->close(my_stock->colours_dbp, 0);
        if (ret != 0)
        {
            fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
        }
    }
    if (my_stock->bitvectors_dbp != NULL)
    {
        ret = my_stock->bitvectors_dbp->close(my_stock->bitvectors_dbp, 0);
        if (ret != 0)
        {
            fprintf(stderr, "Vendor database close failed: %s\n", db_strerror(ret));
        }
    }
    return 0;
}