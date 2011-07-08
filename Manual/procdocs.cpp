#include <io.h>
#include <stdio.h>
#include <string.h>
#include <alloc.h>

void ProcessFile (const char * filname) {
  printf("Processing %s...\n", filname);
  char *data;
  FILE *inp = fopen(filname, "rb");
  long flen = filelength(fileno(inp));
  data = (char*)malloc(flen + 10);
  fread (data, flen, 1, inp);
  fclose(inp);
  data[flen] = 0;
  char *dataptr = data;

  
  char *datawas = data;
  // Replace 'ILBRK' with '<br>'
  while (1) {
    
    data = strstr (data, "ILBRK");
    if (data == NULL)
      break;
    
    strcpy(data, "<br>");
    data[4] = ' ';    
  }
  
  data = datawas;

  // Replace 'GTSS' with '>'
  while (1) {
    
    data = strstr (data, "GTSS");
    if (data == NULL)
      break;
    
    strncpy(data, ">   ", 4);
  }
  
    data = datawas;

  // Replace 'LTSS' with '<'
  while (1) {
    
    data = strstr (data, "LTSS");
    if (data == NULL)
      break;
    
    strncpy(data, "   <", 4);
  }
  
  inp = fopen(filname, "wb");

  data = strstr (datawas, "</title></head>");
  if (data == NULL)
    data = strstr (datawas, "</TITLE></HEAD>");

  if (data != NULL) {
    data+=8;
    fwrite (dataptr, (data - dataptr), 1, inp);
    flen -= (data - dataptr);
    fputs ("<style type=\"text/css\">\n"
      "<!--\n"
      "body         { font-family: Verdana; font-size: 10pt }\n"
      "td           { font-family: Verdana; font-size: 10pt }\n"
      "a            { font-weight: bold }\n"
      "-->\n</style>\n", inp);
  }
  else
    data = datawas;

  fwrite (data, flen, 1, inp);
  fclose (inp);

  free (dataptr);

}

void main() {

    struct _finddata_t c_file;
    long hFile;

    /* Find first .c file in current directory */
    if( (hFile = _findfirst( "*.htm", &c_file )) == -1L )
       printf( "No *.htm files in current directory!\n" );
    else {

     do {
       ProcessFile (c_file.name);
   
            /* Find the rest of the .c files */
      } while( _findnext( hFile, &c_file ) == 0 );
     _findclose( hFile );
   }

}
