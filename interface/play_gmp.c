/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * This is GNU GO, a Go program. Contact gnugo@gnu.org, or see       *
 * http://www.gnu.org/software/gnugo/ for more information.          *
 *                                                                   *
 * Copyright 1999, 2000, 2001, 2002 and 2003                         *
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

#include "gnugo.h"
#include "liberty.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "interface.h"
#include "gmp.h"
#include "sgftree.h"
#include "gg_utils.h"

/* --------------------------------------------------------------*/
/* Play a game against a go-modem-protocol (GMP) client.         */
/* --------------------------------------------------------------*/
void
play_gmp(Gameinfo *gameinfo)
{
  SGFTree sgftree;

  Gmp *ge;
  GmpResult message;
  const char *error;
  
  int i, j;
  int moveval;
  int passes = 0; /* two passes and its over */
  int to_move;  /* who's turn is next ? */

  int mycolor;  /* who has which color */
  int yourcolor;
  
  mycolor = gameinfo->computer_player;

  sgftree_clear(&sgftree);
  sgftreeCreateHeaderNode(&sgftree, gnugo_get_boardsize(), gnugo_get_komi());

  yourcolor = OTHER_COLOR(mycolor);

  ge = gmp_create(0, 1);
  TRACE("board size=%d\n", gnugo_get_boardsize());

  /* Leave all the -1's so the client can negotiate the game parameters. */
  if (chinese_rules)
    gmp_startGame(ge, -1, -1, 5.5, -1, -1);
  else
    gmp_startGame(ge, -1, -1, 5.5, 0, -1);
  
  do {
    message = gmp_check(ge, 1, NULL, NULL, &error);
  } while (message == gmp_nothing || message == gmp_reset);
  
  if (message == gmp_err)  {
    fprintf(stderr, "gnugo-gmp: Error \"%s\" occurred.\n", error);
    exit(EXIT_FAILURE);
  }
  else if (message != gmp_newGame)  {
    fprintf(stderr, "gnugo-gmp: Expecting a newGame, got %s\n",
	    gmp_resultString(message));
    exit(EXIT_FAILURE);
  }

  gameinfo->handicap = gmp_handicap(ge);
  gnugo_clear_board(gmp_size(ge));

#if ORACLE
  if (metamachine && oracle_exists)
    oracle_clear_board(gnugo_get_boardsize());
#endif

  /* 
   * The specification of the go modem protocol doesn't even discuss
   * komi. So we have to guess the komi. If the komi is set on the
   * command line, keep it. Otherwise, its value will be 0.0 and we
   * use 5.5 in an even game, 0.5 otherwise.
   */
  if (gnugo_get_komi() == 0.0) {
    if (gameinfo->handicap == 0)
      gnugo_set_komi(5.5);
    else
      gnugo_set_komi(0.5);
  }

  sgfOverwritePropertyInt(sgftree.root, "SZ", gnugo_get_boardsize());

  TRACE("size=%d, handicap=%d, komi=%f\n", gnugo_get_boardsize(),
	gameinfo->handicap, gnugo_get_komi());

  if (gameinfo->handicap)
    to_move = WHITE;
  else
    to_move = BLACK;

  if (gmp_iAmWhite(ge)) {
    mycolor = WHITE;     /* computer white */
    yourcolor = BLACK;   /* human black */
  }
  else {
    mycolor = BLACK;
    yourcolor = WHITE;
  }

  gameinfo->computer_player = mycolor;
  sgf_write_header(sgftree.root, 1, get_random_seed(), gnugo_get_komi(),
		   level, chinese_rules);
  gameinfo->handicap = gnugo_sethand(gameinfo->handicap, sgftree.root);
  sgfOverwritePropertyInt(sgftree.root, "HA", gameinfo->handicap);

  /* main GMP loop */
  while (passes < 2) {

    if (to_move == yourcolor) {
      moveval = 0;
      /* Get opponent's move from gmp client. */
      message = gmp_check(ge, 1, &j, &i, &error);

      if (message == gmp_err) {
	fprintf(stderr, "GNU Go: Sorry, error from gmp client\n");
        sgftreeAddComment(&sgftree, "got error from gmp client");
        sgffile_output(&sgftree);
	return;
      }

      if (message == gmp_undo) {
	int k;
	assert(j > 0);
	
	for (k = 0; k < j; k++) {
	  if (!gnugo_undo_move(1)) {
	    fprintf(stderr, "GNU Go: play_gmp UNDO: can't undo %d moves\n",
		    j - k);
	    break;
	  }
	  sgftreeAddComment(&sgftree, "undone");
	  sgftreeBack(&sgftree);
	  to_move = OTHER_COLOR(to_move);
	}
	continue;
      }

      if (message == gmp_pass) {
	++passes;
        sgftreeAddPlay(&sgftree, to_move, -1, -1);
	gnugo_play_move(-1, -1, yourcolor);
	sgffile_output(&sgftree);
      }
      else {
	/* not pass */
	passes = 0;
        sgftreeAddPlay(&sgftree, to_move, i, j);
	TRACE("\nyour move: %m\n\n", i, j);
	gnugo_play_move(i, j, yourcolor);
	sgffile_output(&sgftree);
      }

    }
    else {
      /* Generate my next move. */
      moveval = gnugo_genmove(&i, &j, mycolor);
      gnugo_play_move(i, j, mycolor);
      sgffile_add_debuginfo(sgftree.lastnode, moveval);
      
      if (moveval < 0) {
	/* pass */
        sgftreeAddPlay(&sgftree, to_move, -1, -1);
	gmp_sendPass(ge);
	++passes;
      }
      else {
	/* not pass */
        sgftreeAddPlay(&sgftree, to_move, i, j);
	gmp_sendMove(ge, j, i);
	passes = 0;
	TRACE("\nmy move: %m\n\n", i, j);
      }
      sgffile_add_debuginfo(sgftree.lastnode, 0);
      sgffile_output(&sgftree);
    }
    
    to_move = OTHER_COLOR(to_move);
  }
  
  /* two passes: game over */
  gmp_sendPass(ge);   
  
  /* We hang around here until cgoban asks us to go, since
   * sometimes cgoban crashes if we exit first.
   */
  
  if (!quiet)
    fprintf(stderr, "Game over - waiting for client to shut us down\n");
  who_wins(mycolor, stderr);

  if (showtime) {
      gprintf("\nSLOWEST MOVE: %d at %1m ", slowest_movenum, slowest_move);
      fprintf(stderr, "(%.2f seconds)\n", slowest_time);
      fprintf(stderr, "\nAVERAGE TIME: %.2f seconds per move\n",
	      total_time / movenum);
      fprintf(stderr, "\nTOTAL TIME: %.2f seconds\n",
	      total_time);
  }
  
  
  /* play_gmp() does not return to main(), therefore the score
   * writing code is here.
   */
  score = gnugo_estimate_score(&upper_bound, &lower_bound);

  sgfWriteResult(sgftree.root, score, 1);
  sgffile_output(&sgftree);
  
  while (1) {
    message = gmp_check(ge, 1, &j, &i, &error);
    if (!quiet)
      fprintf(stderr, "Message %d from gmp\n", message);
    if (message == gmp_err)
      break;
  }

#if ORACLE
  if (metamachine && oracle_exists)
    dismiss_oracle();
#endif

  if (!quiet)
    fprintf(stderr, "gnugo going down\n");
}

/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
