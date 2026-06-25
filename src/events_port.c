/* events_port.c -- Tile event dispatch for HamsterOS port.
 * Stripped of SDL_Renderer and full services/combat — messages go to
 * player_set_message(); services/combat are stubs for future porting. */

#include "events_port.h"
#include "mazedata.h"
#include "mm_util.h"
#include <stddef.h>

extern int monster_roll(int sides);  /* from monsters.c */

#define strcasestr(h,n) mm_strcasestr((h),(n))
#define strlen(s)       mm_strlen(s)

/* ---- Town / dungeon tables ---- */
const int TOWN_DUNGEON[NUM_TOWN_DUNGEONS][2] = {
    { 0, 5 }, { 1, 7 }, { 2, 6 }, { 3, 9 }, { 4, 8 },
};
const int TOWN_EXIT[NUM_TOWN_EXITS][2] = {
    { 0, 23 }, { 1, 30 }, { 2, 19 }, { 3, 18 }, { 4, 28 },
};

static int party_gold(const GameState *gs)
{
    int i, total = gs->party.shared_gold;
    for(i=0;i<gs->party.count;i++) total += gs->party.members[i].gold;
    return total;
}

static void party_spend(GameState *gs, int amount)
{
    int i;
    if(amount<=0) return;
    if(gs->party.shared_gold>0){
        int take = gs->party.shared_gold < amount ? gs->party.shared_gold : amount;
        gs->party.shared_gold -= take;
        amount -= take;
    }
    for(i=0;i<gs->party.count && amount>0;i++){
        int take = gs->party.members[i].gold < amount ? gs->party.members[i].gold : amount;
        gs->party.members[i].gold -= take;
        amount -= take;
    }
}

static int count_bad_conditions(uint8_t cond)
{
    int n=0;
    if(IS_DEADLIKE(cond)) return 0;
    if(cond & COND_ASLEEP) n++;
    if(cond & COND_BLINDED) n++;
    if(cond & COND_SILENCED) n++;
    if(cond & COND_DISEASED) n++;
    if(cond & COND_POISONED) n++;
    if(cond & COND_PARALYZED) n++;
    if(cond & COND_UNCONSCIOUS) n++;
    return n;
}

static int sp_gain_for_level(const Character *c, int next_level)
{
    int cls=c->cls, sp_stat=0, sp_factor;
    if(cls==4||cls==2) sp_stat=c->stats_base[2];      /* Cleric/Paladin: PER */
    else if(cls==5||cls==3) sp_stat=c->stats_base[0]; /* Sorcerer/Archer: INT */
    else return 0;
    if((cls==2||cls==3)&&next_level<7) return 0;
    sp_factor=(sp_stat>=40)?10:(sp_stat>=35)?9:(sp_stat>=30)?8:(sp_stat>=25)?7:
              (sp_stat>=20)?6:(sp_stat>=16)?5:(sp_stat>=13)?4:(sp_stat>=10)?3:
              (sp_stat>=7)?2:(sp_stat>=5)?1:0;
    return sp_factor;
}

static const char* map_to_name(int m) {
    const char* n = map_ovr_name(m);
    return (n && *n) ? n : "sorpigal";
}

/* ---- Manual tile event table (tiles_all.inc) ---- */
typedef struct {
    const char* map_name;
    int x, y, dir;
    ServiceType service;
    const char* text;
} ManualTileEvent;

static const ManualTileEvent MANUAL_TILE_EVENTS[] = {
    { "sorpigal", 12, 0, 2, SVC_EXIT, "A PASSAGE LEADS OUTSIDE, TAKE IT (Y/N)?" },
    { "cave1", 15, 15, -1, SVC_MESSAGE, "A SIGN ABOVE THE DOOR READS: DON'T TURN AROUND!" },
    { "udrag1", 15, 15, 0, SVC_MESSAGE, "A SIGN ON THE DOOR READS: THE WALL IS PAINTED FROM CEILING TO FLOOR IN A BLACK AND WHITE CHECKERED PATTERN!" },
#include "tiles_all.inc"
};

static char s_display_text[512];

static int incomplete_header_text(const char* text)
{
    int len;
    if (!text || !*text) return 0;
    len = (int)strlen(text);
    if (len <= 0) return 0;
    return (strcasestr(text, "A SIGN ABOVE THE DOOR READS:") ||
            strcasestr(text, "A SIGN ON THE DOOR READS:")) &&
           text[len - 1] == ':';
}

