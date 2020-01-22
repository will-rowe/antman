#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitvector.h"

/*****************************************************************************
 * set macros for bit vector operations.
 */

// set the number of bits per byte
#define BITS_PER_BYTE 8

// return the index position of the byte which contains the bit we want
#define BV_LOCATE_BYTE(byte) (byte / BITS_PER_BYTE)

// return the index position of the bit within a byte
#define BV_LOCATE_BIT(bit) (bit % BITS_PER_BYTE)

// return a bit mask with only the specified bit set
#define BV_BIT_MASK(bit) (uint8_t)(0x01 << bit)

// return the value of a specified bit within the bit vector
#define BV_GET_BIT(byte, bit) (uint8_t)((byte >> bit) & 0x01)

// return an updated byte with the specified bit set
#define BV_SET_BIT(byte, bit) byte |= BV_BIT_MASK(bit)

// return an updated byte with the specified bit unset
#define BV_UNSET_BIT(byte, bit) byte &= ~BV_BIT_MASK(bit)

/*****************************************************************************
 * set helper functions.
 */

// bvPopCount will return the number of set bits in a bit vector
static inline uint64_t bvPopCount(const bitvector_t *bv)
{
    uint64_t count = 0;
    for (uint64_t i = 0; i < bv->bufSize; i++)
    {
        // use the builtin popcount function (presence checked for by automake)
        count += __builtin_popcount(bv->buffer[i]);
    }
    return count;
}

// bvCheck is a basic check that a bit vector was initialised properly
static inline int bvCheck(const bitvector_t *bv, uint64_t bit, uint8_t val)
{
    if (bv == NULL)
    {
        fprintf(stderr, "no bit vector was provided\n");
        return -1;
    }
    if ((bv->count + val) > bv->capacity)
    {
        fprintf(stderr, "bit vector at capacity\n");
        return -1;
    }

    if (bit >= bv->capacity)
    {
        fprintf(stderr, "requested bit index exceeds bit vector capacity (%lu vs %lu)\n", bit, bv->capacity);
        return -1;
    }

    if (val > 1)
    {
        fprintf(stderr, "can only set bit to 1 or 0, not %u\n", val);
        return -1;
    }

    return 0;
}

// bvHelper will set or query the bit vector
static inline int bvHelper(bitvector_t *bv, uint64_t bit, uint8_t *val, uint8_t *result)
{
    // check we are good to go
    if (bvCheck(bv, bit, 0) != 0)
    {
        return -1;
    }

    // get the correct index positions in the vector for the bit
    uint64_t byteIndex = BV_LOCATE_BYTE(bit);
    uint8_t bitIndex = BV_LOCATE_BIT(bit);

    // check that the index positions are going to work
    assert(bitIndex < BITS_PER_BYTE && byteIndex < bv->bufSize);

    // if it is a get request
    if (result)
    {
        // get the byte
        *result = BV_GET_BIT(bv->buffer[byteIndex], bitIndex);
    }

    // if it is a set request
    if (val)
    {
        // set the byte
        if (*val == 1)
        {
            BV_SET_BIT(bv->buffer[byteIndex], bitIndex);
            bv->count++;
        }
        else

        // unset the byte
        {
            BV_UNSET_BIT(bv->buffer[byteIndex], bitIndex);
            bv->count--;
        }
    }

    return 0;
}

