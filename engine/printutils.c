/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * This is GNU Go, a Go program. Contact gnugo@gnu.org, or see       *
 * http://www.gnu.org/software/gnugo/ for more information.          *
 *                                                                   *
 * Copyright 1999, 2000, 2001, 2002, 2003 and 2004                   *
 * by the Free Software Foundation.                                  *
 *                                                                   *
 * This program is free software; you can redistribute it and/or     *
 * modify it under the terms of the GNU General Public License as    *
 * published by the Free Software Foundation - version 2             *
 *                                                                   *
 * This program is distributed in the hope that it will be useful,   *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     *
 * GNU General Public License in file COPYING for more details.      *
 *                                                                   *
 * You should have received a copy of the GNU General Public         *
 * License along with this program; if not, write to the Free        *
 * Software Foundation, Inc., 59 Temple Place - Suite 330,           *
 * Boston, MA 02111, USA.                                            *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "board.h"
#include "gnugo.h"
#include "hash.h"
#include "gg_utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

/*
 * This function underpins all the TRACE and DEBUG stuff.
 * It accepts %c, %d, %f, %s, and %x as usual. But it
 * also accepts %m, which takes TWO integers and writes a move.
 * Other accepted formats are
 * %H: Print a hashvalue.
 * %C: Print a color as a string.
 * Nasty bodge: %o at the start means outdent, i.e. cancel indent.
 */

void 
vgprintf(FILE *outputfile, const char *fmt, va_list ap)
{
  if (fmt[0] == '%' && fmt[1] == 'o')
    fmt += 2;  /* cancel indent */
  else if (stackp > 0)
    fprintf(outputfile, "%.*s", stackp*2, "                                ");

  for (; *fmt; ++fmt) {
    if (*fmt == '%') {
      switch (*++fmt) {
      case 'c':
      {
	/* rules of promotion => passed as int, not char */
	int c = va_arg(ap, int);
	putc(c, outputfile);
	break;
      }
      case 'd':
      {
	int d = va_arg(ap, int);
	fprintf(outputfile, "%d", d);
	break;
      }
      case 'x':
      {
	unsigned int d = va_arg(ap, unsigned int);
	fprintf(outputfile, "%x", d);
	break;
      }
      case 'f':
      {
	double f = va_arg(ap, double); /* passed as double, not float */
	fprintf(outputfile, "%.2f", f);
	break;
      }
      case 'r':
      {
	int result = va_arg(ap, int);

	switch (result) {
	case 0:             
	  fprintf(outputfile, "%s", "0");
	  break;
	case KO_B:          
	  fprintf(outputfile, "%s", "KO_B");
	  break;
	case LOSS:          
	  fprintf(outputfile, "%s", "LOSS");
	  break;
	case GAIN:          
	  fprintf(outputfile, "%s", "GAIN");
	  break;
	case KO_A:          
	  fprintf(outputfile, "%s", "KO_A");
	  break;
	case WIN:           
	  fprintf(outputfile, "%s", "WIN");
	  break;
	  
	default:            fprintf(outputfile, "%s", "ERROR");
	}
	break;
      }
      case 's':
      {
	char *s = va_arg(ap, char *);
	fputs(s, outputfile);
	break;
      }
      case '2':
        fmt++;
        if (*fmt != 'm' && *fmt != 'M') {
	  fprintf(outputfile, "\n\nUnknown format string '2%c'\n", *fmt);
	  break;
        }
        /* else fall through - 2 modifier on %m is default. */
      case 'm':
      case 'M':
      {
	char movename[4];
	int m = va_arg(ap, int);
	int n = va_arg(ap, int);
	if (m == -1 && n == -1)
	  fputs("PASS", outputfile);
	else if (!ON_BOARD2(m, n))
	  fprintf(outputfile, "[%d,%d]", m, n);
	else {
	  /* Generate the move name. */
	  if (n < 8)
	    movename[0] = n + 65;
	  else
	    movename[0] = n + 66;
	  if (*fmt == 'm')
	    sprintf(movename+1, "%d", board_size - m);
	  else
	    sprintf(movename+1, "%-2d", board_size - m);
	  fputs(movename, outputfile);
	}
	break;
      }
      case '1':
      fmt++;
      if (*fmt != 'm' && *fmt != 'M') {
	fprintf(outputfile, "\n\nUnknown format string '1%c'\n", *fmt);
	break;
      }
      else {
	char movename[4];
	int pos = va_arg(ap, int);
	int m = I(pos);
	int n = J(pos);
	if (pos == NO_MOVE)
	  fputs("PASS", outputfile);
	else if (!ON_BOARD1(pos))
	  fprintf(outputfile, "[%d]", pos);
	else {
	  /* Generate the move name. */
	  if (n < 8)
	    movename[0] = n + 65;
	  else
	    movename[0] = n + 66;
	  if (*fmt == 'm')
	    sprintf(movename + 1, "%d", board_size - m);
	  else
	    sprintf(movename + 1, "%-2d", board_size - m);
	  fputs(movename, outputfile);
	}
	break;
      }
      case 'H':
      {
	unsigned long h = va_arg(ap, unsigned long);
	fprintf(outputfile, "%lx", h);
	break;
      }
      case 'C':
      {
	int color = va_arg(ap, int);
	fputs(color_to_string(color), outputfile);
	break;
      }
      default:
	fprintf(outputfile, "\n\nUnknown format character '%c'\n", *fmt);
	break;
      }
    }
    else
      putc(*fmt, outputfile);
  }
}