static int quoted_sign_body(const char* text)
{
    if (!text || !*text) return 0;
    return text[0] == '"';
}

static const char* display_text_for_event(ServiceType svc, const char* text)
{
    if (!text || !*text) return NULL;

    if (svc == SVC_MESSAGE && quoted_sign_body(text)) {
        mm_strncpy(s_display_text, "A SIGN ABOVE THE DOOR READS:\n", sizeof(s_display_text));
        {
            int pos = (int)strlen(s_display_text);
            int i = 0;
            while (text[i] && pos < (int)sizeof(s_display_text) - 1)
                s_display_text[pos++] = text[i++];
            s_display_text[pos] = '\0';
        }
        return s_display_text;
    }

    return text;
}

static ServiceType manual_tile_service(int map_idx, int x, int y, int facing,
                                        const char** out_text)
{
    const char* map_name = map_to_name(map_idx);
    int i;
    for (i = 0; MANUAL_TILE_EVENTS[i].map_name; i++) {
        const ManualTileEvent* e = &MANUAL_TILE_EVENTS[i];
        if (e->x == x && e->y == y &&
            mm_namecmp(e->map_name, map_name) == 0 &&
            (e->dir == -1 || e->dir == facing)) {
            if (out_text) *out_text = e->text;
            return e->service;
        }
    }
    if (out_text) *out_text = NULL;
    return SVC_NONE;
}

static int town_named_in_text(const char* t) {
    static const struct { const char *w; int m; } TOWNS[] = {
        {"SORPIGAL",0},{"PORTSMITH",1},{"PORTSMIT",1},
        {"ALGARY",2},{"DUSK",3},{"ERLIQUIN",4},{NULL,0}
    };
    if (!t) return -1;
    int i;
    for (i=0; TOWNS[i].w; i++)
        if (strcasestr(t, TOWNS[i].w)) return TOWNS[i].m;
    return -1;
}

static int is_town_service(ServiceType s) {
    return (s==SVC_INN||s==SVC_TEMPLE||s==SVC_TRAINING||
            s==SVC_BLACKSMITH||s==SVC_FOOD||s==SVC_TAVERN);
}

/* ---- detect_service ---- */
ServiceType detect_service(const char* t)
{
    if (!t || !*t) return SVC_NONE;
    if (strcasestr(t,"STAIRS GOING DOWN")||(strcasestr(t,"STAIR")&&strcasestr(t,"DOWN")))
        return SVC_STAIRS;
    if (strcasestr(t,"STAIRS GOING UP")||strcasestr(t,"LADDER UP")||
        strcasestr(t,"STAIRS TO SURFACE")||(strcasestr(t,"STAIR")&&strcasestr(t,"UP")))
        return SVC_STAIRS_UP;
    if (strcasestr(t,"STAIR")) return SVC_STAIRS;
    if (strcasestr(t,"LEADS OUTSIDE")||strcasestr(t,"PASSAGE OUTSIDE")||
        strcasestr(t,"LEADS OUTDOORS")||strcasestr(t,"EXIT CASTLE")||
        strcasestr(t,"EXIT (Y/N)")||
        ((strcasestr(t,"PASSAGE")||strcasestr(t,"LEADS TO"))&&town_named_in_text(t)>=0))
        return SVC_EXIT;
    if (strcasestr(t,"SIGN IN (Y/N)")||strcasestr(t,"SIGNING IN")||
        strcasestr(t,"INNKEEPER")||strcasestr(t,"INN OF")) return SVC_INN;
    if (strcasestr(t,"TEMPLE")||strcasestr(t,"ORNATELY ROBED")||
        strcasestr(t,"SEEK OUR HELP")) return SVC_TEMPLE;
    if (strcasestr(t,"TRAINING (Y/N)")||strcasestr(t,"WORKOUT (Y/N)")||
        strcasestr(t,"TRAINING HALL")) return SVC_TRAINING;
    if (strcasestr(t,"BROWSE (Y/N)")||strcasestr(t,"BLACKSMITH")||
        strcasestr(t,"ARMORY")||strcasestr(t,"WEAPON SHOP")) return SVC_BLACKSMITH;
    if (strcasestr(t,"HUNGRY (Y/N)")||strcasestr(t,"BUY SOME FOOD")||
        strcasestr(t,"PROVISIONER")||(strcasestr(t,"FOOD")&&strcasestr(t,"(Y/N)")))
        return SVC_FOOD;
    if (strcasestr(t,"BAR (Y/N)")||strcasestr(t,"TAVERN")||strcasestr(t,"PUB"))
        return SVC_TAVERN;
    if (strcasestr(t,"AN AMBUSH")||strcasestr(t,"ATTACK!")||
        strcasestr(t,"SURROUNDED")) return SVC_ENCOUNTER;
    if (strcasestr(t,"PIT TRAP")||strcasestr(t,"POISON GAS")||
        strcasestr(t,"STALACTITES FALL")||strcasestr(t,"POISON POOL")||
        strcasestr(t,"POOL OF ACID")) return SVC_FLOOR_TRAP;
    if (strcasestr(t,"A PIT!")||strcasestr(t,"TRAP DOOR")||
        strcasestr(t,"TRAP!")||strcasestr(t,"ZAP!")) return SVC_TRAP;
    if (strcasestr(t,"PORTAL")) return SVC_PORTAL;
    if (strcasestr(t,"POOF"))   return SVC_TELEPORT;
    if (strcasestr(t,"SHRINE")||strcasestr(t,"PRAY (Y/N)")||
        strcasestr(t,"DRINK (Y/N)")||strcasestr(t,"POOL OF")) return SVC_SHRINE;
    if (strcasestr(t,"PRISONER")||strcasestr(t,"SET FREE")) return SVC_PRISONER;
    if (strcasestr(t,"LEVER")||strcasestr(t,"PULL IT")||
        strcasestr(t,"PUSH IT")) return SVC_LEVER;
    return SVC_SPECIAL;
}

