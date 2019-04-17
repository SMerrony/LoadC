/*******
  AOS/VS Dump Format structures

  Based on info from AOS/VS Systems Internals Reference Manual (AOS/VS Rev. 5.00)

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
******/

#include "aosvs_types.h"
#include "aosvs_fstat_packet.h"

#define START_DUMP_TYPE 0
#define FSB_TYPE 1
#define NB_TYPE 2
#define UDA_TYPE 3
#define ACL_TYPE 4
#define LINK_TYPE 5
#define START_BLOCK_TYPE 6
#define DATA_BLOCK_TYPE 7
#define END_BLOCK_TYPE 8
#define END_DUMP_TYPE 9

#define MAX_BLOCK_SIZE 32768
#define MAX_ALIGNMENT_OFFSET 256


typedef /* union {
	   WORD header; */
  struct {
    unsigned int record_type;
    unsigned int record_length;
    /*  } bits; */
} RECORD_HEADER;

/* 0 - Start of dump */
typedef struct {
  RECORD_HEADER sod_header;  /* 0/14 */
  WORD dump_format_revision; /* 15/16 */
  WORD dump_time_secs;
  WORD dump_time_mins;
  WORD dump_time_hours;
  WORD dump_date_day;
  WORD dump_date_month;
  WORD dump_date_year;
} START_OF_DUMP;

/* 1 - file status block */
typedef struct {
  RECORD_HEADER fsb_header;
  FSTAT_PKT fstat;
} FILE_STATUS_BLOCK;

/* 2 - Name block */
typedef struct {
  RECORD_HEADER nb_header;
  char file_name[];
} NAME_BLOCK;

/* 3 - Uda block */
typedef struct {
  RECORD_HEADER uda_header;
  char uda[256];
} UDA_BLOCK;


/* 4 - ACL block */
typedef struct {
  RECORD_HEADER acl_header;
  char acl[];
} ACL_BLOCK;

/* 5 - Link block */
typedef struct {
  RECORD_HEADER link_header;
  char link_resolution_name[];
} LINK_BLOCK;

/* 6 - Start block */
typedef struct {
  RECORD_HEADER start_block_header;
} START_BLOCK;

/* 7 - Data header block */
typedef struct {
  RECORD_HEADER data_header;
  DWORD byte_address;
  DWORD byte_length;
  WORD  alignment_count;
} DATA_HEADER_BLOCK;

/* 8 - End block */
typedef struct {
  RECORD_HEADER end_block_header;
} END_BLOCK;

/* 9 - End of dump */
typedef struct {
  RECORD_HEADER end_dump_header;
} END_OF_DUMP;
