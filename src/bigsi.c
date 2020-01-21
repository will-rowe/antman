#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "bigsi.h"
#include "bitvector.c"

/*****************************************************************************
 * bigsInit will initiate a BIGSI.
 * 
 * arguments:
 *      numBits                   - the number of bits per bloom filter
 *      numHashes                 - the number of hashes used per bloom filter
 * 
 * returns:
 *      an initiated BIGSI
 * 
 * note:
 *      the user must free the returned BIGSI
 *      the index is not ready until the bigsIndex function is called
 */
bigsi_t *bigsInit(int numBits, int numHashes)
{
    assert(numBits > 0);
    assert(numHashes > 0);

    bigsi_t *newIndex;
    if ((newIndex = malloc(sizeof *newIndex)) != NULL)
    {
        map_init(&newIndex->id2colour);
        newIndex->colourArray = NULL;
        newIndex->bvArray = NULL;
        newIndex->numBits = numBits;
        newIndex->numHashes = numHashes;
        newIndex->colourIterator = 0;
        newIndex->index = NULL;

        // work out how many bytes are needed to store numBits
        newIndex->numBytes = BV_LOCATE_BYTE((numBits - 1)) + 1;
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

    // wipe the input bit vectors as they're not needed anymore (TODO: could keep if wanting to re-index)
    for (int i = 0; i < bigsi->colourIterator; i++)
    {
        bvDestroy(bigsi->bvArray[i]);
    }
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

        fprintf(stderr, "query for: %s, hash %u = %u\n", ((char *)buffer), i, hv);

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
int bigsDestroy(bigsi_t *index)
{
    if (!index)
    {
        fprintf(stderr, "no BIGSI was provided to bigsDestroy\n");
        return -1;
    }
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
        for (int i = 0; i < index->colourIterator; i++)
        {
            bvDestroy(index->bvArray[i]);
        }
    }
    index->numBits = 0;
    index->numHashes = 0;
    index->colourIterator = 0;
    map_deinit(&index->id2colour);
    free(index);
    return 0;
}
