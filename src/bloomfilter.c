#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "bloomfilter.h"
#include "3rd-party/murmurhash3.h"

/*****************************************************************************
 * set helper functions.
 */

// bloomAddOrQuery will query or add to a bloom filter
static inline int bloomAddOrQuery(bloomfilter_t *bf, const void *buffer, int len, uint8_t *result)
{

  // check bloom filter is good to go
  if (bf == NULL)
  {
    return -1;
  }

  // set up the hash calc

  // loop over the hash funcs
  int hits = 0;
  for (int i = 0; i < bf->numHashes; i++)
  {

    // get a hash value
    int hv = getHashVal(buffer, len, i, bvCapacity(bf->bitvector));

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
 *      modulo                    - used to restrict the hash value between 0->modulo
 * 
 * returns:
 *      a hash value
 * 
 * notes:
 *      this is exported so that BIGSI can use the same function for queries
 *      TODO: this isn't ideal and shouldn't be exported, will get a better solution
 */
unsigned int getHashVal(const void *buffer, int len, int iterator, int modulo)
{
  register unsigned int a = murmurhash(buffer, len, 0x9747b28c);
  register unsigned int b = murmurhash(buffer, len, a);
  return ((a + iterator * b) % modulo);
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
 *      the user must free the returned bloom filter
 *      capacity must be > 1000
 * 
 * calculation:
 * 
 *      * Optimal number of bits is:
 *        numBits = (entries * ln(error)) / ln(2)^2
 * 
 *     * Optimal number of hash functions is:
 *        numHashes = bpe * ln(2)
 */
bloomfilter_t *bfInit(uint64_t estCapacity, double collisionProbability)
{

  // check the requested capacity and error is okay
  assert(estCapacity > 1000);
  assert(collisionProbability > 0.0);

  // assign some memory
  bloomfilter_t *bf;
  if ((bf = malloc(sizeof *bf)) == NULL)
  {
    return NULL;
  }
  bf->estCapacity = estCapacity;
  bf->collisionProbability = collisionProbability;

  // calculate the required number of bits per element
  // see: http://en.wikipedia.org/wiki/Bloom_filter
  double num = log(collisionProbability);
  double denom = 0.480453013918201; // ln(2)^2
  double bitsPerElement = -(num / denom);

  // create the bit vector
  uint64_t numBits = (uint64_t)((double)estCapacity * bitsPerElement);
  bf->bitvector = bvInit(numBits);

  // calculate the number of hashes needed
  bf->numHashes = (int)ceil(bitsPerElement * 0.693147180559945); // ln(2)

  return bf;
}

/*****************************************************************************
 * bfAdd will add a value into a bloom filter.
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
  return bloomAddOrQuery(bf, buffer, len, NULL);
}

/*****************************************************************************
 * bfQuery will check a bloom filter for the presence of a value.
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
  return bloomAddOrQuery(bf, buffer, len, result);
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