/**
* @file state.c
 * @brief Definitions of the global mutable game state.
 * @author Danylo Viakhiriev, Igor Podlesnyy
 * * BE5B35APO - Computer Architectures
 * Semestral Project (preliminary task)
 */

#include "memgame.h"

/* Global state buffers */
unsigned short *fb;                          /**< @brief Framebuffer pointer */
figure_t        targets[MAX_FIGS_PER_ROUND]; /**< @brief The objects to memorize */
figure_t        guesses[MAX_FIGS_PER_ROUND]; /**< @brief The player's recreations */
int             round_scores[ROUNDS_PER_GAME]; /**< @brief Score tracking */