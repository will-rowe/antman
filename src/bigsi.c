#include <stdlib.h>
#include <stdio.h>

#include "bigsi.h"
#include "murmurhash2.h"

/*
    getReqBytes will return the number of bytes needed to hold the number of bits given
        numBits - number of bits to fit into byte(s)
*/
inline static int getReqBytes(int numBits)
{
    if (numBits % 8)
    {
        return (numBits / 8) + 1;
    }
    else
    {
        return numBits / 8;
    }
}

/*
    testBit will test if a bit is set in a bit vector: 1 indicates set, 0 indicates not set
        bv      - the bit vector
        pos     - the bit to test
        setBit  - if 1, set the bit (if it's not already)
*/
inline static int testBit(unsigned char *bv, unsigned int pos, int setBit)
{
    unsigned int byte = pos >> 3;
    unsigned char c = bv[byte];
    unsigned int mask = 1 << (pos % 8);
    if (c & mask)
    {
        return 1;
    }
    else
    {
        if (setBit)
        {
            bv[byte] = c | mask;
        }
        return 0;
    }
}

/*
    initBIGSI initialises the BIGSI data structure
        numBits     - the number of bits per bit vector
        numHashes   - the number of hashes used per bit vector
*/
bigsi_t *initBIGSI(int numBits, int numHashes)
{
    if (numBits < 1 || numHashes < 1)
    {
        return NULL;
    }
    bigsi_t *newIndex;
    if ((newIndex = malloc(sizeof *newIndex)) != NULL)
    {
        map_init(&newIndex->id2colour);
        newIndex->colourArray = NULL;
        map_init(&newIndex->bitvectors);
        newIndex->numBits = numBits;
        newIndex->numBytes = getReqBytes(numBits);
        newIndex->numHashes = numHashes;
        newIndex->colourIterator = 0;
        newIndex->index = NULL;
    }
    return newIndex;
}

/*
    insertBIGSI is used to assign colours to seqIDs, then insert corresponding bit vectors into the data structure
        bigsi       - the bigsi data structure
        id2bv       - a map of the bit vectors to insert (seqID:bitvector)
        numEntries  - the number of bit vectors to be inserted (the number of elements in the map)
*/
int insertBIGSI(bigsi_t *bigsi, map_uchar_t id2bv, int numEntries)
{

    // if this is the first insert, init the colour array
    if (!bigsi->colourArray)
    {
        if ((bigsi->colourArray = malloc(numEntries * sizeof(char *))) == NULL)
        {
            fprintf(stderr, "could not allocate colour array\n");
            return 1;
        }
    }

    // otherwise resize the existing array
    else
    {
        char **tmp = realloc(bigsi->colourArray, (bigsi->colourIterator + numEntries) * sizeof(bigsi->colourArray));
        if (tmp)
        {
            bigsi->colourArray = tmp;
        }
        else
        {
            fprintf(stderr, "could not re-allocate colour array\n");
            return 1;
        }
    }

    // iterate over the input bit vector map
    const char *seqID;
    map_iter_t iter = map_iter(&id2bv);
    while ((seqID = map_next(&id2bv, &iter)))
    {
        // if the seqID is already in BIGSI, return error
        if (map_get(&bigsi->bitvectors, seqID) != NULL)
        {
            fprintf(stderr, "duplicate sequence ID can't be added to BIGSI: %s\n", seqID);
            return 1;
        }

        // add the new bit vector to the BIGSI map
        unsigned char **newBV = map_get(&id2bv, seqID);
        map_set(&bigsi->bitvectors, seqID, *newBV);

        // assign a colour for the sequence ID
        map_set(&bigsi->id2colour, seqID, bigsi->colourIterator);

        // add the new colour -> sequence ID to the look up array
        bigsi->colourArray[bigsi->colourIterator] = malloc(*seqID * sizeof *bigsi->colourArray[bigsi->colourIterator]);
        if (!bigsi->colourArray[bigsi->colourIterator])
        {
            fprintf(stderr, "could not allocate memory for sequence ID: %s\n", seqID);
            return 1;
        }
        bigsi->colourArray[bigsi->colourIterator] = strdup(seqID);

        // increment the colour iterator
        bigsi->colourIterator++;
    }
    return 0;
}

