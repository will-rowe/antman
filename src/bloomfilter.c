#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "3rd-party/murmurhash3.h"

#include "bloomfilter.h"

/*****************************************************************************
 * set helper functions.
 */

// bloomAddOrQuery will query or add to a bloom filter
static inline int bloomAddOrQuery(bloomfilter_t *bf, uint64_t *hashValues, uint8_t *result)
{

  // check bloom filter is good to go
  if (bf == NULL)
  {
    return -1;
  }

  // loop over the hash values
  int hits = 0;
  for (int i = 0; i < bf->numHashes; i++)
  {

    // get the bit vector location
    uint64_t hv = *(hashValues + i) % bvCapacity(bf->bitvector);

    // if we're adding, set the bit
    if (result == NULL)
    {
      if (bvSet(bf->bitvector, hv, 1) != 0)
      {
        return -1;
      }
    }

    // otherwise, we're querying
    else
    {
      uint8_t hit = 0;
      if (bvGet(bf->bitvector, hv, &hit) != 0)
      {
        return -1;
      }
      hits += hit;
    }
  }
  if (hits == bf->numHashes)
  {
    *result = 1;
  }
  return 0;
}

/*****************************************************************************
 * getHashVal will return a hash value for a given buffer.
 * 
 * arguments:
 *      buffer                    - the thing to hash
 *      len                       - the length of the buffer
 *      iterator                  - used to vary the hash for the same buffer
 * 
 * returns:
 *      a hash value
 */
unsigned int getHashVal(const void *buffer, int len, int iterator)
{
  register unsigned int a = murmurhash(buffer, len, 0x9747b28c);
  register unsigned int b = murmurhash(buffer, len, a);
  return (a + iterator * b);
}

/*****************************************************************************
 * bfCalc will calculate the number of bits and hash functions required.
 * 
 * arguments:
 *      estCapacity               - the maximum number of entries the bloom filter should expect
 *      collisionProbability      - collision probibility
 *      numBitsPtr                - pointer to numBits for result
 *      numHashesPtr              - pointer to numHashes for result
 * 
 * returns:
 *      0 on success, -1 on error
 * 
 * note:
 *      * Optimal number of bits is:
 *        numBits = (entries * ln(error)) / ln(2)^2
 * 
 *      * Optimal number of hash functions is:
 *        numHashes = bpe * ln(2)
 */
int bfCalc(uint64_t estCapacity, double collisionProbability, uint64_t *numBitsPtr, uint64_t *numHashesPtr)
{

  // check the requested capacity and error is okay
  if (estCapacity < 1000)
  {
    fprintf(stderr, "estimated capacity must be > 1000\n");
    return -1;
  }
  if (collisionProbability <= 0.0)
  {
    fprintf(stderr, "collision probability must be > 0.0\n");
    return -1;
  }

  // calculate the required number of bits per element
  // see: http://en.wikipedia.org/wiki/Bloom_filter
  double num = log(collisionProbability);
  double denom = 0.480453013918201; // ln(2)^2
  double bitsPerElement = -(num / denom);

  // create the bit vector
  *numBitsPtr = (uint64_t)((double)estCapacity * bitsPerElement);

  // calculate the number of hashes needed
  *numHashesPtr = (int)ceil(bitsPerElement * 0.693147180559945); // ln(2)
  return 0;
}

/*****************************************************************************
 * bfInit will initiate a bloom filter.
 * 
 * arguments:
 *      estCapacity               - the maximum number of entries the bloom filter should expect
 *      collisionProbability      - collision probibility
 * 
 * returns:
 *      an initiated bloom filter
 * 
 * note:
 *      the user must check and free the returned bloom filter
 */
bloomfilter_t *bfInit(uint64_t estCapacity, double collisionProbability)
{

  // assign some memory
  bloomfilter_t *bf;
  if ((bf = malloc(sizeof *bf)) == NULL)
  {
    return NULL;
  }
  bf->estCapacity = estCapacity;
  bf->collisionProbability = collisionProbability;
  if (bfCalc(estCapacity, collisionProbability, &bf->numBits, &bf->numHashes) != 0)
  {
    free(bf);
    return NULL;
  }
  bf->bitvector = bvInit(bf->numBits);
  return bf;
}

