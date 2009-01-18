#ifndef __PKG_UTIL_H__
#define __PKG_UTIL_H__

#include <stdio.h>

#define LINK_OR_COPY_SUCCESS 0
#define LINK_OR_COPY_OUT_OF_DISK -1
#define LINK_OR_COPY_ERROR -2

char * copy_string( const char * );
void dbg_printf( char const *, int, char const *, ... );
char * get_current_dir( void );
char * get_temp_dir( void );
char * get_path_component( char *, char ** );
char * hash_to_string( unsigned char *, unsigned long );
int is_whitespace( char * );
int link_or_copy( const char *, const char * );
int parse_strings_from_line( char *, char *** );
char * read_line_from_file( FILE * );
int recrm( const char * );
char * rename_to_temp( const char * );
int strlistlen( char ** );

#endif
