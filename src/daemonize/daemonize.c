#include <pthread.h> 
#include <stdbool.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // contains fork(3), chdir(3), sysconf(3)
#include <signal.h>   //contains signal(3)
#include <sys/stat.h> // contains umask(3)
//#include <string.h> // contains memset

#include "daemonize.h"
#include "watcher.h"

#include "workerpool.h"
#include "../log/slog.h"

const int NUM_THREADS = 4;
volatile sig_atomic_t done = 1;

// sigTermHandler is called in the event of a SIGTERM signal
void sigTermHandler(int signum, siginfo_t* info, void* arg) {
    slog(0, SLOG_INFO, "sigterm received, shutting down the antman daemon...");
    done = 0;
}

// catchSigterm is used to gracefully exit the daemon when `antman --stop` is called
void catchSigterm() {
    static struct sigaction _sigact;
    memset(&_sigact, 0, sizeof(_sigact));
    _sigact.sa_sigaction = sigTermHandler;
    _sigact.sa_flags = SA_SIGINFO;
    sigaction(SIGTERM, &_sigact, NULL);
}

// startDaemon converts the current program to a daemon process, launches some threads and starts directory watching
int startDaemon(char* daemonName, char* wdir, Config* amConfig) {

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
    amConfig->pid = pid;
    amConfig->running = true;
    if (writeConfig(amConfig, amConfig->configFile) != 0 ) {
        slog(0, SLOG_ERROR, "failed to update config file");
        exit(1);
    }

    // launch the worker threads
    tpool_t* wp;
    wp = tpool_create(NUM_THREADS);
    slog(0, SLOG_INFO, "\t- created workerpool of %d threads", NUM_THREADS);

    // set up the signal catcher
    catchSigterm();

    // do work until the daemon is stopped by a SIGTERM
    while (done)
    {

        // set up the directory watcher
        const FSW_HANDLE handle = fsw_init_session(fsevents_monitor_type);
        
        // add the path(s) for the watcher to watch
        if (FSW_OK != fsw_add_path(handle, amConfig->watchDir)) {
            slog(0, SLOG_ERROR, "could not add a path for libfswatch: %s", amConfig->watchDir);
            return 1;
        }

        // set the watcher callback function
        if (FSW_OK != fsw_set_callback(handle, watcherCallback, wp)) {
            slog(0, SLOG_ERROR, "could not set the callback function for libfswatch");
            return 1;
        }
        slog(0, SLOG_INFO, "\t- set up the directory watcher");

        // start the watcher
        /*
            TODO: put the watcher inside a thread, so that it can be cancelled on signal
        */

        if (FSW_OK != fsw_start_monitor(handle)) {
            slog(0, SLOG_ERROR, "could not start the watcher");
            return 1;
        }
    }

    // clean up once the sigterm is received (which breaks the above while loop, stopping the watcher)
    slog(0, SLOG_INFO, "\t- watcher stopped");
    slog(0, SLOG_INFO, "\t- waiting for threads to finish");
    tpool_wait(wp); // this will wait for threads to finish any work
    slog(0, SLOG_INFO, "\t- cleaning up");
    tpool_destroy(wp);
    return 0;
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