/*****************************************************************************
 * bfInitWithSize will initiate a bloom filter with defined number of bits
 * and hash functions.
 * 
 * arguments:
 *      numBits                   - size of the bit vector
 *      numHash                   - number of hashes to use
 * 
 * returns:
 *      an initiated bloom filter
 * 
 * note:
 *      the user must check and free the returned bloom filter
 */
bloomfilter_t *bfInitWithSize(uint64_t numBits, uint64_t numHashes)
{

  // assign some memory
  bloomfilter_t *bf;
  if ((bf = malloc(sizeof *bf)) == NULL)
  {
    return NULL;
  }
  bf->estCapacity = 0;
  bf->collisionProbability = 0;
  bf->numBits = numBits;
  bf->numHashes = numHashes;
  bf->bitvector = bvInit(numBits);
  return bf;
}

/*****************************************************************************
 * bfAdd will add hash a query and insert the hashes into the bloom filter.
 * 
 * arguments:
 *      bf      - the bloom filter
 *      buffer  - the value to check for
 *      len     - the length of the buffer
 * 
 * returns:
 *      0 on success, -1 on error
 */
int bfAdd(bloomfilter_t *bf, const void *buffer, uint64_t len)
{

  // set up the hash values array
  uint64_t *hashValues;
  hashValues = calloc(sizeof(uint64_t), bf->numHashes);
  if (!hashValues)
    return -1;

  // hash the input
  for (int i = 0; i < bf->numHashes; i++)
    *(hashValues + i) = getHashVal(buffer, len, i);

  // add the hashes to the bloom filter
  int retVal = 0;
  retVal = bloomAddOrQuery(bf, hashValues, NULL);
  free(hashValues);
  return retVal;
}

/*****************************************************************************
 * bfQuery will hash a query, check the bloom filter and set a result to true
 * if all hashes exist in the filter for the query.
 * 
 * arguments:
 *      bf      - the bloom filter
 *      buffer  - the value to check for
 *      len     - the length of the buffer
 *      result  - a pointer to an uint8, used to return query presence (1) or absence (0)
 * 
 * returns:
 *      0 on success, -1 on error
 */
int bfQuery(bloomfilter_t *bf, const void *buffer, uint64_t len, uint8_t *result)
{
  // set up the hash values array
  uint64_t *hashValues;
  hashValues = calloc(sizeof(uint64_t), bf->numHashes);
  if (!hashValues)
    return -1;

  // hash the input
  for (int i = 0; i < bf->numHashes; i++)
    *(hashValues + i) = getHashVal(buffer, len, i);

  // add the hashes to the bloom filter
  int retVal = 0;
  retVal = bloomAddOrQuery(bf, hashValues, result);
  free(hashValues);
  return retVal;
}

/*****************************************************************************
 * bfAddPC will add precomputed hash values to the bloom filter.
 * 
 * arguments:
 *      bf          - the bloom filter
 *      hashValues  - the hash values to add
 *      len         - the number of hash values being added
 * 
 * returns:
 *      0 on success, -1 on error
 * 
 * note:
 *    it's the caller's job to use a consistent hash function for a bloom filter instance
 */
int bfAddPC(bloomfilter_t *bf, uint64_t *hashValues, unsigned int len)
{
  assert(len == bf->numHashes);
  return bloomAddOrQuery(bf, hashValues, NULL);
}

/*****************************************************************************
 * bfQueryPC will check the bloom filter and set a result to true
 * if all precomputed hashes exist in the filter for the query.
 * 
 * arguments:
 *      bf          - the bloom filter
 *      hashValues  - the hash values to add
 *      len         - the number of hash values being added
 * 
 * returns:
 *      0 on success, -1 on error
 * 
 * note:
 *    it's the caller's job to use a consistent hash function for a bloom filter instance
 */
int bfQueryPC(bloomfilter_t *bf, uint64_t *hashValues, unsigned int len, uint8_t *result)
{
  assert(len == bf->numHashes);
  return bloomAddOrQuery(bf, hashValues, result);
}

/*****************************************************************************
 * bfDestroy will clear a bloom filter and release the memory.
 * 
 * arguments:
 *      bf      - the bloom filter
 * 
 * returns:
 *      0 on success, -1 on error
 */
int bfDestroy(bloomfilter_t *bf)
{
  if (!bf)
  {
    fprintf(stderr, "no bloom filter was provided to bfDestroy\n");
    return -1;
  }
  if (bvDestroy(bf->bitvector) != 0)
  {
    return -1;
  }
  free(bf);
  return 0;
}