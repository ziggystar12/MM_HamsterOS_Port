/* combat_port.c — Turn-based combat for HamsterOS port.
 * Ported from combat.c (MM_C_Port). No SDL — updates CombatSession state,
 * sets player messages. mm_port.c owns the state machine and draws the screen. */

#include "combat_port.h"
#include "items.h"
#include "mazedata.h"
#include "spells.h"
#include "mm_util.h"
#include <stddef.h>
#include <stdbool.h>

/* rand() replaced by LCG in monsters.c via #define */
extern int monster_roll(int sides);
extern int monster_count(void);
extern void monster_rng_seed(unsigned seed);
extern void encounter_generate(int max_level, int max_qty,
                                MonsterGroup *out, int *out_count);
extern uint32_t pit_millis(void);

static int s_ovr_max_level;
static int s_ovr_max_qty;

void combat_set_ovr_limits(int max_level, int max_qty)
{
    s_ovr_max_level = max_level;
    s_ovr_max_qty = max_qty;
}

/* ---- stat / item helpers ---- */
#define STAT_INT 0
#define STAT_MIG 1
#define STAT_PER 2
#define STAT_END 3
#define STAT_SPD 4
#define STAT_ACC 5
#define STAT_LCK 6

static int stat_mod(const Character *c, int si) {
    int v = c->stats[si], m = (v-10)/2;
    return m < 0 ? 0 : m;
}
static int weapon_dmg_bonus(const Character *c) {
    int bonus=0, s;
    for (s=0; s<6; s++) {
        const ItemDef *it = item_get(c->equipped[s]);
        if (it && (it->slot==1||it->slot==2||it->slot==3)) bonus+=it->dmg_bonus;
    }
    return bonus;
}
static int has_missile_weapon(const Character *c) {
    int s;
    for (s=0; s<6; s++) {
        const ItemDef *it = item_get(c->equipped[s]);
        if (it && it->slot==SLOT_MISSILE) return 1;
    }
    return 0;
}
static int equipped_ac(const Character *c) {
    int ac=c->ac, i;
    for (i=0; i<6; i++) {
        const ItemDef *it = item_get(c->equipped[i]);
        if (it && (it->slot==4||it->slot==5)) ac -= it->ac_bonus;
    }
    int spd=c->stats[STAT_SPD];
    ac += (spd>=25)?-3:(spd>=20)?-2:(spd>=16)?-1:(spd>=9)?0:(spd>=7)?1:2;
    return ac;
}
static int is_alive(const Character *c) {
    return !(c->condition==0xFF||c->condition==0xA0||c->condition==0xC0);
}
static int can_act_c(int cond) {
    return !(cond==0xFF||cond==0xA0||cond==0xC0) &&
           !(cond & (COND_ASLEEP|COND_PARALYZED|COND_UNCONSCIOUS));
}
static int party_wiped(const Party *p) {
    int i;
    for (i=0; i<p->count; i++) {
        int c=p->members[i].condition;
        if (!(c==0xFF||c==0xA0||c==0xC0) && !(c&COND_UNCONSCIOUS)) return 0;
    }
    return 1;
}
static int monsters_alive(const CombatSession *cs) {
    int g;
    for (g=0; g<cs->n_groups; g++) if (cs->groups[g].alive>0) return 1;
    return 0;
}
static int first_alive_group(const CombatSession *cs) {
    int g;
    for (g=0; g<cs->n_groups; g++) if (cs->groups[g].alive>0) return g;
    return -1;
}
static int random_alive_monster(MonsterGroup *mg) {
    int cands[MAX_MONSTER_GROUP], nc=0, i;
    for (i=0; i<MAX_MONSTER_GROUP; i++) if (mg->hp[i]>0) cands[nc++]=i;
    if (!nc) return -1;
    return cands[monster_roll(nc)-1];
}
static int random_alive_party(const Party *p) {
    int cands[MAX_PARTY], nc=0, i;
    for (i=0; i<p->count; i++)
        if (is_alive(&p->members[i]) && !(p->members[i].condition&COND_UNCONSCIOUS))
            cands[nc++]=i;
    if (!nc) return -1;
    return cands[monster_roll(nc)-1];
}

static int random_protector(const CombatSession *cs, const Party *p, int target) {
    int cands[MAX_PARTY], nc=0, i;
    for (i=0; i<p->count; i++) {
        if (i==target) continue;
        if (!cs->protecting[i]) continue;
        if (is_alive(&p->members[i]) && !(p->members[i].condition&COND_UNCONSCIOUS))
            cands[nc++]=i;
    }
    if (!nc) return target;
    return cands[monster_roll(nc)-1];
}

