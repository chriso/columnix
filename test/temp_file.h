#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *cx_temp_file_new()
{
    char tmp_path[] = "/tmp/cx_temp_file.XXXXXX";
    int fd = mkstemp(tmp_path);
    if (fd < 0)
        return NULL;
    close(fd);
    char *path = malloc(sizeof(tmp_path));
    if (!path)
        goto error;
    memcpy(path, tmp_path, sizeof(tmp_path));
    return path;
error:
    unlink(tmp_path);
    return NULL;
}

static void cx_temp_file_free(char *path)
{
    unlink(path);
    free(path);
}