/* ---- peek_tile_event ---- */
ServiceType peek_tile_event(const GameState* gs, const char* ovr_text,
                             const char** out_text)
{
    if (out_text) *out_text = NULL;
    if (!gs) return SVC_NONE;
    const char* manual_text = NULL;
    ServiceType svc = manual_tile_service(gs->map_idx, gs->player.x,
                                          gs->player.y, gs->player.facing,
                                          &manual_text);
    const char* text = (svc!=SVC_NONE && manual_text && *manual_text)
                     ? manual_text : ovr_text;
    if (incomplete_header_text(text)) {
        if (out_text) *out_text = "";
        return SVC_NONE;
    }
    if (svc == SVC_NONE) {
        if (!text || !*text) return SVC_NONE;
        svc = detect_service(text);
        if (gs->map_idx >= 0 && gs->map_idx <= 4 && is_town_service(svc))
            return SVC_NONE;
    }
    if (out_text) *out_text = display_text_for_event(svc, text);
    return svc;
}

/* ---- Transition helpers ---- */
static int town_to_dungeon(int m) {
    int i;
    for (i=0;i<NUM_TOWN_DUNGEONS;i++) if(TOWN_DUNGEON[i][0]==m) return TOWN_DUNGEON[i][1];
    return -1;
}
static int dungeon_to_town(int m) {
    int i;
    for (i=0;i<NUM_TOWN_DUNGEONS;i++) if(TOWN_DUNGEON[i][1]==m) return TOWN_DUNGEON[i][0];
    return -1;
}
static int town_to_exit(int m) {
    int i;
    for (i=0;i<NUM_TOWN_EXITS;i++) if(TOWN_EXIT[i][0]==m) return TOWN_EXIT[i][1];
    return -1;
}
static int exit_to_town(int outdoor_map) {
    int i;
    for (i=0;i<NUM_TOWN_EXITS;i++) if(TOWN_EXIT[i][1]==outdoor_map) return TOWN_EXIT[i][0];
    return -1;
}

