/**********

  AOS/VS Dump File Loader (adfl)

  This file is part of adfl.

  adfl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  adfl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with adfl.  If not, see <http://www.gnu.org/licenses/>.

  Copyright 2013 Steve Merrony

********/

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <getopt.h>

#include "aosvs_dump_fmt.h"

const char *dump_file_name = NULL;
const char *HERE = "./";
const int VERSION_MAJOR = 1;
const int VERSION_MINOR = 0;


/* Rather a lot of globals - so we can split up the switch statements later more easily */
FILE *dfp, *loadedfp;
int extract_flag = false, ignore_errors_flag = false, summarise_flag = false, verbose_flag = false;
int accum_total_file_size = false;
RECORD_HEADER rec_hdr;
char fsb_blob[sizeof(FILE_STATUS_BLOCK)], alignment_blob[MAX_ALIGNMENT_OFFSET], data_blob[MAX_BLOCK_SIZE];
FILE_STATUS_BLOCK fsb;
DATA_HEADER_BLOCK dhb;
char file_type[20], file_name[128], acl[256], link_target[256], directory[256], load_path[512];
int total_file_byte_size = 0;
int in_file_blocks = false, load_it = false;

void show_usage() {
  printf( "Usage: adfl [-hdlsvVx] <dumpfile>\n"
         "Extract file &/or show contents of an AOS/VS (II) DUMP file on a GNU or Windows system.\n"
         "The current version (%d.%d) can read (at least) versions 15 & 16 of the DUMP format.\n"
         "Arguments:\n"
         "\t-h --help          show this help\n"
         "\t-i --ignore_errors do not exit if a file cannot be created\n"
         "\t-l --list          list the contents of the dump file\n"
         "\t-s --summary       concise list of the dump file contents\n"
         "\t-v --verbose       be rather wordy about what adfl is doing\n"
         "\t-V --version       display the version number of adfl\n"
         "\t-x --extract       extract the files from the dump in the current directory\n"
         , VERSION_MAJOR, VERSION_MINOR
         );
}

/* Byte swap unsigned short */
uint16_t swap_uint16( uint16_t val ) {
    return (val << 8) | (val >> 8 );
}

uint32_t swap_uint32( uint32_t x ) {
  uint32_t y;
  ( ( uint8_t* )( &y ) )[0] = ( ( uint8_t* )( &x ) )[3];
  ( ( uint8_t* )( &y ) )[1] = ( ( uint8_t* )( &x ) )[2];
  ( ( uint8_t* )( &y ) )[2] = ( ( uint8_t* )( &x ) )[1];
  ( ( uint8_t* )( &y ) )[3] = ( ( uint8_t* )( &x ) )[0];
  return y;
}

WORD read_a_word( FILE * fp ) {

  WORD w;
  unsigned char byte_a, byte_b;

  fread( &byte_a, 1, 1, fp );
  fread( &byte_b, 1, 1, fp );

  w = byte_a <<8 | byte_b;

  return w;
}

RECORD_HEADER read_header( FILE * fp ) {

  RECORD_HEADER rh;

  unsigned char byte_a, byte_b;

  fread( &byte_a, 1, 1, fp );
  fread( &byte_b, 1, 1, fp );

  rh.record_type = byte_a >> 2;  /*  6 bits */
  rh.record_length = ((byte_a & 0x03) << 8) | byte_b;

  return rh;
}

START_OF_DUMP read_sod( FILE * fp ) {

  START_OF_DUMP sod = { {0,0}, 0, 0, 0, 0, 0, 0, 0 };

  sod.sod_header = read_header( fp );
  if (sod.sod_header.record_type != START_DUMP_TYPE) {
    fprintf( stderr, "Error: This does not appear to be an AOS/VS dump file.\n" );
    exit( 1 );
  }
  sod.dump_format_revision = read_a_word( fp );
  sod.dump_time_secs = read_a_word( fp );
  sod.dump_time_mins = read_a_word( fp );
  sod.dump_time_hours = read_a_word( fp );
  sod.dump_date_day = read_a_word( fp );
  sod.dump_date_month = read_a_word( fp );
  sod.dump_date_year = read_a_word( fp );

  return sod;
}

char *chngChar ( char *str, char oldChar, char newChar ) {
  char *strPtr = str;
  while ( ( strPtr = strchr ( strPtr, oldChar ) ) != NULL ) *strPtr++ = newChar;
  return str;
}

void process_dir() {
  strcpy( file_type, "Directory" );
  if ( strlen( directory ) > 0 ) strcat( directory, "/" );
  strcat( directory, file_name );
  file_name[0] = '\0';
  if ( extract_flag ) {
    /* make the directory */
    char tmp_dname[512];
    strcpy( tmp_dname, HERE );
    strcat( tmp_dname, directory );
    if ( mkdir( tmp_dname, 0777 ) != 0 ) {
      fprintf( stderr, "Error: Could not create directory %s%s\n", HERE, directory );
      if ( ignore_errors_flag ) {

      } else {
        fprintf( stderr, "Giving up.\n" );
        exit( 1 );
      }
    }
  }
  load_it = false;
}

