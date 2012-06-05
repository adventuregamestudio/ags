#ifndef PE_H
#define PE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  char version[15];
  char description[100];
  char internal_name[100];
} version_info_t;

int getVersionInformation(char* filename, version_info_t* version_info);

#ifdef __cplusplus
}
#endif

#endif