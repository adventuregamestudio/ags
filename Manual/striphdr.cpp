#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdlib.h>

int main (int argc,char*argv[]) {
  FILE *inp = fopen(argv[1], "rb");
  FILE *oup = fopen(argv[2], "wb");
  if ((inp == NULL) || (oup == NULL)) {
    printf("Error opening file.");
    return 1;
  }
  long toread = filelength(fileno(inp));
  char *buf = (char*)malloc (toread+5);
  fread(buf, toread, 1, inp);
  buf[toread] = 0;
  fclose(inp);

  //buf = strstr (buf, "<td width=\"550\">") + 16;
  //char *end = strstr(buf, "<td valign=\"top\" width");
  buf = strstr(buf, "<td class=\"maintext\">") + 21;
  char *end = strstr(buf, "</td>\r\n</tr>\r\n</table></td>");
  if (end == NULL) {
  	printf("ERROR: End not found");
  	return 1;
  }
  end[0] = 0;
  fputs("<html><head>", oup);
  fputs("<style>\r\nbody         { font-family: Verdana; font-size: 10pt }\r\n"
   "a            { font-weight: bold }\r\n</style>\r\n</head><body>", oup);
  char *link = strstr (buf, "actutor2.htm");
  if (link != NULL) {
    link[0] = 0;
    fputs (buf, oup);
    link += 12;
    fwrite("ags30.htm",9,1,oup);
    fputs (link, oup);
  }
  else
    fputs(buf, oup);
  fputs("</body></html>", oup);
  fclose(oup);
  return 0;
}