/* ---- handle_tile_event ---- */
int handle_tile_event(GameState *gs, const OvrFile *ovr, const char *ovr_text)
{
    const char* text = NULL;
    ServiceType svc = peek_tile_event(gs, ovr_text, &text);

    if (svc == SVC_NONE) {
        if (text && *text) player_set_message(&gs->player, text, 120);
        return gs->map_idx;
    }

    /* Show the text always */
    if (text && *text) player_set_message(&gs->player, text, 120);

    switch (svc) {
    case SVC_STAIRS: {
        int dest = town_to_dungeon(gs->map_idx);
        if (dest >= 0) {
            gs->map_idx = dest;
            player_init(&gs->player, 0, 0, 0);
        }
        break;
    }
    case SVC_STAIRS_UP: {
        int dest = dungeon_to_town(gs->map_idx);
        if (dest >= 0) {
            gs->map_idx = dest;
            player_init(&gs->player, 8, 8, 0);
        }
        break;
    }
    case SVC_EXIT: {
        int dest = town_to_exit(gs->map_idx);
        if (dest >= 0) {
            gs->map_idx = dest;
            player_init(&gs->player, 8, 8, 0);
        } else {
            int town = exit_to_town(gs->map_idx);
            if (town >= 0) {
                gs->map_idx = town;
                player_init(&gs->player, 8, 8, 0);
            }
        }
        break;
    }
    case SVC_INN: {
        player_set_message(&gs->player, "INN_CHECKIN", 1);
        break;
    }
    case SVC_TEMPLE: {
        static const int TOWN_RESURRECT_COST[5] = { 500, 1000, 800, 600, 1200 };
        static const int TOWN_RAISE_COST[5]     = { 200,  400, 300, 250,  500 };
        static const int TOWN_CURE_COST[5]      = {  50,  100,  75,  60,  120 };
        static const int TOWN_MINOR_COST[5]     = {  10,   20,  15,  12,   25 };
        int town = (gs->map_idx>=0&&gs->map_idx<=4) ? gs->map_idx : 0;
        int i, resurrect_cost=0, raise_cost=0, cure_cost=0, heal_cost=0, wounded=0;
        for(i=0;i<gs->party.count;i++){
            uint8_t cd=(uint8_t)gs->party.members[i].condition;
            if(cd==COND_ERADICATED) resurrect_cost += TOWN_RESURRECT_COST[town];
            else if(cd==COND_DEAD||cd==COND_STONED) raise_cost += TOWN_RAISE_COST[town];
            cure_cost += count_bad_conditions(cd) * TOWN_CURE_COST[town];
            if(!IS_DEADLIKE(cd) && gs->party.members[i].hp < gs->party.members[i].hp_max){
                wounded=1;
                heal_cost += TOWN_MINOR_COST[town];
            }
        }
        if(heal_cost==0 && wounded) heal_cost=TOWN_MINOR_COST[town];
        int cost = resurrect_cost ? resurrect_cost : (raise_cost ? raise_cost :
                   (cure_cost ? cure_cost : (wounded ? heal_cost : 0)));
        if(cost<=0){
            player_set_message(&gs->player,"PARTY IS IN GOOD HEALTH.",90); break;
        }
        if(party_gold(gs) >= cost){
            party_spend(gs,cost);
            for(i=0;i<gs->party.count;i++){
                Character *c2=&gs->party.members[i];
                if(resurrect_cost){
                    if(c2->condition==COND_ERADICATED){c2->condition=0;if(c2->hp<=0)c2->hp=1;}
                } else if(raise_cost){
                    if(c2->condition==COND_DEAD||c2->condition==COND_STONED){c2->condition=0;if(c2->hp<=0)c2->hp=1;}
                } else if(cure_cost){
                    if(!IS_DEADLIKE(c2->condition)) c2->condition=0;
                } else {
                    if(!IS_DEADLIKE(c2->condition)) c2->hp=c2->hp_max;
                }
            }
            const char *msg = resurrect_cost?"ERADICATED RESURRECTED!":
                              (raise_cost?"DEAD RAISED!":
                               (cure_cost?"CONDITIONS CURED.":"WOUNDS HEALED."));
            player_set_message(&gs->player,msg,120);
        } else {
            char nmsg[48];
            mm_snprintf(nmsg,sizeof(nmsg),"NEED %d GOLD FOR TEMPLE.",cost);
            player_set_message(&gs->player,nmsg,120);
        }
        break;
    }
    case SVC_FOOD: {
        static const int FOOD_PRICE[5] = { 5, 8, 8, 10, 10 };
        int town = (gs->map_idx>=0&&gs->map_idx<=4) ? gs->map_idx : 0;
        int cost = FOOD_PRICE[town], qty = 40;
        if (party_gold(gs) >= cost) {
            char msg[48];
            party_spend(gs,cost);
            gs->party.shared_food += qty;
            if (gs->party.shared_food > 999) gs->party.shared_food = 999;
            mm_snprintf(msg,sizeof(msg),"%d FOOD PURCHASED FOR %d GOLD.",qty,cost);
            player_set_message(&gs->player, msg, 120);
        } else {
            player_set_message(&gs->player, "NOT ENOUGH GOLD FOR FOOD.", 120);
        }
        break;
    }
    case SVC_TRAINING: {
        /* Auto-level has already applied gains; training hall confirms and charges */
        static const int XP_T[6][10]={
            {1500,3000,6000,12000,24000,48000,96000,192000,384000,768000},
            {2000,4000,8000,16000,32000,64000,128000,256000,512000,1024000},
            {2000,4000,8000,16000,32000,64000,128000,256000,512000,1024000},
            {2000,4000,8000,16000,32000,64000,128000,256000,512000,1024000},
            {2500,5000,10000,20000,40000,80000,160000,320000,640000,1280000},
            {1000,2000,4000,8000,16000,32000,64000,128000,256000,512000}
        };
        static const int HP_PER_LEVEL[7] = {0,12,10,8,8,6,8};
        int i, trainable[MAX_PARTY], n_trainable=0, total_cost=0;
        for (i=0;i<gs->party.count;i++) {
            Character *c=&gs->party.members[i];
            if (IS_DEADLIKE(c->condition)||c->level>=10) continue;
            int cls=c->cls; if(cls<1||cls>6) continue;
            int needed=XP_T[cls-1][c->level-1];
            if (c->xp < needed) continue;
            trainable[n_trainable++]=i;
            total_cost += c->level * 1000;
        }
        if(n_trainable<=0){
            player_set_message(&gs->player,"NO CHARACTERS READY FOR TRAINING.",120);
        } else if(party_gold(gs) < total_cost){
            char msg[48];
            mm_snprintf(msg,sizeof(msg),"NEED %d GOLD FOR TRAINING.",total_cost);
            player_set_message(&gs->player,msg,120);
        } else {
            char msg[48];
            party_spend(gs,total_cost);
            for(i=0;i<n_trainable;i++){
                Character *c=&gs->party.members[trainable[i]];
                int next_level=c->level+1;
                c->level=next_level;
                c->hp_max += HP_PER_LEVEL[c->cls];
                c->hp = c->hp_max;
                c->sp_max += sp_gain_for_level(c,next_level);
                c->sp = c->sp_max;
                c->stats_base[3]++;
                c->stats[3]=c->stats_base[3]+c->equip_bonus[3];
            }
            mm_snprintf(msg,sizeof(msg),"%d MEMBER(S) ADVANCED!",n_trainable);
            player_set_message(&gs->player,msg,120);
        }
        break;
    }
    case SVC_BLACKSMITH:
        /* Signal to mm_port.c that shop should open — set a special message */
        player_set_message(&gs->player, "BROWSE_SHOP", 1);
        break;
    case SVC_TAVERN: {
        static int drink_ctr = 0;
        static const char *SNM[7]={"INT","MIG","PER","END","SPD","ACC","LCK"};
        int i;
        /* Build combined message: rumour + drink */
        char full_msg[400]; int fmi=0;
        const char *rumour = NULL;
        if(ovr && ovr->num_strings > 0){
            int si;
            for(si=0;si<ovr->num_strings;si++)
                if((int)mm_strlen(ovr->strings[si])>=12){rumour=ovr->strings[si];break;}
        }
        const char *r_use = rumour ? rumour : "BARKEEP: No news from the road.";
        while(*r_use && fmi<280) full_msg[fmi++]=(char)*r_use++;
        /* Drink: 5g per round, rotating +1 stat */
        if(gs->party.count>0 && (int)gs->party.members[0].gold >= 5){
            int si2=drink_ctr%7; drink_ctr++;
            for(i=0;i<gs->party.count;i++){
                Character *c2=&gs->party.members[i];
                if(!IS_DEADLIKE(c2->condition)&&c2->stats[si2]<25) c2->stats[si2]++;
            }
            gs->party.members[0].gold-=5;
            const char *dp="  DRINKS: +1 "; while(*dp&&fmi<360) full_msg[fmi++]=(char)*dp++;
            const char *sn=SNM[si2]; while(*sn&&fmi<364) full_msg[fmi++]=(char)*sn++;
            full_msg[fmi++]='!'; full_msg[fmi++]=' '; full_msg[fmi++]='('; full_msg[fmi++]='5'; full_msg[fmi++]='g'; full_msg[fmi++]=')';
        }
        full_msg[fmi]='\0';
        player_set_message(&gs->player, full_msg, 180);
        break;
    }
    case SVC_ENCOUNTER:
        /* mm_port.c handles actual combat — just set a message here */
        player_set_message(&gs->player, "MONSTERS APPROACH!", 60);
        break;
    case SVC_TRAP: {
        /* Robber gets a disarm attempt */
        int robber_lvl=0, i;
        for(i=0;i<gs->party.count;i++){
            Character *c2=&gs->party.members[i];
            if(c2->cls==6&&!IS_DEADLIKE(c2->condition)&&c2->level>robber_lvl)
                robber_lvl=c2->level;
        }
        if(robber_lvl>0 && monster_roll(20)+robber_lvl >= 14){
            player_set_message(&gs->player,"ROBBER DISARMS THE TRAP!",120);
        } else {
            int dmg=monster_roll(8);
            for(i=0;i<gs->party.count;i++){
                Character *c2=&gs->party.members[i];
                if(!IS_DEADLIKE(c2->condition)){c2->hp-=dmg;if(c2->hp<0)c2->hp=0;}
            }
            char tmsg[40]; int ti=0;
            const char *tp="TRAP! -"; while(*tp) tmsg[ti++]=(char)*tp++;
            if(dmg>=10) tmsg[ti++]=(char)('0'+dmg/10);
            tmsg[ti++]=(char)('0'+dmg%10);
            const char *th=" HP damage!"; while(*th) tmsg[ti++]=(char)*th++;
            tmsg[ti]='\0';
            player_set_message(&gs->player,tmsg,120);
        }
        break;
    }
    case SVC_SHRINE: {
        static const char *SKW[7]={"INT","MIG","PER","END","SPD","ACC","LCK"};
        static const char *SKW2[7]={"INTELLIGENCE","MIGHT","PERSONALITY","ENDURANCE","SPEED","ACCURACY","LUCK"};
        const char *st = text ? text : "";
        int i, stat_idx=-1, applied=0;
        for(i=0;i<7&&stat_idx<0;i++)
            if(strcasestr(st,SKW[i])||strcasestr(st,SKW2[i])) stat_idx=i;
        if(stat_idx>=0){
            for(i=0;i<gs->party.count;i++){
                Character *c2=&gs->party.members[i];
                if(!IS_DEADLIKE(c2->condition)&&c2->stats[stat_idx]<25) c2->stats[stat_idx]++;
            }
            char msg[32]; int mi=0;
            const char *mp="BLESSED: +1 "; while(*mp) msg[mi++]=(char)*mp++;
            const char *sn=SKW[stat_idx]; while(*sn) msg[mi++]=(char)*sn++;
            msg[mi++]='!'; msg[mi]='\0';
            player_set_message(&gs->player,msg,120); applied=1;
        } else if(strcasestr(st,"GEM")||strcasestr(st,"TREASURE")){
            int gems=monster_roll(3);
            for(i=0;i<gs->party.count;i++)
                if(!IS_DEADLIKE(gs->party.members[i].condition)) gs->party.members[i].gems+=gems;
            player_set_message(&gs->player,"SHRINE GRANTS GEMS!",120); applied=1;
        } else if(strcasestr(st,"HEAL")||strcasestr(st,"HEALTH")){
            for(i=0;i<gs->party.count;i++){
                Character *c2=&gs->party.members[i];
                if(!IS_DEADLIKE(c2->condition)){c2->hp+=20;if(c2->hp>c2->hp_max)c2->hp=c2->hp_max;}
            }
            player_set_message(&gs->player,"SHRINE RESTORES HEALTH!",120); applied=1;
        } else if(strcasestr(st,"CURE")||strcasestr(st,"CLEANSE")||strcasestr(st,"PURIFY")){
            for(i=0;i<gs->party.count;i++)
                if(!IS_DEADLIKE(gs->party.members[i].condition)) gs->party.members[i].condition=0;
            player_set_message(&gs->player,"SHRINE CURES CONDITIONS!",120); applied=1;
        }
        if(!applied) player_set_message(&gs->player,"You feel blessed by a holy presence.",120);
        break;
    }
    default:
        break;
    }

    return gs->map_idx;
}

