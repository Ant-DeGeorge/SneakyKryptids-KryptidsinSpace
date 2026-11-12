// shared between ground.c (logic) and ground_draw.c (sprites and raster): mission 7 is too big for one 16 KB bank
#ifndef GROUND_INT_H
#define GROUND_INT_H
#define MAXG 8
#define MAXTB 2
#define FIELD_BOT 128
#define HEAD_ROW 8
enum { P_WALK, P_AMBUSH, P_BRUISER, P_WRECK, P_GUNS, P_DARK, P_BOSS, P_END };
enum { G_NONE, G_CRAWL, G_SPIT, G_TROOP, G_CHARGE, G_BRUISE, G_CLAW };
extern uint8_t gr_fscy, gr_phase, gr_vis, gr_gt[MAXG], gr_gx[MAXG], gr_gy[MAXG], gr_gs[MAXG], gr_swing, gr_warn, gr_warn_x;
extern uint8_t grd_facing, grd_walk_t, grd_p_inv, grd_howl_t, grd_shake_t, grd_gfl[MAXG], grd_tb_on[MAXTB];
extern int8_t grd_sway;
extern uint16_t grd_t, grd_end_t, grd_tb_x[MAXTB], grd_tb_y[MAXTB];
#define facing grd_facing
#define walk_t grd_walk_t
#define p_inv grd_p_inv
#define howl_t grd_howl_t
#define shake_t grd_shake_t
#define gfl grd_gfl
#define tb_on grd_tb_on
#define tb_x grd_tb_x
#define tb_y grd_tb_y
#define sway grd_sway
#define gr_t grd_t
#define end_t grd_end_t
#define band_bot() ((uint8_t)((comm_on ? 32 : 0) + (comm_on && gr_vis > 24 ? 24 : gr_vis)))
void ground_draw(void) BANKED;
#endif