/*
 * required wrapper around vgprintf, writes to outfile.
 */

void 
gfprintf(FILE *outfile, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vgprintf(outfile, fmt, ap);
  va_end(ap);
}


/*
 * required wrapper around vgprintf, writes to stderr.
 * Always returns 1 to allow use in short-circuit logical expressions.
 */

int 
gprintf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vgprintf(stderr, fmt, ap);
  va_end(ap);
  return 1;
}


/*
 * required wrapper around vgprintf, in contrast to gprintf this one
 * writes to stdout.
 */

void
mprintf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vgprintf(stdout, fmt, ap);
  va_end(ap);
}


/*
 * A wrapper around abort() which shows the state variables at the time
 * of the problem. (pos) is typically a related move, or NO_MOVE.
 */

void 
abortgo(const char *file, int line, const char *msg, int pos)
{
  gprintf("%o\n\n***assertion failure:\n%s:%d - %s near %1m***\n\n",
	  file, line, msg, pos);
  dump_stack();

  /* Dump the stack as board images. */
  simple_showboard(stderr);
  while (stackp > 0) {
    popgo();
    simple_showboard(stderr);
  }

#if 0
  if (sgf_root) {
    sgf_write_header(sgf_root, 1, get_random_seed()
		     komi, level, chinese_rules);
    writesgf(sgf_root, "abortgo.sgf");
  }
#endif

  fprintf(stderr, "\ngnugo %s (seed %d): You stepped on a bug.\n",
          gg_version(), get_random_seed());
  if (board_size >= 9 && board_size <= 19) {
    fprintf(stderr, "\
Please save this game as an sgf file and mail it to gnugo@gnu.org\n\
If you can, please also include the debug output above this message.\n");
  }
  fprintf(stderr, "\n");

  fflush(stderr);
  fflush(stdout);

  abort();  /* cause core dump */
}

static const char *color_names[] = {
  COLOR_NAMES
};

/* Convert a color value to a string. */
const char *
color_to_string(int color)
{
  gg_assert(color < NUM_KOMASTER_STATES);
  return color_names[color];
}

/* Convert a location to a string. */
const char *
location_to_string(int pos)
{
  static int init = 0;
  static char buf[BOARDSIZE][5];
  if (!init) {
    int pos;
    for (pos = 0; pos < BOARDSIZE; pos++)
      location_to_buffer(pos, buf[pos]);
    init = 1;
  }
  ASSERT1(pos >= 0 && pos < BOARDSIZE, pos);
  return buf[pos];
}

/* Convert a location to a string, writing to a buffer. */

void
location_to_buffer(int pos, char *buf)
{
  char *bufp = buf;
  int i = I(pos);
  int j = J(pos);

  if (pos == NO_MOVE) {
    strcpy(buf, "Pass");
    return;
  }

  *bufp = 'A'+j;
  if (*bufp >= 'I')
    (*bufp)++;
  bufp++;

  i = board_size - i;
  if (i > 9)
    *bufp++ = '0' + i/10;
  *bufp++ = '0' + i%10;

  *bufp = 0;
}