/*
    indexBIGSI is used to transform the bit vectors into the BIGSI index
        bigsi       - the bigsi data structure
*/
int indexBIGSI(bigsi_t *bigsi)
{

    // check there are some bit vectors inserted into the data structure
    if (bigsi->colourIterator < 1)
    {
        fprintf(stderr, "no bit vectors have been inserted into the BIGSI, nothing to index\n");
        return 1;
    }

    // check it hasn't already been indexed
    if (bigsi->index != NULL)
    {
        fprintf(stderr, "indexing has already been run on this BIGSI\n");
        return 1;
    }

    // allocate the memory to store the array of BIGSI bit vectors
    if ((bigsi->index = malloc(bigsi->numBits * sizeof(char *))) == NULL)
    {
        fprintf(stderr, "could not assign memory during BIGSI indexing\n");
        return 1;
    }

    // calculate how many bytes are needed to hold the colours
    bigsi->numColourBytes = getReqBytes(bigsi->colourIterator);

    // iterate over the rows in the BIGSI
    for (int i = 0; i < bigsi->numBits; i++)
    {

        // create a new bit vector for the row that can hold all the colours
        bigsi->index[i] = calloc(bigsi->numColourBytes, sizeof(unsigned char *));
        if (bigsi->index[i] == NULL)
        {
            fprintf(stderr, "could not assign memory during BIGSI indexing\n");
            return 1;
        }

        // iterate over the bit vector map
        const char *seqID;
        map_iter_t iter = map_iter(&bigsi->bitvectors);
        while ((seqID = map_next(&bigsi->bitvectors, &iter)))
        {

            // get the bit vector
            unsigned char **bv = map_get(&bigsi->bitvectors, seqID);
            if (bv == NULL)
            {
                fprintf(stderr, "lost bit vector from BIGSI: %s\n", seqID);
                return 1;
            }

            // check the bit at the required position (current BIGSI row) - skip if 0
            if (!testBit(*bv, i, 0))
            {
                continue;
            }

            // otherwise, get the colour and update the BIGSI
            int *colour = map_get(&bigsi->id2colour, seqID);
            if (colour == NULL)
            {
                fprintf(stderr, "no colour recorded for seq ID: %s\n", seqID);
                return 1;
            }
            if (testBit(bigsi->index[i], *colour, 1))
            {
                fprintf(stderr, "trying to set same bit twice in BIGSI for: %s\n", seqID);
                return 1;
            }
        }
    }

    // wipe the input map of bit vectors as it's not needed anymore (could keep if wanting to re-index)
    map_deinit(&bigsi->bitvectors);
    return 0;
}

/*
    queryBIGSI will determine if any sequences in the BIGSI contain a query k-mer
        bigsi       - the bigsi data structure
        kmer        - the query k-mer
        result      - a user-provided array to return a bit vector of the colours containing the k-mer (must be freed by user)

*/
int queryBIGSI(bigsi_t *bigsi, char *kmer, int kSize, unsigned char *result)
{
    // check the BIGSI is ready for querying
    if (bigsi->index == NULL)
    {
        fprintf(stderr, "need to run the index function first\n");
        return 1;
    }

    // check a k-mer has been provided
    if (!kmer)
    {
        fprintf(stderr, "no k-mer provided\n");
        return 1;
    }

    // check we can send the result
    if (!result)
    {
        fprintf(stderr, "no pointer provided for returning query results\n");
        return 1;
    }

    // prepare the k-mer hashing
    register unsigned int a = murmurhash2(kmer, kSize, 0x9747b28c);
    register unsigned int b = murmurhash2(kmer, kSize, a);
    register unsigned int hv;

    // hash the query k-mer n times, grabbing the corresponding row in the bigsi index
    for (int i = 0; i < bigsi->numHashes; i++)
    {
        // get the hash value
        hv = (a + i * b) % bigsi->numBits;

        // get the corresponding bit vector in the bigsi index
        if (!bigsi->index[hv])
        {
            fprintf(stderr, "missing row in BIGSI for hash value: %d\n", hv);
            return 1;
        }

        // iterate over the bytes in this bit vector
        for (int j = 0; j < bigsi->numColourBytes; j++)
        {

            // if it's the first bit vector for this query, use it as the base for the result
            if (i == 0)
            {
                result[j] = 0x000 | bigsi->index[hv][j];
            }

            // otherwise, bitwise& this bit vector with the previous ones
            else
            {
                result[j] = result[j] & bigsi->index[hv][j];
            }
        }
    }
    return 0;
}

// destroyBIGSI clears up the BIGSI data structure
void destroyBIGSI(bigsi_t *index)
{
    if (!index)
        return;
    index->numBits = 0;
    index->numHashes = 0;
    index->colourIterator = 0;
    if (index->colourArray)
    {
        for (int i = 0; i < index->colourIterator; i++)
        {
            free(index->colourArray[i]);
        }
        free(index->colourArray);
    }
    if (index->index)
    {
        for (int i = 0; i < index->numBits; i++)
        {
            free(index->index[i]);
        }
        free(index->index);
    }
    else
    {
        map_deinit(&index->bitvectors);
    }
    map_deinit(&index->id2colour);
    free(index);
}
