#ifndef TEST_BITVECTOR
#define TEST_BITVECTOR

#include <stdlib.h>
#include <stdio.h>

#include "minunit.h"
#include "../bitvector.h"

#define CAPACITY 10
#define CAPACITY_IN_BYTES 2
#define ERR_init "could not init a bit vector"
#define ERR_capacity "incorrect capacity reported by bit vector"
#define ERR_count "incorrect count reported by bit vector"
#define ERR_destroy "could not destroy the bit vector"
#define ERR_set "could not set bit in bit vector"
#define ERR_set2 "set bit exceeding bit vector capacity"
#define ERR_get "could not get bit in bit vector"
#define ERR_get2 "incorrect bit set or returned from bit vector"
#define ERR_fill "could not fill bit vector"
#define ERR_empty "could not empty bit vector"
#define ERR_clear "could not clear the bit vector"
#define ERR_band "could not run a bitwise AND"
#define ERR_band2 "incorrect bit set after bitwise AND"
#define ERR_clone "could not clone a bit vector"

int tests_run = 0;

/*
  test the bit vector initialisation and destruction
*/
static char *test_initBV()
{
    // init a bit vector
    bitvector_t *bv = bvInit(CAPACITY);
    if (bv == NULL)
    {
        return ERR_init;
    }

    // check capacity set
    if (bv->capacity != CAPACITY)
    {
        return ERR_capacity;
    }
    if (bv->bufSize != CAPACITY_IN_BYTES)
    {
        return ERR_capacity;
    }

    // check count is zeroed
    if (bv->count != 0)
    {
        return ERR_count;
    }

    // clean up the test
    if (bvDestroy(bv) != 0)
    {
        return ERR_destroy;
    }
    return 0;
}

/*
  test the set and get bit vector functions
*/
static char *test_bv_funcs()
{
    // init a bit vector
    bitvector_t *bv = bvInit(CAPACITY);
    if (bv == NULL)
    {
        return ERR_init;
    }

    // set a bit
    if (bvSet(bv, 4, 1) != 0)
    {
        return ERR_set;
    }

    // check count is updated
    if (bv->count != 1)
    {
        return ERR_count;
    }

    // check the bit
    uint8_t bitVal = 0;
    if (bvGet(bv, 4, &bitVal) != 0)
    {
        return ERR_get;
    }
    if (bitVal != 1)
    {
        bvPrint(bv);
        return ERR_get2;
    }

    // make sure you can't set a bit which exceedes capacity of the bit vector
    if (bvSet(bv, CAPACITY, 1) == 0)
    {
        return ERR_set2;
    }

    // clean up the test
    if (bvDestroy(bv) != 0)
    {
        return ERR_destroy;
    }
    return 0;
}

/*
    test filling and emptying
*/
static char *test_fillempty()
{
    // init a bit vector
    bitvector_t *bv = bvInit(CAPACITY);
    if (bv == NULL)
    {
        return ERR_init;
    }

    // fill it, checking each set func
    for (int i = 0; i < CAPACITY; i++)
    {
        if (bvSet(bv, i, 1) != 0)
        {
            return ERR_set;
        }
        uint8_t bitVal = 0;
        if (bvGet(bv, i, &bitVal) != 0)
        {
            return ERR_get;
        }
        if (bitVal != 1)
        {
            return ERR_get2;
        }
        //bvPrint(bv);
    }
    if (bv->count != CAPACITY)
    {
        return ERR_fill;
    }

    // empty it, checking each set func
    for (int i = CAPACITY - 1; i >= 0; i--)
    {
        if (bvSet(bv, i, 0) != 0)
        {
            return ERR_set;
        }
        uint8_t bitVal = 0;
        if (bvGet(bv, i, &bitVal) != 0)
        {
            return ERR_get;
        }
        if (bitVal != 0)
        {
            return ERR_get2;
        }
        if (bvCount(bv) != i)
        {
            return ERR_empty;
        }
        //bvPrint(bv);
    }
    if (bv->count != 0)
    {

        return ERR_empty;
    }

    // clean up the test
    if (bvDestroy(bv) != 0)
    {
        return ERR_destroy;
    }
    return 0;
}