/* ---- Log helpers ---- */
static void log_msg(CombatSession *cs, const char *s) {
    mm_strncpy(cs->log[cs->log_head], s, COMBAT_LOG_LEN);
    cs->log_head = (cs->log_head+1) % COMBAT_LOG_LINES;
    if (cs->log_count < COMBAT_LOG_LINES) cs->log_count++;
}
static void log_fmt2(CombatSession *cs, const char *a, const char *b, int n) {
    /* "a verb b for N" without snprintf */
    char buf[COMBAT_LOG_LEN]; int pos=0;
    while (*a && pos<30) buf[pos++]=*a++;
    buf[pos++]=' ';
    while (*b && pos<50) buf[pos++]=*b++;
    if (n > 0) {
        buf[pos++]=' '; buf[pos++]='(';
        if (n>=100) buf[pos++]=(char)('0'+n/100);
        if (n>=10)  buf[pos++]=(char)('0'+(n/10)%10);
        buf[pos++]=(char)('0'+n%10);
        buf[pos++]=')';
    }
    buf[pos]='\0';
    log_msg(cs, buf);
}

void combat_note_monster_kill(CombatSession *cs, const MonsterType *mt)
{
    if (!mt) return;
    cs->pending_xp += mt->xp;
    cs->pending_gold += monster_roll(mt->level*5+1);
    if (mt->loot & 1) cs->pending_gems += monster_roll(mt->level+1);
    if (mt->loot & 0x7E) cs->treasure_flags |= mt->loot;
    if ((mt->loot & 0x7E) && !cs->treasure_container) {
        cs->treasure_container = (monster_roll(100)<=50) ? 1 : 2;
        cs->treasure_trap = (mt->level + monster_roll(8)) & 7;
    }
    cs->n_hits_dealt += 10;
}

static void log_round(CombatSession *cs)
{
    char buf[24];
    mm_snprintf(buf, sizeof(buf), "Round %d:", cs->round);
    log_msg(cs, buf);
}

static void setup_can_attack(CombatSession *cs, GameState *gs, const struct Map *m)
{
    int i, outdoor = (m && m->outdoor) || (gs->map_idx>=14 && gs->map_idx<=33);
    for (i=0; i<MAX_PARTY; i++) cs->can_attack[i]=0;
    if (outdoor) {
        for (i=0; i<gs->party.count; i++)
            cs->can_attack[i] = (i<5) || (monster_roll(100)<=10);
        return;
    }

    if (gs->party.count>0) cs->can_attack[0]=1;
    if (gs->party.count>1) cs->can_attack[1]=1;

    int left_open=1, right_open=1;
    if (m) {
        const Cell *c = map_cell(m, gs->player.x, gs->player.y);
        if (c) {
            int lf = WALL_LEFT[gs->player.facing];
            int rf = WALL_RIGHT[gs->player.facing];
            left_open = c->pass[lf] || c->wall[lf] != WT_WALL;
            right_open = c->pass[rf] || c->wall[rf] != WT_WALL;
        }
    }
    if (gs->party.count>2) cs->can_attack[2] = left_open || (monster_roll(100)<=25);
    if (gs->party.count>3) cs->can_attack[3] = right_open || (monster_roll(100)<=25);
    if (gs->party.count>4) cs->can_attack[4] = cs->can_attack[2] && (monster_roll(100)<=5);
    if (gs->party.count>5) cs->can_attack[5] = cs->can_attack[3] && (monster_roll(100)<=5);
}

static void begin_round_flow(CombatSession *cs, GameState *gs, const struct Map *m)
{
    int i;
    cs->round++;
    cs->current_char = -1;
    cs->pending_action = COMBAT_ACT_NONE;
    cs->n_hits_dealt = 0;
    for (i=0; i<MAX_PARTY; i++) {
        cs->checked_party[i]=0;
        cs->defending[i]=0;
        cs->protecting[i]=0;
    }
    for (i=0; i<4; i++) cs->checked_group[i]=0;
    if (cs->bless_rounds > 0) cs->bless_rounds--;
    setup_can_attack(cs, gs, m);
    log_round(cs);
}

static int all_party_checked(CombatSession *cs, const Party *p)
{
    int i;
    for (i=0; i<p->count; i++)
        if (can_act_c(p->members[i].condition) && !cs->checked_party[i]) return 0;
    return 1;
}

static int all_groups_checked(const CombatSession *cs)
{
    int g;
    for (g=0; g<cs->n_groups; g++)
        if (cs->groups[g].alive>0 && !cs->checked_group[g]) return 0;
    return 1;
}

static int choose_next_party(const CombatSession *cs, const Party *p)
{
    int i, best=-1, best_sp=-999;
    for (i=0; i<p->count; i++) {
        const Character *c=&p->members[i];
        if (cs->checked_party[i] || !can_act_c(c->condition)) continue;
        if (c->stats[STAT_SPD] > best_sp) { best=i; best_sp=c->stats[STAT_SPD]; }
    }
    return best;
}

