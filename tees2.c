#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "tlpi_hdr.h"

#define BUF_SIZE 1024

#ifdef __GNUC__
__attribute__((__noreturn__))
#endif

int main(int argc, char * argv[])
{
    int fd[argc];
    mode_t modes, modFile, modeAdd;
    static char * buffer;
    ssize_t numWritten, numWrite, numRead;

    modes = O_RDWR | O_CREAT;
    modFile = S_IRUSR | S_IWUSR;
    modeAdd = O_WRONLY | O_CREAT | O_APPEND | S_IRUSR | S_IWUSR;

    printf("Mode File %d\n", modes);
    printf("MOde FILE %d\n", modFile);
    printf("Mode Append %d\n", modeAdd);
    getpid();
    if(argc < 2 || strcmp(argv[1], "--help") == 0)
        usageErr("Terminal command for write in files %s\n", argv[0]);

    int flag = strcmp(argv[1], "--a");
    if(flag == 0)
        modes = O_RDWR | O_CREAT | O_APPEND;

    buffer = malloc(BUF_SIZE);
    if(buffer == NULL)
        errExit("Malloc for buffer");

    for(int index = 1; index < argc; index++)
    {
        fd[index] = open(argv[index], modes, modFile);
        if(fd[index] == -1)
            errExit("Open file for write");
    }
    while((numRead = read(STDIN_FILENO, buffer, BUF_SIZE)) > 0)
    {
        if((numWrite = write(STDOUT_FILENO, buffer, numRead)) > 0)
            errExit("Output cmd");
        for(int count = 1; count < argc; count++)
        {
            numWritten = write(fd[count], buffer, numRead);
            if(numWritten == -1)
            {
                printf("File name: %s\n", argv[count]);
                errExit("Write in file 3");
            }
        }
    }

    for(int i = argc -1; i >= 1; --i)
        close(fd[i]);
    free(buffer);
    buffer = NULL;

    exit(EXIT_SUCCESS);
}

static void terminate(bool useExit3)
{
    char * s;
    s = getenv("EF_DUMPCORE");
    if (s != NULL && *s != '\0')
        abort();
    else if (useExit3)
        exit(EXIT_FAILURE);
    else
        _exit(EXIT_FAILURE);
}

static void outputError(bool useErr, int err, bool flushStdout, const char * format, va_list ap)
{
    //    #define BUF_SIZE 500
    char buf[BUF_SIZE];
    char userMsg[BUF_SIZE];
    char errText[BUF_SIZE];
    vsnprintf(userMsg, BUF_SIZE, format, ap);

    if (useErr)
        snprintf(errText, BUF_SIZE, " [%s %s]",
                 (err > 0 && err <= MAX_ENAME) ?
                 ename[err] : "?UNKNOWN?", strerror(err));
        else
            snprintf(errText, BUF_SIZE, "ERRORS %s\n", errText, userMsg);
    if (flushStdout)
        fflush(stdout);
    fputs(buf, stderr);
    fflush(stderr);
}

static void gnFail(const char * fname, const char * msg, const char * arg, const char * name)
{
    fprintf(stderr, "%s error", fname);
    if (name != NULL)
        fprintf(stderr, " (in %s)", name);
    fprintf(stderr, ": %s\n", msg);
    if (arg != NULL && *arg != '\0')
        fprintf(stderr, "offending text: %s\n", arg);
    exit(EXIT_FAILURE);
}

static long getNum(const char * fname, const char * arg, int flags, const char * name)
{
    long res;
    char * endptr;
    int base;
    if (arg == NULL || *arg == '\0')
        gnFail(fname, "null or empty string", arg, name);
    base = (flags & GN_ANY_BASE) ? 0 : (flags & GN_BASE_8) ? 8 :
    (flags & GN_BASE_16) ? 16 : 10;
    errno = 0;
    res = strtol(arg, &endptr, base);
    if (errno != 0)
        gnFail(fname, "strtol() failed", arg, name);
    if ((*endptr != '\0'))
        gnFail(fname, "nonnumeric characters", arg, name);
    if ((flags & GN_NONNEG) && res < 0)
        gnFail(fname, "negative value not allowed", arg, name);
    if ((flags & GN_GT_0) && res <= 0)
        gnFail(fname, "value must be > 0", arg, name);
    return res;
}

long getLong(const char * arg, int flags, const char * name)
{
    return getNum("getLong", arg, flags, name);
}

int getInt(const char * arg, int flags, const char * name)
{
    long res;
    res = getNum("getInt", arg, flags, name);
    if (res > INT_MAX || res < INT_MIN)
        gnFail("GetInt", "Integer out of range", arg, name);
    return (int) res;
}

void errExit(const char * format, ...)
{
    va_list argList;
    va_start(argList, format);
    outputError(true, errno, false, format, argList);
    va_end(argList);
    terminate(true);
}

void errMsg(const char * format, ...)
{
    va_list argList;
    int savedErrno;
    savedErrno = errno;
    va_start(argList, format);
    outputError(true, errno, true, format, argList);
    va_end(argList);
    errno = savedErrno;
}

void err_exit(const char * format, ...)
{
    va_list argList;
    va_start(argList, format);
    outputError(true, errno, true, format, argList);
    va_end(argList);
    terminate(false);
}

void ExitEn(int errnum, const char * format, ...)
{
    va_list argList;
    va_start(argList, format);
    outputError(true, errno, true, format, argList);
    va_end(argList);
    terminate(true);
}

void fatal(const char * format, ...)
{
    va_list argList;
    va_start(argList, format);
    outputError(false, errno, false, format, argList);
    va_end(argList);
    terminate(true);
}

void usageErr(const char * format, ...)
{
    va_list argList;
    fflush(stdout);
    fprintf(stderr, "Usage: ");
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    va_end(argList);
    fflush(stderr);
    exit(EXIT_FAILURE);
}

void cmdLineErr(const char * format, ...)
{
    va_list argList;
    fflush(stdout);
    fprintf(stderr, "Command line usage error: ");
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    va_end(argList);
    fflush(stderr);
    exit(EXIT_FAILURE);
}

