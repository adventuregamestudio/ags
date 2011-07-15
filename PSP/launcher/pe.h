#ifndef PE_H
#define PE_H

typedef struct {
  char version[10];
  char description[100];
  char internal_name[100];
} version_info_t;

int getVersionInformation(char* filename, version_info_t* version_info);

#endif