static int choose_next_group(const CombatSession *cs)
{
    int g, best=-1, best_sp=-999;
    for (g=0; g<cs->n_groups; g++) {
        const MonsterGroup *mg=&cs->groups[g];
        int sp;
        if (mg->alive<=0 || cs->checked_group[g]) continue;
        sp = mg->type->level*3 + monster_roll(6);
        if (sp > best_sp) { best=g; best_sp=sp; }
    }
    return best;
}

/* ---- Init ---- */
void combat_init(CombatSession *cs, GameState *gs) {
    int g;
    mm_memset(cs, 0, sizeof(*cs));
    monster_rng_seed((unsigned)pit_millis());
    int max_lv = 1, max_qty = 3;
    if (gs->map_idx >= 5) { max_lv=3; max_qty=4; }
    if (gs->map_idx >= 14) { max_lv=5; max_qty=5; }
    if (gs->map_idx >= 34) { max_lv=8; max_qty=6; }
    if (s_ovr_max_level > 0) max_lv = s_ovr_max_level;
    if (s_ovr_max_qty > 0) max_qty = s_ovr_max_qty;
    if (max_lv < 1) max_lv = 1;
    if (max_lv > 10) max_lv = 10;
    if (max_qty < 1) max_qty = 1;
    if (max_qty > 8) max_qty = 8;
    encounter_generate(max_lv, max_qty, cs->groups, &cs->n_groups);
    cs->round = 0;
    cs->over  = false;
    cs->current_char = -1;
    cs->target_group = -1;
    for (g=0; g<cs->n_groups && g<4; g++)
        cs->group_range[g] = (g==0) ? 0 : (g<3 ? 1 : 2);
}

/* ---- Party attacks ---- */
static int party_attack(CombatSession *cs, Party *p, int map_idx) {
    int xp=0, ao, i;
    int gi = (cs->target_group>=0&&cs->target_group<cs->n_groups&&
              cs->groups[cs->target_group].alive>0)
           ? cs->target_group : first_alive_group(cs);
    if (gi < 0) return 0;
    MonsterGroup *mg = &cs->groups[gi];

    /* Sort attack order by SPD */
    int order[MAX_PARTY], na=0;
    for (i=0; i<p->count; i++) order[na++]=i;
    for (ao=1; ao<na; ao++) {
        int key=order[ao], b=ao-1;
        while (b>=0 && p->members[order[b]].stats[STAT_SPD]<p->members[key].stats[STAT_SPD])
            { order[b+1]=order[b]; b--; }
        order[b+1]=key;
    }

    for (ao=0; ao<na; ao++) {
        i = order[ao];
        Character *c = &p->members[i];
        if (!can_act_c(c->condition)) continue;

        /* Dungeon position restrictions */
        if (map_idx>=5 && map_idx<14) {
            if ((i==2||i==3) && monster_roll(100)>=75) continue;
            if (i>=4 && monster_roll(100)>=25) continue;
        }

        int acc = c->stats[STAT_ACC];
        int acc_b = (acc>=40)?7:(acc>=35)?6:(acc>=30)?5:(acc>=24)?4:(acc>=19)?3:
                    (acc>=16)?2:(acc>=13)?1:(acc>=9)?0:(acc>=7)?-1:-2;
        int mig = c->stats[STAT_MIG];
        int mig_b = (mig>=40)?13:(mig>=35)?12:(mig>=29)?11:(mig>=27)?10:(mig>=25)?9:
                    (mig>=23)?8:(mig>=21)?7:(mig>=19)?6:(mig>=18)?5:(mig>=17)?4:
                    (mig>=16)?3:(mig>=15)?2:(mig>=13)?1:(mig>=9)?0:-1;

        int target = 10 + (mg->type->ac<10 ? 10-mg->type->ac : 0);
        int bless_b = (cs->bless_rounds > 0) ? cs->bless_bonus : 0;
        int atks = (c->cls==4 && c->level>=8) ? 1+c->level/8 : 1;
        int ai;
        for (ai=0; ai<atks; ai++) {
            int d20r = monster_roll(20);
            int hits = (d20r==20) || (d20r!=1 && (d20r+c->level+acc_b+bless_b)>=target);
            if (!hits) { log_fmt2(cs, c->name, "misses", 0); continue; }

            int dmg = monster_roll(6+c->level) + stat_mod(c,STAT_MIG)
                    + weapon_dmg_bonus(c) + mig_b;
            if (c->cls==3) dmg += (c->level+1)/2;  /* Archer bonus */
            if (d20r==20) { dmg*=2; log_msg(cs,"Critical hit!"); }
            if (dmg<1) dmg=1;

            int mi = random_alive_monster(mg);
            if (mi<0) break;
            mg->hp[mi] -= dmg;
            log_fmt2(cs, c->name, "hits", dmg);
            cs->n_hits_dealt++;

            if (mg->hp[mi]<=0) {
                mg->hp[mi]=0; mg->alive--;
                cs->n_hits_dealt += 10;  /* +10 signals a kill (≥10) vs plain hit */
                xp += mg->type->xp;
                combat_note_monster_kill(cs, mg->type);
                cs->pending_xp -= mg->type->xp;
                { char buf[40]; int bi=0;
                  while (mg->type->name[bi] && bi<20) { buf[bi]=mg->type->name[bi]; bi++; }
                  buf[bi++]=' '; buf[bi++]='s'; buf[bi++]='l'; buf[bi++]='a'; buf[bi++]='i'; buf[bi++]='n'; buf[bi++]='!'; buf[bi]='\0';
                  log_msg(cs, buf); }
                gi = (cs->target_group>=0&&cs->target_group<cs->n_groups&&
                      cs->groups[cs->target_group].alive>0)
                   ? cs->target_group : first_alive_group(cs);
                if (gi<0) break;
                mg = &cs->groups[gi];
            }
        }
    }
    return xp;
}