/*
 * Get the (m, n) coordinates in the standard GNU Go coordinate system
 * from the string STR.  This means that m is the nth row from the top
 * and n is the column. Both coordinates are between 0 and boardsize-1,
 * inclusive.
 *
 * Return 1 if ok, otherwise return 0;
 */

int
string_to_location(int boardsize, char *str, int *m, int *n)
{
  if (*str == '\0')
    return 0;

  if (!isalpha((int) *str))
    return 0;
  *n = tolower((int) *str) - 'a';
  if (tolower((int) *str) >= 'i')
    --*n;
  if (*n < 0 || *n > boardsize - 1)
    return 0;

  if (!isdigit((int) *(str+1)))
    return 0;
  *m = boardsize - atoi(str + 1);
  if (*m < 0 || *m > boardsize - 1)
    return 0;

  return 1;
}


/* Some simple functions to draw an ASCII board. */

/* True if the coordinate is a hoshi point. */
int
is_hoshi_point(int m, int n)
{
  int hoshi;
  int middle;

  /* No hoshi points on these boards. */
  if (board_size == 2 || board_size == 4)
    return 0;

  /* In the middle of a 3x3 board. */
  if (board_size == 3) {
    if (m == 1 && n == 1)
      return 1;

    return 0;
  }

  if (board_size == 5) {
    if (m == 1 && (n == 1 || n == 3))
      return 1;
    if (m == 2 && n == 2)
      return 1;
    if (m == 3 && (n == 1 || n == 3))
      return 1;

    return 0;
  }

  /* 3-3 points are hoshi on sizes 7--11, 4-4 on larger. */
  if (board_size <= 11)
    hoshi = 2;
  else
    hoshi = 3;

  /* Coordinate for midpoint. */
  middle = board_size/2;
    
  /* Normalize the coordinates by mirroring to the lower numbers. */
  if (m >= middle)
    m = board_size - 1 - m;
  if (n >= middle)
    n = board_size - 1 - n;
  
  /* Is this a corner hoshi? */
  if (m == hoshi && n == hoshi)
    return 1;

  /* If even sized board, only hoshi points in the corner. */
  if (board_size%2 == 0)
    return 0;

  /* Less than 12 in board size only middle point. */
  if (board_size < 12) {
    if (m == middle && n == middle)
      return 1;

    return 0;
  }

  /* Is this a midpoint hoshi? */
  if ((m == hoshi || m == middle)
      && (n == hoshi || n == middle))
    return 1;

  /* No more chances. */
  return 0;
}


/* Print a line with coordinate letters above the board. */
void
draw_letter_coordinates(FILE *outfile)
{
  int i;
  int ch;
  
  fprintf(outfile, "  ");
  for (i = 0, ch = 'A'; i < board_size; i++, ch++) {
    if (ch == 'I')
      ch++;
    fprintf(outfile, " %c", ch);
  }
}


/* Bare bones version of showboard(0). No fancy options, no hint of
 * color, and you can choose where to write it.
 */
void
simple_showboard(FILE *outfile)
{
  int i, j;

  draw_letter_coordinates(outfile);
  
  for (i = 0; i < board_size; i++) {
    fprintf(outfile, "\n%2d", board_size - i);
    
    for (j = 0; j < board_size; j++) {
      if (BOARD(i, j) == EMPTY)
	fprintf(outfile, " %c", is_hoshi_point(i, j) ? '+' : '.');
      else
	fprintf(outfile, " %c", BOARD(i, j) == BLACK ? 'X' : 'O');
    }

    fprintf(outfile, " %d", board_size - i);
    
    if ((board_size < 10 && i == board_size-2)
	|| (board_size >= 10 && i == 8))
      fprintf(outfile, "     WHITE (O) has captured %d stones", black_captured);
    
    if ((board_size < 10 && i == board_size-1)
	|| (board_size >= 10 && i == 9))
      fprintf(outfile, "     BLACK (X) has captured %d stones", white_captured);
  }
  
  fprintf(outfile, "\n");
  draw_letter_coordinates(outfile);
}



/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
