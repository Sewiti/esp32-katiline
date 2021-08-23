#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>
#include <FS.h>

#define TEMP_SUFFIX ".temp"
#define BUFFER_SIZE 32

class Logging
{
protected:
    fs::FS *fs;
    char *path;

    int n; // current records
    int keep_soft;
    int keep_hard;

    bool append(const char *line);
    bool rotateAppend(const char *line);

public:
    Logging(fs::FS *fs, const char *path, int keep_soft, int keep_hard);

    int getCount();
    bool log(const char *line);
};

#endif