/*
    test clear
*/
static char *test_clear()
{
    // init a bit vector
    bitvector_t *bv = bvInit(CAPACITY);
    if (bv == NULL)
    {
        return ERR_init;
    }

    // add a bit, then clear and check
    uint8_t bitVal = 1;
    bvSet(bv, 4, bitVal);
    if (bvClear(bv) != 0)
    {
        return ERR_clear;
    }
    bvGet(bv, 4, &bitVal);
    if (bitVal != 0)
    {
        return ERR_get2;
    }
    if (bv->count != 0)
    {
        return ERR_clear;
    }

    // clean up the test
    if (bvDestroy(bv) != 0)
    {
        return ERR_destroy;
    }
    return 0;
}

/*
    test bitwise AND
*/
static char *test_band()
{
    // init 3 bit vectors
    bitvector_t *bv1 = bvInit(CAPACITY);
    bitvector_t *bv2 = bvInit(CAPACITY);
    bitvector_t *bv3 = bvInit(CAPACITY);
    if ((bv1 == NULL) || (bv2 == NULL) || (bv3 == NULL))
    {
        return ERR_init;
    }
    bvSet(bv1, 2, 1);
    bvSet(bv2, 2, 1);
    bvSet(bv1, 3, 1);
    bvSet(bv2, 3, 1);
    bvSet(bv2, 4, 1);
    bvSet(bv1, 9, 1);
    bvSet(bv2, 9, 1);

    // bitwise AND them
    if (bvBAND(bv1, bv2, bv3) != 0)
    {
        return ERR_band;
    }

    // result should have bits 2, 3 and 9 set; 4 should be unset
    uint8_t val1 = 0;
    uint8_t val2 = 0;
    uint8_t val3 = 0;
    uint8_t val4 = 0;
    bvGet(bv3, 2, &val1);
    bvGet(bv3, 3, &val2);
    bvGet(bv3, 4, &val3);
    bvGet(bv3, 9, &val4);
    if ((val1 != 1) || (val2 != 1) || (val3 != 0) || (val4 != 1))
    {
        bvPrint(bv3);
        return ERR_band2;
    }

    // the result should also have had the count updated
    if (bvCount(bv3) != 3)
    {
        bvPrint(bv3);
        return ERR_count;
    }

    // check the clone func while we are at it
    bitvector_t *bv4 = bvClone(bv3);
    if (bvCount(bv4) != 3)
    {
        bvPrint(bv4);
        return ERR_clone;
    }

    // check the bitwise AND update
    if (bvBANDupdate(bv3, bv4) != 0)
    {
        return ERR_band;
    }
    if (bvCount(bv3) != bvCount(bv4))
    {
        return ERR_count;
    }

    // clean up the test
    if ((bvDestroy(bv1) != 0) || (bvDestroy(bv2) != 0) || (bvDestroy(bv3) != 0) || (bvDestroy(bv4) != 0))
    {
        return ERR_destroy;
    }
    return 0;
}

/*
    helper function to run all the tests
*/
static char *all_tests()
{
    mu_run_test(test_initBV);
    mu_run_test(test_bv_funcs);
    mu_run_test(test_fillempty);
    mu_run_test(test_clear);
    mu_run_test(test_band);
    return 0;
}

/*
    entrypoint
*/
int main(int argc, char **argv)
{
    fprintf(stderr, "\t\tbitvector_test...");
    char *result = all_tests();
    if (result != 0)
    {
        fprintf(stderr, "failed\n");
        fprintf(stderr, "\ntest function %d failed:\n", tests_run);
        fprintf(stderr, "%s\n", result);
    }
    else
    {
        fprintf(stderr, "passed\n");
    }
    return result != 0;
}

#endif