// bvBitwise will perform some bitwise operations
static inline int bvBitwise(const bitvector_t *bv1, const bitvector_t *bv2, bitvector_t *result, int band, int bor, int xor, int bandUpdate)
{
    if ((band + bor + xor+bandUpdate) != 1)
    {
        fprintf(stderr, "provide bvBitwise function with ONE of: band, bor, xor, bandUpdate\n");
        return -1;
    }

    // do checks
    if (!bandUpdate)
    {
        if ((bv1 == NULL) || (bv2 == NULL) || (result == NULL))
        {
            fprintf(stderr, "too few bit vectors were provided for bitwise operation\n");
            return -1;
        }
        if (result->count > 0)
        {
            fprintf(stderr, "bit vector for the result is not empty\n");
            return -1;
        }
        if ((bv1->capacity != bv2->capacity) || (bv2->capacity != result->capacity))
        {
            fprintf(stderr, "provided bit vectors have different capacities: %lu, %lu and %lu\n", bv1->capacity, bv2->capacity, result->capacity);
            return -1;
        }
    }
    else
    {
        if (bv2->capacity != result->capacity)
        {
            fprintf(stderr, "provided bit vectors have different capacities: %lu and %lu\n", bv2->capacity, result->capacity);
            return -1;
        }
    }

    // cycle through the bytes
    for (uint64_t i = 0; i < result->bufSize; i++)
    {
        if (band == 1)
        {
            if (bv1 == NULL)
            {
                result->buffer[i] &= bv2->buffer[i];
            }
            else
            {
                result->buffer[i] = bv1->buffer[i] & bv2->buffer[i];
            }
        }
        if (bor == 1)
        {
            result->buffer[i] = bv1->buffer[i] | bv2->buffer[i];
        }
        if (xor == 1)
        {
            result->buffer[i] = bv1->buffer[i] ^ bv2->buffer[i];
        }
    }

    // update the bit count
    // TODO: should I do this during the previous loop?
    result->count = bvPopCount(result);
    return 0;
}

/*****************************************************************************
 * bvInit will initiate a bit vector.
 * 
 * arguments:
 *      capacity - the maximum number of bits the bit vector should hold
 * 
 * returns:
 *      an initiated bit vector
 * 
 * note:
 *      the user must free the returned bit vector
 */
bitvector_t *bvInit(uint64_t capacity)
{
    // check the requested capacity is okay
    assert(capacity > 0);

    // work out how many bytes are needed to store the requested bit capacity
    uint64_t bufSize = BV_LOCATE_BYTE((capacity - 1)) + 1;

    // allocate memory for the bit vector struct
    bitvector_t *bv;
    if ((bv = calloc(sizeof(bitvector_t) + sizeof(uint8_t), bufSize)) != NULL)
    {
        bv->capacity = capacity;
        bv->count = 0;
        bv->bufSize = bufSize;
    }
    return bv;
}

/*****************************************************************************
 * bvClone will initiate a bit vector which is a clone of an existing bit
 * vector.
 * 
 * arguments:
 *      bv - the bit vector to be cloned
 * 
 * returns:
 *      an initiated bit vector
 * 
 * note:
 *      the user must free the returned bit vector
 */
bitvector_t *bvClone(const bitvector_t *bv)
{
    // check the input bit vector
    assert(bv != NULL);
    assert(bv->capacity > 0);

    // allocate memory for the bit vector struct
    bitvector_t *clone;
    if ((clone = calloc(sizeof(bitvector_t) + sizeof(uint8_t), bv->bufSize)) != NULL)
    {
        clone->capacity = bv->capacity;
        clone->bufSize = bv->bufSize;
    }

    // copy the buffer
    for (uint64_t i = 0; i < bv->bufSize; i++)
    {
        clone->buffer[i] |= bv->buffer[i];
    }
    clone->count = bvPopCount(clone);

    // check that the buffer copied
    assert(bv->count == clone->count);
    return clone;
}

/*****************************************************************************
 * bvCapacity will return the number of bits a bit vector can hold.
 * 
 * arguments:
 *      bv - the query bit vector
 * 
 * returns:
 *      maximum number of bits the input bit vector can hold
 */
uint64_t bvCapacity(const bitvector_t *bv)
{
    assert(bv != NULL);
    return bv->capacity;
}

/*****************************************************************************
 * bvCount will return the number of bits set in the bit vector.
 * 
 * arguments:
 *      bv - the query bit vector
 * 
 * returns:
 *      number of bits set
 */
uint64_t bvCount(const bitvector_t *bv)
{
    assert(bv != NULL);
    return bv->count;
}

/*****************************************************************************
 * bvSet will set the specified bit to 1 or 0.
 * 
 * arguments:
 *      bv  - the query bit vector
 *      bit - the bit index to set
 *      val - the value to set the bit to (1 or 0)
 * 
 * returns:
 *      0 on success, -1 on error
 * 
 * note:
 *      0 based indexing of bits
 */
int bvSet(bitvector_t *bv, uint64_t bit, uint8_t val)
{
    return bvHelper(bv, bit, &val, NULL);
}

