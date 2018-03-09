#include "CutterApplication.h"
#include "MainWindow.h"

#ifdef APPIMAGE
#define PREFIX "/tmp/.cutter_usr"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

void set_appimage_symlink()
{
    char *path = realpath("/proc/self/exe", NULL);
    char *i = strrchr(path, '/');
    *(i + 1) = '\0';
    char *dest = strcat(path, "../");
    struct stat buf;
    if (lstat(PREFIX, &buf) == 0 && S_ISLNK(buf.st_mode))
    {
        remove(PREFIX);
    }
    symlink(dest, PREFIX);
    printf("'%s' '%s' '%s'\n", path, i, dest);
    free(path);
}
#endif

int main(int argc, char *argv[])
{
    // Hack to make it work with AppImage
#ifdef APPIMAGE
    set_appimage_symlink();
#endif

    CutterApplication a(argc, argv);

    int ret = a.exec();

#ifdef APPIMAGE
    remove(PREFIX);
#endif

    return ret;
}
