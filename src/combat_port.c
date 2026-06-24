/* combat_port.c — Turn-based combat for HamsterOS port.
 * Ported from combat.c (MM_C_Port). No SDL — updates CombatSession state,
 * sets player messages. mm_port.c owns the state machine and draws the screen. */

#include "combat_port.h"
#include "items.h"
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
    for (i=0; i<mg->type->max_group; i++) if (mg->hp[i]>0) cands[nc++]=i;
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

/* ---- Init ---- */
void combat_init(CombatSession *cs, GameState *gs) {
    mm_memset(cs, 0, sizeof(*cs));
    monster_rng_seed((unsigned)pit_millis());
    int max_lv = 1, max_qty = 3;
    if (gs->map_idx >= 5) { max_lv=3; max_qty=4; }
    if (gs->map_idx >= 14) { max_lv=5; max_qty=5; }
    if (gs->map_idx >= 34) { max_lv=8; max_qty=6; }
    encounter_generate(max_lv, max_qty, cs->groups, &cs->n_groups);
    cs->round = 0;
    cs->over  = false;
}

/* ---- Party attacks ---- */
static int party_attack(CombatSession *cs, Party *p, int map_idx) {
    int xp=0, ao, i;
    int gi = first_alive_group(cs);
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
                xp += mg->type->xp;
                cs->pending_gold += monster_roll(mg->type->level*5+1);
                { char buf[40]; int bi=0;
                  while (mg->type->name[bi] && bi<20) buf[bi]=mg->type->name[bi++];
                  buf[bi++]=' '; buf[bi++]='s'; buf[bi++]='l'; buf[bi++]='a'; buf[bi++]='i'; buf[bi++]='n'; buf[bi++]='!'; buf[bi]='\0';
                  log_msg(cs, buf); }
                gi = first_alive_group(cs);
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
            Character *c = &p->members[pi];
            int roll = monster_roll(20);
            int eff_ac = equipped_ac(c);
            int target = 10 + (eff_ac<10 ? 10-eff_ac : 0);
            if (roll+mg->type->level+2 < target) {
                log_fmt2(cs, mg->type->name, "misses", 0); continue;
            }
            int dmg = monster_roll(mg->type->max_dmg);
            if (dmg<1) dmg=1;
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
                default: break;
                }
                if (t->hp<=0 && !IS_DEADLIKE(t->condition)) { t->hp=0; t->condition=COND_DEAD; }
            }
        }
    }
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
    return false;
}

/* ---- Rewards ---- */
void combat_reward(CombatSession *cs, GameState *gs) {
    if (cs->result!=CR_VICTORY) return;
    int eligible=0, i;
    for (i=0; i<gs->party.count; i++) if(is_alive(&gs->party.members[i])) eligible++;
    if (!eligible) return;
    int xp_share = cs->pending_xp / eligible;
    int gld_share = cs->pending_gold / eligible;
    for (i=0; i<gs->party.count; i++) {
        Character *c=&gs->party.members[i];
        if (!is_alive(c)) continue;
        c->xp   += xp_share;
        c->gold += gld_share;
    }
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
    int g, avg=1, n=0;
    for (g=0;g<cs->n_groups;g++) { avg+=cs->groups[g].type->level; n++; }
    if (n>0) avg/=n;
    const int *tbl; int tbl_n;
    if      (avg<=2){tbl=LOOT_T1;tbl_n=T1N;}
    else if (avg<=4){tbl=LOOT_T2;tbl_n=T2N;}
    else if (avg<=6){tbl=LOOT_T3;tbl_n=T3N;}
    else            {tbl=LOOT_T4;tbl_n=T4N;}

    int roll=monster_roll(100);
    int drops=(roll<=10)?2:(roll<=40)?1:0;
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