#include <signal.h>
#include <stdio.h>

#include "../src/3rd-party/slog.h"

#include "subcommands.h"
#include "../src/config.h"

/*****************************************************************************
 * stop will stop the daemon. It issues a SIGTERM and updates the config.
 *
 * note:
 *      TODO: instead of using SIGTERM, use waitpid and then decide to
 *              use a SIGKILL before harvesting zombies
 */
int stop(config_t *config)
{
    // check if the daemon is running
    if (checkPID(config) < 0)
    {
        fprintf(stderr, "no daemon currently running\n");
        return -1;
    }

    // kill
    if (kill(config->pid, SIGTERM) != 0)
    {
        fprintf(stderr, "could not kill the daemon process on PID %d\n", config->pid);
        return -1;
    }

    // update
    config->pid = -1;
    if (writeConfig(config, config->filename) != 0)
    {
        fprintf(stderr, "could not update config file after stopping daemon\n");
        return -1;
    }
    return 0;
}

/*****************************************************************************
 * shrink will start the daemon.
 */
int shrink(config_t *config)
{
    // check if the daemon is running
    if (checkPID(config) >= 0)
    {
        slog(0, SLOG_ERROR, "daemon is already running on PID %u", config->pid);
        return -1;
    }

    return 0;
}
