#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>
#include <FS.h>

class Logging
{
protected:
    fs::FS *fs;
    char *name;

    int n; // lines count in last file

    int maxFiles;
    int maxPerFile;

    bool append(int log, const char *line);
    bool rotate();

public:
    Logging(fs::FS *fs, const char *dirPath, int files, int perFile);

    bool log(const char *line);

    int getFileLinesCount(int log);
    int getFilesCount();
    String getPath(int log);
};

#endif