/* ---- Monsters attack ---- */
static void monsters_attack(CombatSession *cs, Party *p) {
    int g, atk;
    for (g=0; g<cs->n_groups; g++) {
        MonsterGroup *mg = &cs->groups[g];
        if (!mg->alive) continue;
        if (mg->sleep_rounds>0) { mg->sleep_rounds--; continue; }
        for (atk=0; atk<mg->type->num_attacks; atk++) {
            int pi = random_alive_party(p);
            if (pi<0) return;
            pi = random_protector(cs, p, pi);
            Character *c = &p->members[pi];
            int roll = monster_roll(20);
            int eff_ac = equipped_ac(c);
            int target = 10 + (eff_ac<10 ? 10-eff_ac : 0);
            if (roll+mg->type->level+2 < target) {
                log_fmt2(cs, mg->type->name, "misses", 0); continue;
            }
            int dmg = monster_roll(mg->type->max_dmg);
            if (dmg<1) dmg=1;
            if (cs->defending[pi]) dmg = (dmg+1)/2;
            c->hp -= dmg;
            log_fmt2(cs, mg->type->name, "hits", dmg);
            cs->n_hits_dealt++;
            if (c->hp<=0) {
                c->hp=0;
                if (monster_roll(100)<=12) { c->condition=COND_DEAD; log_msg(cs,"...is KILLED!"); }
                else { c->condition|=COND_UNCONSCIOUS; log_msg(cs,"...unconscious!"); }
            }
            /* Special attacks */
            if (mg->type->special>0 && monster_roll(100)<=25) {
                int pi2=random_alive_party(p); if (pi2<0) continue;
                Character *t=&p->members[pi2];
                int sdmg=monster_roll(mg->type->level*3+5);
                switch(mg->type->special) {
                case 1: case 2: case 3: t->hp-=sdmg; log_fmt2(cs,mg->type->name,"special",sdmg); break;
                case 4: t->condition|=COND_POISONED; log_msg(cs,"...POISONED!"); break;
                case 5: if(monster_roll(100)<=40){t->condition|=COND_PARALYZED; log_msg(cs,"...PARALYZED!");} break;
                case 6: if(monster_roll(100)<=25&&!IS_DEADLIKE(t->condition)){t->condition=COND_DEAD;t->hp=0;log_msg(cs,"...DEATH GAZE!");} break;
                case 7: { /* psychic blast: drain INT */
                    int drain=monster_roll(mg->type->level+2);
                    t->stats[0]-=drain; if(t->stats[0]<1)t->stats[0]=1;
                    log_msg(cs,"...INT DRAINED!");
                } break;
                default: break;
                }
                if (t->hp<=0 && !IS_DEADLIKE(t->condition)) { t->hp=0; t->condition=COND_DEAD; }
            }
        }
    }
}