/* ---- handle_floor_trap ---- */
void handle_floor_trap(GameState *gs, const OvrFile *ovr)
{
    const char* text = ovr_text_at(ovr, gs->player.x, gs->player.y,
                                    gs->player.facing);
    if (!text || !*text) return;
    ServiceType svc = detect_service(text);
    if (svc != SVC_FLOOR_TRAP) return;
    player_set_message(&gs->player, text, 120);

    int i, dmg=0;
    if(strcasestr(text,"PIT TRAP")||strcasestr(text,"TRAP DOOR")){
        /* Levitate bypasses pit traps */
        if(gs->party.protections[PROT_LEVITATE]>0) return;
        dmg=monster_roll(10);
        for(i=0;i<gs->party.count;i++){
            Character *c2=&gs->party.members[i];
            if(!IS_DEADLIKE(c2->condition)){c2->hp-=dmg;if(c2->hp<0)c2->hp=0;}
        }
    } else if(strcasestr(text,"POISON GAS")||strcasestr(text,"POISON POOL")){
        for(i=0;i<gs->party.count;i++)
            if(!IS_DEADLIKE(gs->party.members[i].condition))
                gs->party.members[i].condition|=COND_POISONED;
    } else if(strcasestr(text,"ACID")){
        dmg=monster_roll(6);
        for(i=0;i<gs->party.count;i++){
            Character *c2=&gs->party.members[i];
            if(!IS_DEADLIKE(c2->condition)){
                c2->hp-=dmg; if(c2->hp<0)c2->hp=0;
                c2->condition|=COND_POISONED;
            }
        }
    } else if(strcasestr(text,"STALACTITE")){
        dmg=monster_roll(6);
        for(i=0;i<gs->party.count;i++){
            Character *c2=&gs->party.members[i];
            if(!IS_DEADLIKE(c2->condition)){c2->hp-=dmg;if(c2->hp<0)c2->hp=0;}
        }
    } else {
        dmg=monster_roll(8);
        for(i=0;i<gs->party.count;i++){
            Character *c2=&gs->party.members[i];
            if(!IS_DEADLIKE(c2->condition)){c2->hp-=dmg;if(c2->hp<0)c2->hp=0;}
        }
    }
    (void)dmg;
}

