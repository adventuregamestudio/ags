include(CheckCSourceCompiles)

check_c_source_compiles("
    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    int main(void) {
        FILE *stream;
        off_t offset;
        int whence;
        int res = fseeko(stream, offset, whence);
        offset = ftello(stream);
        return 0;
    }"
    HAVE_FSEEKO
)