static void monster_group_turn(CombatSession *cs, Party *p, int g)
{
    int atk;
    MonsterGroup *mg;
    if (g<0 || g>=cs->n_groups) return;
    mg = &cs->groups[g];
    if (mg->alive<=0) return;
    if (mg->sleep_rounds>0) { mg->sleep_rounds--; log_fmt2(cs, mg->type->name, "sleeps", 0); return; }
    if (cs->group_range[g]>0) {
        cs->group_range[g]--;
        log_fmt2(cs, mg->type->name, "advances", 0);
        return;
    }
    for (atk=0; atk<mg->type->num_attacks; atk++) {
        int pi = random_alive_party(p);
        Character *c;
        int roll, eff_ac, target, dmg;
        if (pi<0) return;
        pi = random_protector(cs, p, pi);
        c = &p->members[pi];
        roll = monster_roll(20);
        eff_ac = equipped_ac(c);
        target = 10 + (eff_ac<10 ? 10-eff_ac : 0);
        if (roll+mg->type->level+2 < target) {
            log_fmt2(cs, mg->type->name, "misses", 0);
            continue;
        }
        dmg = monster_roll(mg->type->max_dmg);
        if (dmg<1) dmg=1;
        if (cs->defending[pi]) dmg=(dmg+1)/2;
        c->hp -= dmg;
        log_fmt2(cs, mg->type->name, "hits", dmg);
        cs->n_hits_dealt++;
        if (c->hp<=0) {
            c->hp=0;
            if (monster_roll(100)<=12) { c->condition=COND_DEAD; log_msg(cs,"...is KILLED!"); }
            else { c->condition|=COND_UNCONSCIOUS; log_msg(cs,"...unconscious!"); }
        }
        if (mg->type->special>0 && monster_roll(100)<=25) {
            int pi2=random_alive_party(p);
            Character *t;
            int sdmg;
            if (pi2<0) continue;
            pi2=random_protector(cs,p,pi2);
            t=&p->members[pi2];
            sdmg=monster_roll(mg->type->level*3+5);
            switch(mg->type->special) {
            case 1: case 2: case 3: t->hp-=sdmg; log_fmt2(cs,mg->type->name,"special",sdmg); break;
            case 4: t->condition|=COND_POISONED; log_msg(cs,"...POISONED!"); break;
            case 5: if(monster_roll(100)<=40){t->condition|=COND_PARALYZED; log_msg(cs,"...PARALYZED!");} break;
            case 6: if(monster_roll(100)<=25&&!IS_DEADLIKE(t->condition)){t->condition=COND_DEAD;t->hp=0;log_msg(cs,"...DEATH GAZE!");} break;
            case 7: {
                int drain=monster_roll(mg->type->level+2);
                t->stats[0]-=drain; if(t->stats[0]<1)t->stats[0]=1;
                log_msg(cs,"...INT DRAINED!");
            } break;
            default: break;
            }
            if (t->hp<=0 && !IS_DEADLIKE(t->condition)) { t->hp=0; t->condition=COND_DEAD; }
        }
    }
}

static void combat_apply_condition_damage(CombatSession *cs, GameState *gs) {
    int pi;
    for (pi=0; pi<gs->party.count; pi++) {
        Character *c=&gs->party.members[pi];
        if (!IS_DEADLIKE(c->condition) && (c->condition & COND_POISONED)) {
            c->hp--; cs->n_hits_dealt++;
            if (c->hp<=0) { c->hp=0; c->condition|=COND_UNCONSCIOUS; }
        }
    }
}

static int character_attack_group(CombatSession *cs, Party *p, int ci, int gi, int shoot)
{
    Character *c;
    MonsterGroup *mg;
    int acc, acc_b, mig, mig_b, target, bless_b, atks, ai;
    if (ci<0 || ci>=p->count) return 0;
    if (gi<0 || gi>=cs->n_groups || cs->groups[gi].alive<=0) return 0;
    c=&p->members[ci];
    if (!can_act_c(c->condition)) return 0;
    mg=&cs->groups[gi];

    acc = c->stats[STAT_ACC];
    acc_b = (acc>=40)?7:(acc>=35)?6:(acc>=30)?5:(acc>=24)?4:(acc>=19)?3:
            (acc>=16)?2:(acc>=13)?1:(acc>=9)?0:(acc>=7)?-1:-2;
    mig = c->stats[STAT_MIG];
    mig_b = (mig>=40)?13:(mig>=35)?12:(mig>=29)?11:(mig>=27)?10:(mig>=25)?9:
            (mig>=23)?8:(mig>=21)?7:(mig>=19)?6:(mig>=18)?5:(mig>=17)?4:
            (mig>=16)?3:(mig>=15)?2:(mig>=13)?1:(mig>=9)?0:-1;
    target = 10 + (mg->type->ac<10 ? 10-mg->type->ac : 0);
    bless_b = (cs->bless_rounds > 0) ? cs->bless_bonus : 0;
    atks = (c->cls==4 && c->level>=8) ? 1+c->level/8 : 1;
    if (shoot) {
        atks = 1;
        if (!has_missile_weapon(c) && c->cls!=3) {
            log_fmt2(cs, c->name, "has no missile", 0);
            return 0;
        }
    }

    for (ai=0; ai<atks; ai++) {
        int d20r = monster_roll(20);
        int hits = (d20r==20) || (d20r!=1 && (d20r+c->level+acc_b+bless_b+(shoot?1:0))>=target);
        int dmg, mi;
        if (!hits) { log_fmt2(cs, c->name, "misses", 0); continue; }
        dmg = monster_roll(6+c->level) + stat_mod(c,STAT_MIG)
            + weapon_dmg_bonus(c) + mig_b;
        if (c->cls==3 || shoot) dmg += (c->level+1)/2;
        if (d20r==20) { dmg*=2; log_msg(cs,"Critical hit!"); }
        if (dmg<1) dmg=1;
        mi = random_alive_monster(mg);
        if (mi<0) break;
        mg->hp[mi] -= dmg;
        log_fmt2(cs, c->name, shoot ? "shoots" : "hits", dmg);
        cs->n_hits_dealt++;
        if (mg->hp[mi]<=0) {
            char buf[40];
            mg->hp[mi]=0;
            mg->alive--;
            combat_note_monster_kill(cs, mg->type);
            mm_snprintf(buf, sizeof(buf), "%s slain!", mg->type->name);
            log_msg(cs, buf);
            if (!monsters_alive(cs)) break;
        }
    }
    return 1;
}

