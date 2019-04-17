/*******
  AOS/VS ?FSTAT Packet Record

  Based on info from System Call Dictionary 093-000241 p.2-162

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

/* on-disk/tape size of FSTAT packet is 50 bytes */
#define SLTH_W 25
#define SLTH_B 50

#define FLNK 0
#define FDIR 12
#define FDMP 64 /* guessed symbol */
#define FSTF 67
#define FTXT 68
#define FPRV 74
#define FPRG 87


typedef struct {
  BYTE record_format; /* ?STYP */
  BYTE entry_type;
  WORD stim, sacp, shfs, slau, smsh, smsl, smil, stch, stcl, stah, stal, stmh, stml, ssts,
    sefw, sefl, sfah, sfal, sidx, sopn, scsh, scsl;
} FSTAT_PKT;