void process_name_block() {

  int c;

  fread( file_name, rec_hdr.record_length, 1, dfp );
  for ( c = 0; c < strlen( file_name ); c++ ) file_name[c] = toupper( file_name[c] );
#ifdef _WIN32
  /* check for ? in filename and substitute with 'q' */
  if ( strchr( file_name, '?' ) != NULL ) {
    printf( "Warning: Filename changed from %s in dump ", file_name );
    char * tmp_fn = chngChar( file_name, '?', 'q' );
    printf( "to %s on disk\n", tmp_fn );
    strcpy( file_name, tmp_fn );
  }
#endif
  if ( summarise_flag && verbose_flag ) {
    printf( "\n" );
  }
  switch( fsb_blob[1] ) {
  case FLNK:
    strcpy( file_type, "Link" );
    load_it = false;
    break;
  case FDIR:
    process_dir();
    break;
    /* case FDMP: strcpy( file_type, "Dump file" ); break; */
  case FSTF:
    strcpy( file_type, "Symbol table" );
    load_it = true;
    break;
  case FTXT:
    strcpy( file_type, "Text file" );
    load_it = true;
    break;
  case FPRG:  /* deliberate fall-through */
  case FPRV:
    strcpy( file_type, "Program file" );
    load_it = true;
    break;
  default:  /* we don't explicitly recognise the file type */ /* TODO: get definitive list from paru.32.sr */
    strcpy( file_type, "File" );
    load_it = true;
  }

  if ( summarise_flag ) {
    if ( strlen( directory ) == 0 ) {
      printf( "%-12s: %s%s", file_type, HERE, file_name );
    } else {
      printf( "%-12s: %s%s/%s", file_type, HERE, directory, file_name );
    }
    if ( verbose_flag || fsb_blob[1]==FDIR ) {
      printf( "\n" );
    } else {
      printf( "\t" );
    }
  }

  /* create/open the file */
  if ( extract_flag && load_it ) {
    if ( strlen( directory ) == 0 ) { /* at top */
      sprintf( load_path, "%s%s", HERE, file_name );
    } else {
      sprintf( load_path, "%s%s/%s", HERE, directory, file_name );
    }
    loadedfp = fopen( load_path, "w" );
    if ( loadedfp == NULL ) {
      fprintf( stderr, "Error: Could not create file\nFilename: %s\nReason: %s\n", load_path, strerror( errno ) );
      if ( ignore_errors_flag ) {

      } else {
        fprintf( stderr, "Giving up.\n" );
        exit( 1 );
      }
    }
  }
}

void process_link() {

  int c;
  char cwd[1024];

  fread( link_target, rec_hdr.record_length, 1, dfp );
  link_target[rec_hdr.record_length] = 0;
  for ( c = 0; c < strlen( link_target ); c++ ) link_target[c] = toupper( link_target[c] );
  /* convert AOS/VS : directory separators to Unix slashes */
  if ( strchr( link_target, ':' ) != NULL ) {
    char * tmp_ln = chngChar( link_target, ':', '/' );
    strcpy( link_target, tmp_ln );
  }
  if ( summarise_flag ) {
    printf( " -> Link Target: %s\n", link_target );
  }
  if ( extract_flag ) {
#if defined(__MINGW32__)
    printf( "Warning: Cannot create links on mingw32 platform - skipping\n" );
#else
    if (strlen( directory) > 0 ) {
      /* preserve cwd and move to the desired dir */
      getcwd( cwd, 1024 );
      chdir( directory );
    }
    if ( symlink( link_target, file_name ) != 0 ) {
      printf( "Warning: Could not create symbolic link %s to %s - continuing anyway...\n", link_target, file_name );
    }
    if (strlen( directory) > 0 ) {
        chdir( cwd );
    }
#endif
  }
}

void process_data_block() {
  /* for the summary - we only care about the length... */
  fread( &dhb.byte_address, 4, 1, dfp );
  fread( &dhb.byte_length, 4, 1, dfp );
  dhb.byte_length = swap_uint32( dhb.byte_length );
  fread( &dhb.alignment_count, 2, 1, dfp );
  dhb.alignment_count = swap_uint16( dhb.alignment_count );
  if ( summarise_flag && verbose_flag ) {
    printf( " Data block: %i (bytes)\n", dhb.byte_length );
  }
  if ( extract_flag && loadedfp != NULL ) {
    if ( dhb.alignment_count > 0 ) fread( alignment_blob, dhb.alignment_count, 1, dfp );
    fread( data_blob, dhb.byte_length, 1, dfp );
    fwrite( data_blob, dhb.byte_length, 1, loadedfp );
  } else {
    /* skip the actual data */
    fseek( dfp, dhb.alignment_count + dhb.byte_length, SEEK_CUR );
  }
  total_file_byte_size += dhb.byte_length;
  accum_total_file_size = true;
  in_file_blocks = true;
}

