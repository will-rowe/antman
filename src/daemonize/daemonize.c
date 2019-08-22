#include <pthread.h> 
#include <stdbool.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   // contains fork(3), chdir(3), sysconf(3)
#include <signal.h>   //contains signal(3)
#include <sys/stat.h> // contains umask(3)

#include "daemonize.h"
#include "watcher.h"
#include "../log/slog.h"

// launchWatcher is used to start watching the specified directory
void *launchWatcher(void* ddconfig) {

    // cast the void pointer back to a config pointer
    Config* config;
    config = (Config*)ddconfig;

    // setup the watcher and start it going
    slog(0, SLOG_INFO, "\t- starting directory watcher on a new thread");
    if (setupWatcher(config->watchDir) != 0) {
        slog(0, SLOG_ERROR, "failed to launch watcher");
        exit(1);
    }
   pthread_exit(NULL);
}

// miscWorker is a test function to help make sure I have got the hang of c threads!
void *miscWorker(void *threadid) {
    long tid;
    tid = (long)threadid;
    slog(0, SLOG_INFO, "\t- thread 2 (id %d) is being used to make sure the watcher isn't blocking antman", tid);

    int ttl=100;
    int delay=2;
    while( ttl>0 ) {
        sleep(delay);
        //slog(0, SLOG_INFO, "\t- daemon ttl %d", ttl);
        ttl-=delay;
    }
    slog(0, SLOG_INFO, "finished work on thread 2, closing it down");
    pthread_exit(NULL);
}

// startDaemon converts the current program to a daemon process, launches some threads and starts directory watching
void startDaemon(char* daemonName, char* wdir, Config* ddconfig) {

    // try daemonising the program
    int res;
    if( (res=daemonize(daemonName, wdir, NULL, NULL, NULL)) != 0 ) {
        slog(0, SLOG_ERROR, "could not start the antman daemon");
        exit(1);
    }

    // divert log to file
    SlogConfig slgCfg;
    slog_config_get(&slgCfg);
    slgCfg.nToFile = 1;
    slgCfg.nFileStamp = 0;
    slgCfg.nTdSafe = 1;
    slog_config_set(&slgCfg);

    // log some progress
    slog(0, SLOG_INFO, "started the antman daemon");
    pid_t pid = getpid();
    slog(0, SLOG_INFO, "\t- daemon pid: %d", pid);

    // update the config with the PID TODO: this should probably be done in a lock file instead...
    ddconfig->pid = pid;
    ddconfig->running = true;
    if (writeConfig(ddconfig, ddconfig->configFile) != 0 ) {
        slog(0, SLOG_ERROR, "failed to update config file");
        exit(1);
    }

    // launch threads
    slog(0, SLOG_INFO, "creating threads");
    pthread_t threads[2];
    int threadStatus;
    threadStatus = pthread_create(&threads[0], NULL, launchWatcher, (void *)ddconfig);
    if (threadStatus) {
        slog(0, SLOG_ERROR, "could not create thread for the watcher");
        exit(1);
    }
    threadStatus = pthread_create(&threads[1], NULL, miscWorker, (void *)1);
    if (threadStatus) {
        slog(0, SLOG_ERROR, "could not create thread for the watcher");
        exit(1);
    }

    pthread_exit(NULL);
    return;
}

// daemonize is used to fork, detach, fork again, change permissions, change directory and then reopen streams
int daemonize(char* name, char* path, char* outfile, char* errfile, char* infile ) {
    if(!path) { path="/"; }
    if(!name) { name="antman"; }
    if(!infile) { infile="/dev/null"; }
    if(!outfile) { outfile="/dev/null"; }
    if(!errfile) { errfile="/dev/null"; }

    pid_t child;

    //fork, detach from process group leader
    if( (child=fork())<0 ) { 
        //failed fork
        fprintf(stderr,"error: failed fork\n");
        exit(EXIT_FAILURE);
    }
    if (child>0) {
        //parent
        exit(EXIT_SUCCESS);
    }
    if( setsid()<0 ) {
        //failed to become session leader
        fprintf(stderr,"error: failed setsid\n");
        exit(EXIT_FAILURE);
    }

    //catch/ignore signals
    signal(SIGCHLD,SIG_IGN);
    signal(SIGHUP,SIG_IGN);

    //fork second time
    if ( (child=fork())<0) {
        //failed fork
        fprintf(stderr,"error: failed fork\n");
        exit(EXIT_FAILURE);
    }
    if( child>0 ) {
        //parent
        exit(EXIT_SUCCESS);
    }

    //new file permissions
    umask(0);

    //change to path directory
    chdir(path);

    //close all open file descriptors
    int fd;
    for( fd=sysconf(_SC_OPEN_MAX); fd>0; --fd )
    {
        close(fd);
    }

    //reopen stdin, stdout, stderr
    stdin=fopen(infile,"r");   //fd=0
    stdout=fopen(outfile,"w+");  //fd=1
    stderr=fopen(errfile,"w+");  //fd=2

    return(0);
}