bool combat_monsters_retaliate(CombatSession *cs, GameState *gs) {
    if (cs->over) return true;
    cs->n_hits_dealt = 0;
    monsters_attack(cs, &gs->party);
    combat_apply_condition_damage(cs, gs);
    if (party_wiped(&gs->party)) {
        log_msg(cs, "PARTY DEFEATED!");
        cs->result=CR_DEFEAT;
        cs->over=true;
        return true;
    }
    return false;
}

int combat_current_char(const CombatSession *cs)
{
    return cs ? cs->current_char : -1;
}

int combat_can_current_attack(const CombatSession *cs)
{
    if (!cs || cs->current_char<0 || cs->current_char>=MAX_PARTY) return 0;
    return cs->can_attack[cs->current_char];
}

int combat_group_in_range(const CombatSession *cs, int target, bool shoot)
{
    int gi;
    if (!cs) return 0;
    gi = (target>=0&&target<cs->n_groups&&cs->groups[target].alive>0)
       ? target : first_alive_group(cs);
    if (gi<0) return 0;
    return shoot || cs->group_range[gi]<=0;
}

bool combat_advance_flow(CombatSession *cs, GameState *gs, const struct Map *m)
{
    int safety=0;
    if (cs->over) return true;
    cs->current_char=-1;
    cs->pending_action=COMBAT_ACT_NONE;
    while (!cs->over && safety++<64) {
        int pi, gi;
        if (!monsters_alive(cs)) {
            log_msg(cs, "Victory!");
            cs->result=CR_VICTORY;
            cs->over=true;
            return true;
        }
        if (party_wiped(&gs->party)) {
            log_msg(cs, "PARTY DEFEATED!");
            cs->result=CR_DEFEAT;
            cs->over=true;
            return true;
        }
        if (cs->round<=0 || (all_party_checked(cs,&gs->party)&&all_groups_checked(cs))) {
            if (cs->round>0) {
                combat_apply_condition_damage(cs, gs);
                if (party_wiped(&gs->party)) {
                    log_msg(cs, "PARTY DEFEATED!");
                    cs->result=CR_DEFEAT;
                    cs->over=true;
                    return true;
                }
            }
            begin_round_flow(cs, gs, m);
        }

        pi=choose_next_party(cs,&gs->party);
        gi=choose_next_group(cs);
        if (pi>=0 && (gi<0 || gs->party.members[pi].stats[STAT_SPD] >= cs->groups[gi].type->level*3)) {
            cs->current_char=pi;
            return false;
        }
        if (gi>=0) {
            cs->checked_group[gi]=1;
            monster_group_turn(cs,&gs->party,gi);
            continue;
        }
        if (pi>=0) {
            cs->current_char=pi;
            return false;
        }
        begin_round_flow(cs, gs, m);
    }
    return cs->over;
}

void combat_start_flow(CombatSession *cs, GameState *gs, const struct Map *m)
{
    if (!cs || cs->over) return;
    cs->round=0;
    begin_round_flow(cs, gs, m);
    combat_advance_flow(cs, gs, m);
}

void combat_finish_player_action(CombatSession *cs, GameState *gs, const struct Map *m)
{
    int ci;
    if (!cs || cs->over) return;
    ci=cs->current_char;
    if (ci>=0 && ci<MAX_PARTY) cs->checked_party[ci]=1;
    cs->current_char=-1;
    cs->pending_action=COMBAT_ACT_NONE;
    combat_advance_flow(cs, gs, m);
}

