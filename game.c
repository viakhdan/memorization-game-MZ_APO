/**
 * @file game.c
 * @brief Game-logic helpers: timing, target generation and scoring.
 * @author Danylo Viakhiriev, Igor Podlesnyy
 * * BE5B35APO - Computer Architectures
 * Semestral Project (preliminary task)
 */

#include "memgame.h"

/**
 * @brief Returns elapsed time in milliseconds relative to a start marker
 * @param start Pointer to the timespec structure marking the start time
 * @return Elapsed time in milliseconds
 */
long elapsed_ms(const struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec  - start->tv_sec)  * 1000L +
           (now.tv_nsec - start->tv_nsec) / 1000000L;
}

/**
 * @brief Randomly places n objects across the screen, avoiding the status area
 * @param n_figs Number of targets to generate
 */
void generate_targets(int n_figs)
{
    int margin = FIG_SIZE + 12;
    int top    = TOP_RESERVED + margin;
    int xrange = SCREEN_W - 2 * margin;
    int yrange = SCREEN_H - margin - top;

    for (int i = 0; i < n_figs; ++i) {
        targets[i].shape     = rand() % N_SHAPES;
        targets[i].color_idx = rand() % N_COLORS;
        targets[i].x         = margin + rand() % xrange;
        targets[i].y         = top    + rand() % yrange;
    }
}

/**
 * @brief Awards colour credit by RGB distance.
 * @details Only an exact hue earns full points; the closest palette pairs earn
 * a single point; everything else (mid / complementary) earns nothing.
 * @param a_idx Index of the first color in the palette
 * @param b_idx Index of the second color in the palette
 * @return Color score
 */
static int color_score(int a_idx, int b_idx)
{
    if (a_idx == b_idx) return PTS_COLOR;
    uint32_t a = led_palette[a_idx], b = led_palette[b_idx];

    int dr = (int)((a >> 16) & 0xFF) - (int)((b >> 16) & 0xFF);
    int dg = (int)((a >>  8) & 0xFF) - (int)((b >>  8) & 0xFF);
    int db = (int)( a        & 0xFF) - (int)( b        & 0xFF);
    int dist = isqrt_i(dr * dr + dg * dg + db * db);

    if (dist <= 260) return 1; /* Adjacent hue: small partial credit */
    return 0;                  /* Mid / complementary: no credit */
}

/**
 * @brief Scores a single placed object against a target object
 * @param t Pointer to the target figure
 * @param g Pointer to the guessed figure
 * @return Score for the single pair
 */
static int score_pair(const figure_t *t, const figure_t *g)
{
    int shape_ok    = (g->shape == t->shape);
    int color_pts   = color_score(g->color_idx, t->color_idx);
    int color_exact = (g->color_idx == t->color_idx);

    int dx = g->x - t->x;
    int dy = g->y - t->y;
    int dist2 = dx * dx + dy * dy;
    int tol2  = POS_TOLERANCE_PX * POS_TOLERANCE_PX;

    /* Quadratic falloff for positional distance inside tolerance radius */
    int pos_pts = 0;
    if (dist2 <= tol2) {
        int penalty = (PTS_POS * dist2 + tol2 / 2) / tol2;
        pos_pts = PTS_POS - penalty;
        if (pos_pts < 0) pos_pts = 0;
    }

    /* Total miss: wrong shape AND wrong colour earns nothing, no matter
       how close it landed. Stops random placement from banking points. */
    if (!shape_ok && color_pts == 0)
        return 0;

    int pts = (shape_ok ? PTS_SHAPE : 0) + color_pts + pos_pts;

    /* Combo bonus: correct shape, exact colour and a tight placement
       (within half the tolerance radius). */
    if (shape_ok && color_exact && dist2 * 4 <= tol2)
        pts += PTS_COMBO;

    return pts;
}

/**
 * @brief Best-match (greedy) scoring
 * @details Each player guess is paired with the unmatched target that yields
 * the highest score. Order of placement does not matter.
 * @param n_figs Number of figures placed in the round
 * @return Total score accumulated for the round
 */
int round_score(int n_figs)
{
    int used[MAX_FIGS_PER_ROUND] = { 0 };
    int total = 0;
    for (int i = 0; i < n_figs; ++i) {
        int best_pts = -1, best_j = -1;
        for (int j = 0; j < n_figs; ++j) {
            if (used[j]) continue;
            int pts = score_pair(&targets[j], &guesses[i]);
            if (pts > best_pts) { best_pts = pts; best_j = j; }
        }
        if (best_j >= 0) {
            used[best_j] = 1;
            total += best_pts;
        }
    }
    return total;
}