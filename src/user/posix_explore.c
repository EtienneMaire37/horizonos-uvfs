#include "../vnode.h"
#include "../log.h"
#include <limits.h>
#include <string.h>
#include <sys/dir.h>

int posix_explore(vnode_ref_t ref)
{
  char path[PATH_MAX];
  char current_path[PATH_MAX];
  size_t len = vfs_get_absolute_path_to_node(ref, path, sizeof(path));
  memcpy(current_path, path, PATH_MAX);

  LOG(TRACE, "vfs_explore: Exploring path \"%s\"", path);
  DIR *dir = opendir(path);
  if (dir) {
    struct dirent *ent;
    while ((ent = readdir(dir))) {
      if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
        sprintf(&current_path[len], "%s", ent->d_name);
        current_path[len - 1] = '/';
        struct stat st;
        if (lstat(current_path, &st) == 0)
          vfs_add_new_child_node(ref, ent->d_name, &st);
        else {
          char buf[PATH_MAX + 17];
          snprintf(buf, sizeof(buf) - 1, "Couldn't stat \"%s\"", current_path);
          perror(buf);
        }
      }
    }
    closedir(dir);
  }
  return 0;
}
