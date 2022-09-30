/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * This is GNU Go, a Go program. Contact gnugo@gnu.org, or see   *
 * http://www.gnu.org/software/gnugo/ for more information.      *
 *                                                               *
 * To facilitate development of the Go Text Protocol, the two    *
 * files gtp.c and gtp.h are licensed under less restrictive     *
 * terms than the rest of GNU Go.                                *
 *                                                               *
 * Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,     *
 * 2009, 2010 and 2011 by the Free Software Foundation.          *
 *                                                               *
 * Permission is hereby granted, free of charge, to any person   *
 * obtaining a copy of this file gtp.h, to deal in the Software  *
 * without restriction, including without limitation the rights  *
 * to use, copy, modify, merge, publish, distribute, and/or      *
 * sell copies of the Software, and to permit persons to whom    *
 * the Software is furnished to do so, provided that the above   *
 * copyright notice(s) and this permission notice appear in all  *
 * copies of the Software and that both the above copyright      *
 * notice(s) and this permission notice appear in supporting     *
 * documentation.                                                *
 *                                                               *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY     *
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE    *
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR       *
 * PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO      *
 * EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS  *
 * NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR    *
 * CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING    *
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF    *
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT    *
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS       *
 * SOFTWARE.                                                     *
 *                                                               *
 * Except as contained in this notice, the name of a copyright   *
 * holder shall not be used in advertising or otherwise to       *
 * promote the sale, use or other dealings in this Software      *
 * without prior written authorization of the copyright holder.  *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* NOTE: GNU Go specific, workarounds dumb Windows sockets. */
#include "winsocket.h"

#include <stdarg.h>
#include <stdio.h>

/* Maximum allowed line length in GTP. */
#define GTP_BUFSIZE 1000

/* Status returned from callback functions. */
#define GTP_QUIT    -1
#define GTP_OK       0
#define GTP_FATAL    1

/* Whether the GTP command was successful. */
#define GTP_SUCCESS  0
#define GTP_FAILURE  1

/* Function pointer for callback functions. */
typedef int (*gtp_fn_ptr)(char *s);

/* Function pointer for vertex transform functions. */
typedef void (*gtp_transform_ptr)(int ai, int aj, int *bi, int *bj);

/* Elements in the array of commands required by gtp_main_loop. */
struct gtp_command {
  const char *name;
  gtp_fn_ptr function;
};

void gtp_main_loop(struct gtp_command commands[],
		   FILE *gtp_input, FILE *gtp_output, FILE *gtp_dump_commands);
void gtp_internal_set_boardsize(int size);
void gtp_set_vertex_transform_hooks(gtp_transform_ptr in,
				    gtp_transform_ptr out);
void gtp_mprintf(const char *format, ...);
void gtp_printf(const char *format, ...);
void gtp_start_response(int status);
int gtp_finish_response(void);
int gtp_success(const char *format, ...);
int gtp_failure(const char *format, ...);
void gtp_panic(void);
int gtp_decode_color(char *s, int *color);
int gtp_decode_coord(char *s, int *m, int *n);
int gtp_decode_move(char *s, int *color, int *i, int *j);
void gtp_print_vertices(int n, int movei[], int movej[]);
void gtp_print_vertex(int i, int j);

extern FILE *gtp_output_file;

/* Internal state that's not part of the engine. */
 static int report_uncertainty;
 static int gtp_orientation;

void rotate_on_input(int ai, int aj, int *bi, int *bj);
void rotate_on_output(int ai, int aj, int *bi, int *bj);
void gtp_init(int board_size, int gtp_initial_orientation);


static void gtp_print_code(int c);
static void gtp_print_vertices2(int n, int *moves);


#define DECLARE(func) int func(const char *s)

