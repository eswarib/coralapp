
#include "stderrSilencer.h"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>

stderrSilencer::stderrSilencer() 
{
    // Save original stderr file descriptor
    originalStderr = dup(STDERR_FILENO);
    // Open /dev/null and redirect stderr
    int devNull = open("/dev/null", O_WRONLY);
    dup2(devNull, STDERR_FILENO);
    close(devNull);
}

stderrSilencer::~stderrSilencer() {
    // Restore original stderr on destruction
    if (originalStderr != -1) {
        dup2(originalStderr, STDERR_FILENO);
        close(originalStderr);
    }
}


