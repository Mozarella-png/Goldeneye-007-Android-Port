/* errno access for the port, kept out of romdata.c on purpose.
 * include/PR/os.h declares a struct field named `errno`, which collides with
 * the errno macro when both headers are in one translation unit. */
#include <errno.h>
#include <string.h>

int portLastErrno(void)
{
    return errno;
}

const char *portLastErrnoText(void)
{
    return strerror(errno);
}
