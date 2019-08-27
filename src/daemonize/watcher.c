#include <string.h>

#include "watcher.h"
#include "workerpool.h"
#include "../fastq/fastq.h"
#include "../log/slog.h"

// getExt takes a filename and returns the extension
char* getExt(const char *filename) {

    // strrchr finds the final occurance of a char
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

// watcherCallback is a test callback function for when the watcher spots a change
void watcherCallback(fsw_cevent const* const events, const unsigned int event_num, void* workerPool) {

    // get the workerpool ready to process an event
    tpool_t* wp;
    wp = (tpool_t*)workerPool; 

    // set the flags from libfswatch that we want to check events against
    int fileCheckList = Created | IsFile;

    //NoOp = 0,                     /**< No event has occurred. */
    //PlatformSpecific = (1 << 0),  /**< Platform-specific placeholder for event type that cannot currently be mapped. */
    //Created = (1 << 1),           /**< An object was created. */
    //Updated = (1 << 2),           /**< An object was updated. */
    //Removed = (1 << 3),           /**< An object was removed. */
    //Renamed = (1 << 4),           /**< An object was renamed. */
    //OwnerModified = (1 << 5),     /**< The owner of an object was modified. */
    //AttributeModified = (1 << 6), /**< The attributes of an object were modified. */
    //MovedFrom = (1 << 7),         /**< An object was moved from this location. */
    //MovedTo = (1 << 8),           /**< An object was moved to this location. */
    //IsFile = (1 << 9),            /**< The object is a file. */
    //IsDir = (1 << 10),            /**< The object is a directory. */
    //IsSymLink = (1 << 11),        /**< The object is a symbolic link. */
    //Link = (1 << 12),             /**< The link count of an object has changed. */
    //Overflow = (1 << 13)          /**< The event queue has overflowed. */

    
    // loop over the events
    unsigned int i, j;
    for(i=0; i < event_num; i++) {
		fsw_cevent const *e = &events[i];

        // check if the event concerns a filetype we are interested in
        // TODO: this is just an extension test for now, will make it more robust...
        char* ext = getExt(e->path);
        if ((strcmp(ext, "fastq") == 0) || (strcmp(ext, "fq") == 0 )) {
            slog(0, SLOG_INFO, "found a FASTQ file");
        }  else {
            continue;
        }

        // combine the flags for the event into a bitmask
        unsigned int setFlags = 0;
		for(j=0; j < e->flags_num; j++) {
            setFlags |= e->flags[j];
		}

        // use the bitmask to determine how to handle the event
        if ((setFlags & fileCheckList) == fileCheckList) {
            slog(0, SLOG_INFO, "\t- filepath: %s", e->path);
        } else {
            if ((setFlags & 1 << 3) == 1 << 3) {
                slog(0, SLOG_INFO, "\t- turns out it was a file being deleted, not created");
            } else {
                slog(0, SLOG_INFO, "\t- looks old, skipping");
            }
            continue;
        }

        // copy the filename to the workerpool queue
        if (!tpool_add_work(wp, printFastq, events[i].path)) {
            slog(0, SLOG_ERROR, "\t- failed to send the file to the workerpool");
        }
	}
}