void process_end_block() {
  if ( in_file_blocks ) {
    if ( extract_flag && load_it ) {
      fclose( loadedfp );
    }
    if ( accum_total_file_size && summarise_flag ) {
      printf( " %i bytes\n", total_file_byte_size );
    }
    total_file_byte_size = 0;
    accum_total_file_size = false;
    in_file_blocks = false;
    if ( verbose_flag ) {
      printf( " Closed file\n" );
    }
  } else {
    /* not in the middle of file, this is a directory pop instruction */
    if ( strlen( directory ) > 0 ) {
      char *temp = &directory[strlen( directory )];
      while ( temp > directory && *temp!= '/' ) temp--;
      *temp = '\0';
    }
    if ( verbose_flag ) {
      printf( "Popped dir - new dir is: %s/%s\n", HERE, directory );
    }
  }
}

void process_dumpfile() {

  dfp = fopen( dump_file_name, "rb" );
  if (dfp == NULL) {
    fprintf( stderr, "Error: Unable to open file %s\n", dump_file_name );
    exit( 1 );
  }

  START_OF_DUMP sod;

  /* there should always be a SOD record... */
  sod = read_sod( dfp );
  if (summarise_flag) {
    printf( "AOS/VS Dump version : %d\n", sod.dump_format_revision );
    printf( "Dump date (y-m-d)   : %d-%d-%d\n", sod.dump_date_year, sod.dump_date_month, sod.dump_date_day );
    printf( "Dump time (hh:mm:ss): %02d:%02d:%02d\n", sod.dump_time_hours, sod.dump_time_mins, sod.dump_time_secs );
  }

  /* now go through the dump examining each block type and acting accordingly... */
  int more = true;

  while (more) {

    rec_hdr = read_header( dfp );

    switch( rec_hdr.record_type ) {
    case FSB_TYPE:
      /* ignore for now - I think we will only need it if we plan to handle 'sparse' file dumps */
      fread( fsb_blob, rec_hdr.record_length, 1, dfp );
      /* printf( "FSB type %i/%i\t", (int) fsb_blob[0], (int) fsb_blob[1] ); */
      load_it = false;
      break;
    case NB_TYPE: /* Name Block */
      process_name_block();
      break;
    case UDA_TYPE:
      /* ignore for now */
      /* printf( "Ignoring UDA block\n" ); */
      fread( &fsb, rec_hdr.record_length, 1, dfp );
      break;
    case ACL_TYPE:
      fread( acl, rec_hdr.record_length, 1, dfp );
      acl[rec_hdr.record_length] = 0;
      if (verbose_flag) { printf( " ACL: %s\n", acl ); }
      break;
    case LINK_TYPE:
      process_link();
      break;
    case START_BLOCK_TYPE:
      /* nothing to do - it's just a record_header */
      break;
    case DATA_BLOCK_TYPE:
      process_data_block();
      break;
    case END_BLOCK_TYPE:
      process_end_block();
      break;
    case END_DUMP_TYPE:
      printf( "=== End of Dump ===\n" );
      more = false;
      break;
    default:
      fprintf( stderr, "Error: Unknown block type (%d) in dump file.  Giving up.\n", rec_hdr.record_type );
      more = false;
    }
  }
  fclose( dfp );
}

int main( int argc, char *argv[] ) {

  if (argc == 1) {
    show_usage();
    exit( 1 );
  }

  static struct option long_options[] = {
    { "help",          no_argument,       0, 'h' },
    { "ignore_errors", no_argument,       0, 'i' },
    { "list",          no_argument,       0, 'l' },
    { "summary",       no_argument,       0, 's' },
    { "verbose",       no_argument,       0, 'v' },
    { "version",       no_argument,       0, 'V' },
    { "extract",       no_argument,       0, 'x' },
    { 0, 0, 0, 0 }
  };

  int opt = 0, option_index = 0;

  do {
    opt = getopt_long( argc, argv, "hilsvVx", long_options, &option_index );
    switch( opt ) {
    case 'h':
      show_usage();
      exit( 0 );
      break;
    case 'i':
        ignore_errors_flag = true;
        break;
    case 'l': /* deliberate fall-through */
    case 's':
      summarise_flag = true;
      break;
    case 'v':
      verbose_flag = true;
      break;
    case 'V':
      printf( "adfl version %d.%d\n", VERSION_MAJOR, VERSION_MINOR );
      exit( 0 );
      break;
    case 'x':
      extract_flag = true;
      break;
    case -1:
      /* options exhausted */
      break;
    default:
      exit( 1 );
    }
  } while (opt != -1);

  if ( optind >= argc ) { /* no input file specified */
    show_usage();
    exit( 1 );
  }

  dump_file_name = argv[optind];
  if (dump_file_name == NULL) {
    show_usage();
    exit( 1 );
  }

  if (summarise_flag || extract_flag) {
    process_dumpfile();
  }

  return 0;
}