bool combat_action_fight(CombatSession *cs, GameState *gs, const struct Map *m, int target, bool shoot)
{
    int ci, gi;
    if (!cs || cs->over) return true;
    ci=cs->current_char;
    if (ci<0 || ci>=gs->party.count) {
        combat_advance_flow(cs, gs, m);
        return cs->over;
    }
    gi = (target>=0&&target<cs->n_groups&&cs->groups[target].alive>0)
       ? target : first_alive_group(cs);
    cs->target_group=gi;
    if (gi<0) {
        combat_finish_player_action(cs, gs, m);
        return cs->over;
    }
    if (!shoot && !cs->can_attack[ci]) {
        log_fmt2(cs, gs->party.members[ci].name, "can't reach", 0);
        combat_finish_player_action(cs, gs, m);
        return cs->over;
    }
    if (!shoot && cs->group_range[gi]>0) {
        log_fmt2(cs, gs->party.members[ci].name, "out of range", 0);
        combat_finish_player_action(cs, gs, m);
        return cs->over;
    }
    character_attack_group(cs, &gs->party, ci, gi, shoot);
    if (!monsters_alive(cs)) {
        log_msg(cs, "Victory!");
        cs->result=CR_VICTORY;
        cs->over=true;
        return true;
    }
    combat_finish_player_action(cs, gs, m);
    return cs->over;
}

bool combat_action_block(CombatSession *cs, GameState *gs, const struct Map *m)
{
    int ci=cs?cs->current_char:-1;
    if (!cs || cs->over) return true;
    if (ci>=0 && ci<MAX_PARTY) {
        cs->defending[ci]=1;
        log_fmt2(cs, gs->party.members[ci].name, "blocks", 0);
    }
    combat_finish_player_action(cs, gs, m);
    return cs->over;
}

bool combat_action_protect(CombatSession *cs, GameState *gs, const struct Map *m)
{
    int ci=cs?cs->current_char:-1;
    if (!cs || cs->over) return true;
    if (ci>=0 && ci<MAX_PARTY) {
        cs->protecting[ci]=1;
        log_fmt2(cs, gs->party.members[ci].name, "protects", 0);
    }
    combat_finish_player_action(cs, gs, m);
    return cs->over;
}

bool combat_action_delay(CombatSession *cs, GameState *gs, const struct Map *m)
{
    int ci=cs?cs->current_char:-1;
    if (!cs || cs->over) return true;
    if (ci>=0 && ci<MAX_PARTY) log_fmt2(cs, gs->party.members[ci].name, "delays", 0);
    combat_finish_player_action(cs, gs, m);
    return cs->over;
}

bool combat_action_exchange(CombatSession *cs, GameState *gs, const struct Map *m)
{
    int ci=cs?cs->current_char:-1, ni, s;
    if (!cs || cs->over) return true;
    if (ci>=0 && ci<gs->party.count) {
        ni=(ci+1)%gs->party.count;
        for (s=0; s<gs->party.count; s++, ni=(ni+1)%gs->party.count)
            if (ni!=ci && is_alive(&gs->party.members[ni])) break;
        if (ni!=ci && ni<gs->party.count) {
            Character tmp=gs->party.members[ci];
            gs->party.members[ci]=gs->party.members[ni];
            gs->party.members[ni]=tmp;
            log_msg(cs, "Positions exchanged.");
        } else {
            log_msg(cs, "No one to exchange.");
        }
    }
    combat_finish_player_action(cs, gs, m);
    return cs->over;
}

/* ---- Main round ---- */
bool combat_round(CombatSession *cs, GameState *gs) {
    if (cs->over) return true;
    cs->round++;
    cs->n_hits_dealt = 0;
    if (cs->bless_rounds > 0) cs->bless_rounds--;
    { char buf[24]; buf[0]='R'; buf[1]='o'; buf[2]='u'; buf[3]='n'; buf[4]='d'; buf[5]=' ';
      buf[6]=(char)('0'+cs->round/10); buf[7]=(char)('0'+cs->round%10); buf[8]=':'; buf[9]='\0';
      if (cs->round<10) { buf[6]='R'; buf[7]='o'; buf[8]='u'; buf[9]='n'; buf[10]='d'; buf[11]=' ';
                          buf[12]=(char)('0'+cs->round); buf[13]=':'; buf[14]='\0'; }
      log_msg(cs, buf); }

    cs->pending_xp += party_attack(cs, &gs->party, gs->map_idx);
    if (!monsters_alive(cs)) {
        log_msg(cs, "Victory!"); cs->result=CR_VICTORY; cs->over=true; return true;
    }
    monsters_attack(cs, &gs->party);
    if (party_wiped(&gs->party)) {
        log_msg(cs, "PARTY DEFEATED!"); cs->result=CR_DEFEAT; cs->over=true; return true;
    }
    /* Condition damage: poisoned = 1 HP/round */
    {
      combat_apply_condition_damage(cs, gs);
      if (party_wiped(&gs->party)) {
          log_msg(cs,"PARTY DEFEATED!"); cs->result=CR_DEFEAT; cs->over=true; return true;
      }
    }
    if (cs->round >= 20) {
        log_msg(cs, "Combat ended."); cs->pending_xp/=2; cs->result=CR_VICTORY; cs->over=true; return true;
    }
    return false;
}

