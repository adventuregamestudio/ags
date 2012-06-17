#ifndef __AC_FILE_H
#define __AC_FILE_H

void get_current_dir_path(char* buffer, const char *fileName);
bool validate_user_file_path(const char *fnmm, char *output, bool currentDirOnly);

extern char* game_file_name;

#endif // __AC_FILE_H