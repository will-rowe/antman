#ifndef TEST_CONFIG
#define TEST_CONFIG

#include <stdint.h>
#include <stdio.h>

#include "minunit.h"
#include "../src/config.c"
#include "../src/frozen.c"

#define AM_CONFIG "./tmp.config"
#define ERR_initConf1 "could not init a config instance"
#define ERR_initConf2 "could not write conf file to disk"
#define ERR_initConf3 "could not load conf from disk"
#define ERR_initConf4 "loaded conf file does not match original conf"
#define ERR_checkConf1 "check failed for existing conf file"
#define ERR_checkConf2 "could not create new conf file during the checkConfig function"

int tests_run = 0;

/*
  test the config initialisation and destruction
*/
static char* test_initConf() {

  // create a config
  Config *tmp = initConfig();
  tmp->pid = 666;
  if (tmp == 0) return ERR_initConf1;

  // write it to disk
  if (writeConfig(tmp, AM_CONFIG) != 0 ) return ERR_initConf2;

  // try loading from file
  Config *tmp2 = initConfig();
  if (loadConfig(tmp2, AM_CONFIG) != 0) return ERR_initConf3;
  if (tmp->pid != tmp2->pid) return ERR_initConf4;

  // clean up the test
  destroyConfig(tmp);
  destroyConfig(tmp2);
  remove(AM_CONFIG);
  return 0;
}

/*
  helper function to run all the tests
*/
static char* all_tests() {
  mu_run_test(test_initConf);
  return 0;
}

/*
  entrypoint
*/
int main(int argc, char **argv) {
  fprintf(stderr, "\t\tconfig_test...");
  char *result = all_tests();
  if (result != 0) {
    fprintf(stderr, "failed\n");
    fprintf(stderr, "\ntest function %d failed:\n", tests_run);
    fprintf(stderr, "%s\n", result);
  } else {
    fprintf(stderr, "passed\n");
  }
  return result != 0;
}

#endif