/* ---- area_edge_transition ---- */
int area_edge_transition(const GameState *gs, const struct Map *m,
                          int *new_x, int *new_y)
{
    int mi = gs->map_idx;
    if (mi < 14 || mi > 33) return -1;

    int row = (mi - 14) / 4;
    int col = (mi - 14) % 4;
    int px  = gs->player.x, py = gs->player.y;

    *new_x = px; *new_y = py;
    (void)m;

    if (py >= MAP_SIZE - 1 && row > 0) {  /* North edge -> row above */
        *new_y = 0;
        return 14 + (row-1)*4 + col;
    }
    if (py <= 0 && row < 4) {             /* South edge -> row below */
        *new_y = MAP_SIZE - 1;
        return 14 + (row+1)*4 + col;
    }
    if (px >= MAP_SIZE - 1 && col < 3) {  /* East edge -> next col */
        *new_x = 0;
        return 14 + row*4 + (col+1);
    }
    if (px <= 0 && col > 0) {             /* West edge -> prev col */
        *new_x = MAP_SIZE - 1;
        return 14 + row*4 + (col-1);
    }
    return -1;
}

/* ---- tile_service_at (for minimap) ---- */
ServiceType tile_service_at(int map_idx, int x, int y)
{
    const char* map_name = map_to_name(map_idx);
    int i;
    for (i = 0; MANUAL_TILE_EVENTS[i].map_name; i++) {
        const ManualTileEvent* e = &MANUAL_TILE_EVENTS[i];
        if (e->x == x && e->y == y &&
            mm_namecmp(e->map_name, map_name) == 0)
            return e->service;
    }
    return SVC_NONE;
}

#undef strcasestr
#undef strlen