DECLARE(gtp_aa_confirm_safety);
DECLARE(gtp_accurate_approxlib);
DECLARE(gtp_accuratelib);
DECLARE(gtp_advance_random_seed);
DECLARE(gtp_all_legal);
DECLARE(gtp_all_move_values);
DECLARE(gtp_analyze_eyegraph);
DECLARE(gtp_analyze_semeai);
DECLARE(gtp_analyze_semeai_after_move);
DECLARE(gtp_attack);
DECLARE(gtp_attack_either);
DECLARE(gtp_block_off);
DECLARE(gtp_break_in);
DECLARE(gtp_captures);
DECLARE(gtp_clear_board);
DECLARE(gtp_clear_cache);
DECLARE(gtp_combination_attack);
DECLARE(gtp_combination_defend);
DECLARE(gtp_connect);
DECLARE(gtp_countlib);
DECLARE(gtp_cputime);
DECLARE(gtp_decrease_depths);
DECLARE(gtp_defend);
DECLARE(gtp_defend_both);
DECLARE(gtp_disconnect);
DECLARE(gtp_does_attack);
DECLARE(gtp_does_defend);
DECLARE(gtp_does_surround);
DECLARE(gtp_dragon_data);
DECLARE(gtp_dragon_status);
DECLARE(gtp_dragon_stones);
DECLARE(gtp_draw_search_area);
DECLARE(gtp_dump_stack);
DECLARE(gtp_echo);
DECLARE(gtp_echo_err);
DECLARE(gtp_estimate_score);
DECLARE(gtp_eval_eye);
DECLARE(gtp_experimental_score);
DECLARE(gtp_eye_data);
DECLARE(gtp_final_score);
DECLARE(gtp_final_status);
DECLARE(gtp_final_status_list);
DECLARE(gtp_findlib);
DECLARE(gtp_finish_sgftrace);
DECLARE(gtp_fixed_handicap);
DECLARE(gtp_followup_influence);
DECLARE(gtp_genmove);
DECLARE(gtp_genmove_black);
DECLARE(gtp_genmove_white);
DECLARE(gtp_get_connection_node_counter);
DECLARE(gtp_get_handicap);
DECLARE(gtp_get_komi);
DECLARE(gtp_get_life_node_counter);
DECLARE(gtp_get_owl_node_counter);
DECLARE(gtp_get_random_seed);
DECLARE(gtp_get_reading_node_counter);
DECLARE(gtp_get_trymove_counter);
DECLARE(gtp_gg_genmove);
DECLARE(gtp_gg_undo);
DECLARE(gtp_half_eye_data);
DECLARE(gtp_increase_depths);
DECLARE(gtp_initial_influence);
DECLARE(gtp_invariant_hash);
DECLARE(gtp_invariant_hash_for_moves);
DECLARE(gtp_is_legal);
DECLARE(gtp_is_surrounded);
DECLARE(gtp_kgs_genmove_cleanup);
DECLARE(gtp_known_command);
DECLARE(gtp_ladder_attack);
DECLARE(gtp_last_move);
DECLARE(gtp_limit_search);
DECLARE(gtp_list_commands);
DECLARE(gtp_list_stones);
DECLARE(gtp_loadsgf);
DECLARE(gtp_move_influence);
DECLARE(gtp_move_probabilities);
DECLARE(gtp_move_reasons);
DECLARE(gtp_move_uncertainty);
DECLARE(gtp_move_history);
DECLARE(gtp_name);
DECLARE(gtp_owl_attack);
DECLARE(gtp_owl_connection_defends);
DECLARE(gtp_owl_defend);
DECLARE(gtp_owl_does_attack);
DECLARE(gtp_owl_does_defend);
DECLARE(gtp_owl_substantial);
DECLARE(gtp_owl_threaten_attack);
DECLARE(gtp_owl_threaten_defense);
DECLARE(gtp_place_free_handicap);
DECLARE(gtp_play);
DECLARE(gtp_playblack);
DECLARE(gtp_playwhite);
DECLARE(gtp_popgo);
DECLARE(gtp_printsgf);
DECLARE(gtp_program_version);
DECLARE(gtp_protocol_version);
DECLARE(gtp_query_boardsize);
DECLARE(gtp_query_orientation);
DECLARE(gtp_quit);
DECLARE(gtp_reg_genmove);
DECLARE(gtp_report_uncertainty);
DECLARE(gtp_reset_connection_node_counter);
DECLARE(gtp_reset_life_node_counter);
DECLARE(gtp_reset_owl_node_counter);
DECLARE(gtp_reset_reading_node_counter);
DECLARE(gtp_reset_search_mask);
DECLARE(gtp_reset_trymove_counter);
DECLARE(gtp_restricted_genmove);
DECLARE(gtp_same_dragon);
DECLARE(gtp_set_boardsize);
DECLARE(gtp_set_free_handicap);
DECLARE(gtp_set_komi);
DECLARE(gtp_set_level);
DECLARE(gtp_set_orientation);
DECLARE(gtp_set_random_seed);
DECLARE(gtp_set_search_diamond);
DECLARE(gtp_set_search_limit);
DECLARE(gtp_showboard);
DECLARE(gtp_start_sgftrace);
DECLARE(gtp_surround_map);
DECLARE(gtp_test_eyeshape);
DECLARE(gtp_time_left);
DECLARE(gtp_time_settings);
DECLARE(gtp_top_moves);
DECLARE(gtp_top_moves_black);
DECLARE(gtp_top_moves_white);
DECLARE(gtp_tryko);
DECLARE(gtp_trymove);
DECLARE(gtp_tune_move_ordering);
DECLARE(gtp_unconditional_status);
DECLARE(gtp_undo);
DECLARE(gtp_what_color);
DECLARE(gtp_worm_cutstone);
DECLARE(gtp_worm_data);
DECLARE(gtp_worm_stones);

/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