/* ---- Flee ---- */
bool combat_flee(CombatSession *cs, GameState *gs) {
    int spd=0, i;
    for (i=0; i<gs->party.count; i++) {
        if (is_alive(&gs->party.members[i]) && gs->party.members[i].stats[STAT_SPD]>spd)
            spd=gs->party.members[i].stats[STAT_SPD];
    }
    if (monster_roll(30) <= spd/3+2) {
        log_msg(cs,"Party flees!"); cs->result=CR_FLED; cs->over=true; return true;
    }
    log_msg(cs,"Can't escape!");
    /* Monsters get a free attack */
    monsters_attack(cs, &gs->party);
    if (party_wiped(&gs->party)) {
        log_msg(cs,"PARTY DEFEATED!");
        cs->result=CR_DEFEAT;
        cs->over=true;
        return true;
    }
    return false;
}

/* ---- Rewards ---- */
void combat_reward(CombatSession *cs, GameState *gs) {
    if (cs->result!=CR_VICTORY) return;
    if (cs->reward_applied) return;
    int eligible=0, i;
    for (i=0; i<gs->party.count; i++) if(is_alive(&gs->party.members[i])) eligible++;
    if (!eligible) return;
    int xp_share = cs->pending_xp / eligible;
    int gld_share = cs->pending_gold / eligible;
    int gem_share = cs->pending_gems / eligible;
    for (i=0; i<gs->party.count; i++) {
        Character *c=&gs->party.members[i];
        if (!is_alive(c)) continue;
        c->xp   += xp_share;
        c->gold += gld_share;
        c->gems += gem_share;
    }
    cs->reward_applied=1;
}

/* ---- Targeted round (uses cs->target_group) ---- */
bool combat_round_targeted(CombatSession *cs, GameState *gs, int target)
{
    cs->target_group = target;
    return combat_round(cs, gs);
}

/* ---- Loot after victory ---- */
int combat_loot(CombatSession *cs, GameState *gs)
{
    static const int LOOT_T1[] = { 183, 185, 186, 188, 178, 179, 184 };
    static const int LOOT_T2[] = { 12, 17, 18, 21, 23, 131, 132, 134 };
    static const int LOOT_T3[] = { 26, 29, 30, 33, 35, 139, 140, 144 };
    static const int LOOT_T4[] = { 38, 49, 53, 55, 59, 150, 152, 169 };
    static const int T1N=7,T2N=8,T3N=8,T4N=8;
    int g, avg=1, n=0, loot=cs->treasure_flags;
    for (g=0;g<cs->n_groups;g++) {
        avg+=cs->groups[g].type->level;
        loot |= cs->groups[g].type->loot;
        n++;
    }
    if (n>0) avg/=n;
    const int *tbl; int tbl_n;
    if      (avg<=2){tbl=LOOT_T1;tbl_n=T1N;}
    else if (avg<=4){tbl=LOOT_T2;tbl_n=T2N;}
    else if (avg<=6){tbl=LOOT_T3;tbl_n=T3N;}
    else            {tbl=LOOT_T4;tbl_n=T4N;}

    int roll=monster_roll(100);
    int drops=0;
    if (loot & 0x40) drops=2;
    else if (loot & 0x3E) drops=(roll<=45)?1:0;
    else drops=(roll<=15)?1:0;
    if ((loot & 0x80) && monster_roll(100)<=20) drops++;
    if (drops>3) drops=3;
    cs->treasure_container = (loot & 0x7E) ? ((monster_roll(100)<=50)?1:2) : 0;
    cs->treasure_trap = cs->treasure_container ? ((avg+monster_roll(8))&7) : 0;
    cs->n_found_items=0;
    int d;
    for (d=0;d<drops;d++) {
        int item_id=tbl[monster_roll(tbl_n)-1];
        int i,s;
        for (i=0;i<gs->party.count;i++){
            Character *c=&gs->party.members[(d+i)%gs->party.count];
            if (!is_alive(c)) continue;
            for (s=0;s<6;s++) {
                if (c->backpack[s]==0) {
                    c->backpack[s]=item_id;
                    if (cs->n_found_items<4)
                        cs->found_items[cs->n_found_items++]=item_id;
                    goto next_drop;
                }
            }
        }
        next_drop:;
    }
    return cs->n_found_items;
}