/*****************************************************************************
 * bvGet will will check the value of the specified bit.
 * 
 * arguments:
 *      bv      - the query bit vector
 *      bit     - the bit index to get
 *      result  - a pointer to an uint8, used to return the bit value
 * 
 * returns:
 *      0 on success, -1 on error
 * 
 * note:
 *      0 based indexing of bits
 */
int bvGet(bitvector_t *bv, uint64_t bit, uint8_t *result)
{
    return bvHelper(bv, bit, NULL, result);
}

/*****************************************************************************
 * bvClear will clear a bit vector but keep it initialised, allowing it to be reused.
 * 
 * arguments:
 *      bv      - the query bit vector
 * 
 * returns:
 *      0 on success, -1 on error
 */
int bvClear(bitvector_t *bv)
{
    if (!bv)
    {
        fprintf(stderr, "no bit vector was provided to bvClear\n");
        return -1;
    }
    if (bv->count != 0)
    {
        for (uint64_t i = 0; i < bv->bufSize; i++)
        {
            bv->buffer[i] = 0x000;
        }
        bv->count = 0;
    }
    return 0;
}

/*****************************************************************************
 * bvDestroy will destroy a bit vector and release the memory.
 * 
 * arguments:
 *      bv      - the query bit vector
 * 
 * returns:
 *      0 on success, -1 on error
 */
int bvDestroy(bitvector_t *bv)
{
    if (!bv)
    {
        fprintf(stderr, "no bit vector was provided to bvDestroy\n");
        return -1;
    }
    free(bv);
    return 0;
}

/*****************************************************************************
 * bvPrint will print a summary of a bit vector.
 * 
 * arguments:
 *      bv      - the query bit vector
 */
void bvPrint(bitvector_t *bv)
{
    printf("bit vector summary->\n- set bits: %lu\n- bit capacity: %lu\n- bytes reserved: %lu\nbit vector->\n", bv->count, bv->capacity, bv->bufSize);
    if (bv == NULL)
    {
        printf("NULL\n");
        return;
    }
    uint8_t val = 0;
    for (uint64_t i = 0; i < bv->capacity; i++)
    {
        bvGet(bv, i, &val);
        printf("%u", val);
    }
    printf("\n\n");
}

/*****************************************************************************
 * bvBAND will bitwise AND two bit vectors.
 * 
 * arguments:
 *      bv1      - the first bit vector
 *      bv2      - the second bit vector
 *      result   - an empty bit vector, used to return the result 
 * 
 * returns:
 *      0 on success, -1 on error
 */
int bvBAND(const bitvector_t *bv1, const bitvector_t *bv2, bitvector_t *result)
{
    return bvBitwise(bv1, bv2, result, 1, 0, 0, 0);
}

/*****************************************************************************
 * bvBOR will bitwise OR two bit vectors.
 * 
 * arguments:
 *      bv1      - the first bit vector
 *      bv2      - the second bit vector
 *      result   - an empty bit vector, used to return the result 
 * 
 * returns:
 *      0 on success, -1 on error
 */
int bvBOR(const bitvector_t *bv1, const bitvector_t *bv2, bitvector_t *result)
{
    return bvBitwise(bv1, bv2, result, 0, 1, 0, 0);
}

/*****************************************************************************
 * bvXOR will bitwise XOR two bit vectors.
 * 
 * arguments:
 *      bv1      - the first bit vector
 *      bv2      - the second bit vector
 *      result   - an empty bit vector, used to return the result 
 * 
 * returns:
 *      0 on success, -1 on error
 */
int bvXOR(const bitvector_t *bv1, const bitvector_t *bv2, bitvector_t *result)
{
    return bvBitwise(bv1, bv2, result, 0, 0, 1, 0);
}

/*****************************************************************************
 * bvBANDupdate will bitwise AND two bit vectors, storing the answer in the
 * first bit vector
 * 
 * arguments:
 *      bv1      - the first bit vector (which is updated)
 *      bv2      - the second bit vector
 * 
 * returns:
 *      0 on success, -1 on error
 */
int bvBANDupdate(bitvector_t *bv1, const bitvector_t *bv2)
{
    return bvBitwise(NULL, bv2, bv1, 0, 0, 0, 1);
}
