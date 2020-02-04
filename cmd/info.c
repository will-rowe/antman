#include <signal.h>
#include <stdio.h>

#include "subcommands.h"

/*****************************************************************************
 * info is the subcommand function.
 */
int info(config_t *config, bool pidOnly)
{
    // check the PID
    int pid;
    pid = checkPID(config);
    if (pid == -2)
    {
        fprintf(stderr, "error: registered daemon not running and can't update config\n");
        return -1;
    }

    // print the pid
    if (pidOnly)
    {
        fprintf(stdout, "%d\n", pid);
        return 0;
    }

    // TODO: otherwise just print the summary information
    else
    {
        fprintf(stdout, "\n%s version %s\n------------------\n", PROG_NAME, PROG_VERSION);
        fprintf(stdout, "config: %s\n", CONFIG_LOCATION);
        fprintf(stdout, "created on: %s\n", config->created);
        fprintf(stdout, "modified on: %s\n", config->modified);
        fprintf(stdout, "watch directory: %s\n", config->watchDir);
        fprintf(stdout, "white list: %s\n", config->white_list);
        fprintf(stdout, "current log file: %s\n", config->currentLogFile);
        if (pid != -1)
        {
            fprintf(stdout, "daemon running: %d\n", pid);
        }
        else
        {
            fprintf(stdout, "daemon running: false\n");
        }
        putchar('\n');
    }

    return 0;
}

/*****************************************************************************
 * checkPID will check if the daemon is running and return the PID or error.
 * 
 * arguments:
 *      capacity - the maximum number of bits the bit vector should hold
 * 
 * returns:
 *      >=0     - the PID that the daemon is running on
 *      -1      - indicates no running daemon
 *      -2      - error updating the config (registered daemon not running)
 * 
 */
int checkPID(config_t *amConfig)
{

    // check if there is a registered pid
    if (amConfig->pid >= 0)
    {

        // check to see if the registered pid is actually running
        if (kill(amConfig->pid, 0) != 0)
        {

            // if not running, update the config
            amConfig->pid = -1;
            if (writeConfig(amConfig, amConfig->filename) != 0)
            {
                return -2;
            }
        }
        return amConfig->pid;
    }
    return -1;
}