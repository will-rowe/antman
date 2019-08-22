#include <string.h>
#include "watcher.h"
#include "../log/slog.h"

// getExt takes a filename and returns the extension
char* getExt(const char *filename) {

    // strrchr finds the final occurance of a char
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

// watcherCallback is a test callback function for when the watcher spots a change
void watcherCallback(fsw_cevent const * const events, const unsigned int event_num, void * data) {


    // TODO: need to make a copy of any event if I plan to use later...
    
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
        slog(0, SLOG_INFO, "\t- sending file on to do cool stuff with");
	}
}

// setupWatcher starts a watcher on the specified directory
int setupWatcher(char* watchPath) {
    const FSW_HANDLE handle = fsw_init_session(fsevents_monitor_type);
    
    // add the path(s) for the watcher to watch
    if (FSW_OK != fsw_add_path(handle, watchPath)) {
        slog(0, SLOG_ERROR, "could not add a path for libfswatch: %s", watchPath);
        return 1;
    }

    // set the watcher callback function
    if (FSW_OK != fsw_set_callback(handle, watcherCallback, NULL)) {
        slog(0, SLOG_ERROR, "could not set the callback function for libfswatch");
        return 1;
    }

    // start the watcher
    if (FSW_OK != fsw_start_monitor(handle)) {
        slog(0, SLOG_ERROR, "could not start the watcher");
        return 1;
    }
    return 0;
}


