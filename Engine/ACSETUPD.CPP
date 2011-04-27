#include <process.h>
#include <dir.h>
#include <string.h>

void main() {
  char buffer[100];
  getcwd(buffer, 99);
  strcat(buffer,"\\acdos.exe");
  execl(buffer,buffer,"--setup",NULL);
}
