/* mm_port.c — HamsterOS coordinator for MM_HamsterOS_Port
 * 640x440 render buffer @ scale=1 blit (8px font on screen).
 * 3D view is rendered into a 240x132 sub-buffer, blitted separately
 * at scale=2 to give a large 480x264 apparent viewport. */

#include "app_abi.h"
#include "window.h"
#include "mm_util.h"
#include "mazedata.h"
#include "game.h"
#include "wallpix.h"
#include "render3d.h"
#include "hud_port.h"
#include "font.h"
#include "ovr.h"
#include "events_port.h"
#include "roster_port.h"
#include "combat_port.h"
#include "screen_port.h"
#include "monpix_port.h"
#include "mm_music.h"
#include "items.h"
#include "spells.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ---- Stubs ---- */
extern void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c);
extern void draw_text_bold(int16_t x, int16_t y, const char *s, uint8_t c, uint8_t sc);
extern void draw_border(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c);
extern void blit_opaque_pixels(int16_t dx, int16_t dy, const uint8_t *pix,
                                uint16_t stride, uint16_t sx, uint16_t sy,
                                uint16_t sw, uint16_t sh, uint8_t scale);
extern void present_region(int16_t x, int16_t y, int16_t w, int16_t h);
extern void wnd_init(WindowFrame *f, int16_t x, int16_t y, int16_t w, int16_t h,
                     int16_t mw, int16_t mh);
extern void wnd_draw_frame(const WindowFrame *f, const char *title);
extern void wnd_content_rect(const WindowFrame *f, int16_t inset,
                              int16_t *x, int16_t *y, int16_t *w, int16_t *h);
extern void wnd_display_bounds(const WindowFrame *f,
                               int16_t *x, int16_t *y, int16_t *w, int16_t *h);
extern WindowHit wnd_hit_test(const WindowFrame *f, int16_t x, int16_t y);
extern void wnd_begin_drag(WindowFrame *f, int16_t x, int16_t y);
extern void wnd_begin_resize(WindowFrame *f, int16_t x, int16_t y);
extern bool wnd_update_pointer(WindowFrame *f, int16_t x, int16_t y);
extern void wnd_end_interaction(WindowFrame *f);
extern bool fat_load_file(FatDrive drive, uint32_t dir, const char *name,
                           char *buf, uint32_t max, uint32_t *out_size, bool *trunc);
extern bool fat_save_file(FatDrive drive, uint32_t dir, const char *name,
                           const char *buf, uint32_t size);
extern bool mm_fat_list_dir(FatDrive drive, uint32_t dir, FatRootEntry *entries,
                             uint32_t max, uint32_t start, uint32_t *count,
                             bool *more, const char *ext_filter);
extern void *heap_alloc(uint32_t size);
extern void  heap_free(void *ptr);
extern void serial_str(const char *s);
extern void serial_dec16(int16_t v);
extern uint32_t pit_millis(void);
extern void speaker_note_on(uint16_t hz);
extern void speaker_note_off(void);
extern void sb16_stop(void);  /* called in close to stop any residual audio */

/* Embedded binary data (generated from Original_Source/) */
extern const uint8_t  wallpix_dta_data[];
extern const uint32_t wallpix_dta_size;
extern const uint8_t  monpix_dta_data[];
extern const uint32_t monpix_dta_size;
extern const uint8_t  mazedata_dta_data[];
extern const uint32_t mazedata_dta_size;
extern const uint8_t  screen_dta_data[];
extern const uint32_t screen_dta_offsets[10];
extern const uint32_t screen_dta_sizes[10];
extern void set_game_mode(bool g);

/* ---- Music / SFX state (uses authentic MM1 sequences from mm_music.h) ---- */
/* Background music sequencer */
static const MusicNote *g_mus_seq  = NULL;
static int              g_mus_len  = 0;
static int              g_mus_idx  = 0;
static uint32_t         g_mus_next = 0;
/* One-shot SFX sequencer (takes priority over music) */
static const MusicNote *g_sfx_seq  = NULL;
static int              g_sfx_len  = 0;
static int              g_sfx_idx  = 0;
static uint32_t         g_sfx_next = 0;

static void music_start(const MusicNote *seq, int len) {
    g_mus_seq = seq; g_mus_len = len; g_mus_idx = 0; g_mus_next = 0;
}
static void sfx_play(const MusicNote *seq, int len) {
    g_sfx_seq = seq; g_sfx_len = len; g_sfx_idx = 0; g_sfx_next = pit_millis();
}

/* ---- Window (game_mode hides header bar, so window starts at y=0) ---- */
#define WIN_X     0
#define WIN_Y     0
#define WIN_W     640
#define WIN_H     480
#define WIN_MIN_W WIN_W
#define WIN_MIN_H WIN_H

/* ---- Save state format ---- */
typedef struct {
    uint32_t magic;          /* 0x4D4D3153 'MM1S' */
    uint8_t  map_idx;
    uint8_t  player_x;
    uint8_t  player_y;
    uint8_t  player_facing;
} SaveState;
#define SAVE_MAGIC 0x4D4D3153u

/* ---- Save slot filenames (3 slots) ---- */
static const char * const SAVE_FNAMES[3]  = {"SAVE1.DAT","SAVE2.DAT","SAVE3.DAT"};
static const char * const ROSTR_FNAMES[3] = {"ROSTR1.DTA","ROSTR2.DTA","ROSTR3.DTA"};
typedef struct { bool exists; uint8_t map_idx; } SaveSlotInfo;

/* ---- Game mode ---- */
typedef enum {
    GM_TITLE=0, GM_TOWN_SELECT, GM_EXPLORE, GM_COMBAT,
    GM_OPTIONS, GM_QUIT_CONFIRM, GM_DEFEAT, GM_CHARSHEET,
    GM_SHOP, GM_MAP, GM_SPELL_SELECT, GM_COMBAT_ITEM,
    GM_SAVE_SLOT, GM_CHAR_CREATE, GM_PRE_COMBAT, GM_SAGE
} GameMode;

/* ---- Static state ---- */
static WindowFrame  g_frame;
static bool         g_open, g_active, g_needs_redraw;
static GameMode     g_mode;
static GameMode     g_prev_mode;   /* mode before options / quit confirm */
static int          g_town_cursor;
static bool         g_music_en = true;
static bool         g_sfx_en   = true;

/* Render buffers */
static uint8_t      g_render_buf[RENDER_W * RENDER_H];
static uint8_t      g_view3d_buf[VIEW3D_W * VIEW3D_H];

/* Character sheet state */
static int          g_charsheet_idx;
/* Shop state */
static int          g_shop_scroll;
static int          g_shop_town;
/* Spell select state */
static int          g_spell_ids[20];
static int          g_spell_count;
static int          g_spell_scroll;
/* Combat item select state */
static int          g_citem_ids[9];
static int          g_citem_char[9];
static int          g_citem_slot[9];
static int          g_citem_count;
/* Starvation */
static int          g_hunger_steps;

/* Game data */
static struct Map   g_maps[NUM_MAPS];
static int          g_maps_loaded;
static bool         g_wallpix_loaded;

static GameState    g_gs;
static bool         g_game_ready;
static bool         g_save_exists;

static OvrFile      g_ovr;
static bool         g_ovr_loaded;
static uint32_t     g_mm_cluster;

/* Title */
static int          g_title_idx;
static uint32_t     g_title_next_change;

/* Save slot state */
static int          g_save_slot_mode;    /* 0=saving 1=loading */
static int          g_save_slot_cursor;
static SaveSlotInfo g_slot_info[3];

/* Character creation state */
static int          g_cc_phase;          /* 0=class 1=race 2=stats */
static int          g_cc_cls, g_cc_race;
static int          g_cc_stats[7];
static int          g_cc_hp, g_cc_sp;
static int          g_cc_member_idx;
static Party        g_cc_party;
/* Sage/help page */
static int          g_sage_page;

/* Monster sprites */
static uint8_t     *g_monpix_buf;
static bool         g_monpix_loaded;
static uint8_t      g_mon_sprite[MONPIX_RAW_SIZE];

/* Combat */
static CombatSession g_combat;

/* ---- Forward declarations ---- */
static void mm_on_map_change(int map_idx);
static void mm_apply_equip_bonuses(Party *p);

/* ---- FAT helpers ---- */
static bool mm_find_subdir(FatDrive drive, uint32_t parent,
                            const char *name, uint32_t *out)
{
    FatRootEntry entries[32]; uint32_t count=0,i; bool more=false;
    if (!mm_fat_list_dir(drive,parent,entries,32,0,&count,&more,NULL)) return false;
    for (i=0;i<count;i++)
        if (entries[i].is_dir && mm_namecmp(entries[i].name,name)==0)
            { *out=entries[i].first_cluster; return true; }
    return false;
}

static void mm_load_ovr_for_map(int map_idx)
{
    const char *base=map_ovr_name(map_idx);
    if(!base||!*base){g_ovr_loaded=false;return;}
    char fname[16]; int i=0;
    while(base[i]&&i<10){fname[i]=(char)mm_toupper(base[i]);i++;}
    fname[i++]='.';fname[i++]='O';fname[i++]='V';fname[i++]='R';fname[i]='\0';
    uint8_t *buf=(uint8_t*)heap_alloc(4096); if(!buf){g_ovr_loaded=false;return;}
    uint32_t sz=0; bool trunc=false;
    if(fat_load_file(FAT_DRIVE_B,g_mm_cluster,fname,(char*)buf,4096,&sz,&trunc)){
        ovr_load_buf(&g_ovr,buf,sz);g_ovr_loaded=true;
    } else {g_ovr_loaded=false;mm_memset(&g_ovr,0,sizeof(g_ovr));}
    heap_free(buf);
    mm_on_map_change(map_idx);  /* show map name on entry */
}

static void mm_load_title_screen(int idx)
{
    g_title_idx = idx;  /* screen data is embedded — no floppy read needed */
}

static void mm_load_game_data(void)
{
    uint32_t loaded_size=0; bool trunc=false;
    if(!mm_find_subdir(FAT_DRIVE_B,0,"MM1",&g_mm_cluster)){
        serial_str("MM: no MM1/ on B:\n"); return;
    }

    g_maps_loaded=mazedata_load(g_maps,mazedata_dta_data,mazedata_dta_size);
    serial_str("MM: maps=");serial_dec16((int16_t)g_maps_loaded);serial_str("\n");

    /* WALLPIX — embedded in app, no floppy read needed */
    if (wallpix_load(wallpix_dta_data, wallpix_dta_size) == 0) {
        g_wallpix_loaded = true;
        serial_str("MM: wallpix embedded ok\n");
    }

    /* Game state + party */
    if(g_maps_loaded>0){
        game_state_init(&g_gs,"","");
        g_gs.map_idx=0;
        uint8_t *rbuf=(uint8_t*)heap_alloc(4096);
        if(rbuf){
            uint32_t rsz=0;bool rt=false;
            if(fat_load_file(FAT_DRIVE_B,g_mm_cluster,"ROSTER.DTA",
                             (char*)rbuf,4096,&rsz,&rt))
                roster_load_buf(rbuf,rsz,&g_gs.party);
            heap_free(rbuf);
        }
        mm_apply_equip_bonuses(&g_gs.party);
        g_game_ready=true;
        mm_load_ovr_for_map(0);
    }

    /* Give party starting food (10 rations) if none loaded */
    if(g_gs.party.shared_food <= 0) g_gs.party.shared_food = 10;

    /* Check for save files across all 3 slots */
    {
        int ss2; g_save_exists=false;
        for(ss2=0;ss2<3;ss2++){
            uint8_t stest[8]; uint32_t ssz=0; bool st=false;
            if(fat_load_file(FAT_DRIVE_B,g_mm_cluster,SAVE_FNAMES[ss2],
                             (char*)stest,8,&ssz,&st) && ssz>=8)
                { g_save_exists=true; break; }
        }
    }

    /* MONPIX — embedded in app */
    if (monpix_load(monpix_dta_data, monpix_dta_size) == 0) {
        g_monpix_loaded = true;
        serial_str("MM: monpix embedded ok\n");
    }

    /* PC speaker music only */

    g_title_idx=0; g_title_next_change=0;
    mm_load_title_screen(0);
    g_mode=GM_TITLE;
    g_needs_redraw=true;
    serial_str("MM: ready\n");
}

/* ---- Save / Load (slot-based) ---- */
static void mm_build_roster_raw(uint8_t *raw)
{
    int i,s;
    mm_memset(raw,0,18*127+18);
    for(i=0;i<g_gs.party.count&&i<18;i++){
        Character *c=&g_gs.party.members[i];
        uint8_t *r=raw+i*127;
        mm_memcpy(r,c->name,15);
        r[0x13]=(uint8_t)c->race; r[0x14]=(uint8_t)c->cls;
        for(s=0;s<7;s++){r[0x15+s*2]=(uint8_t)c->stats[s];r[0x15+s*2+1]=(uint8_t)c->stats_base[s];}
        r[0x23]=(uint8_t)c->level;
        r[0x27]=(uint8_t)c->xp;r[0x28]=(uint8_t)(c->xp>>8);
        r[0x29]=(uint8_t)(c->xp>>16);r[0x2A]=(uint8_t)(c->xp>>24);
        r[0x33]=(uint8_t)c->hp;r[0x34]=(uint8_t)(c->hp>>8);
        r[0x37]=(uint8_t)c->hp_max;r[0x38]=(uint8_t)(c->hp_max>>8);
        r[0x39]=(uint8_t)c->gold;r[0x3A]=(uint8_t)(c->gold>>8);r[0x3B]=(uint8_t)(c->gold>>16);
        r[0x3C]=(uint8_t)c->ac;r[0x3E]=(uint8_t)c->food;r[0x3F]=(uint8_t)c->condition;
        for(s=0;s<6;s++){r[0x40+s]=(uint8_t)c->equipped[s];r[0x46+s]=(uint8_t)c->backpack[s];}
        mm_memcpy(r+0x70,c->char_quests,8);r[0x7B]=c->visit_flags;
        raw[18*127+i]=1;
    }
}

static void mm_save_game_slot(int slot)
{
    extern void fat_batch_begin(FatDrive d);
    extern void fat_batch_end(FatDrive d);
    if(slot<0||slot>2) slot=0;

    player_set_message(&g_gs.player,"Saving...",30);

    SaveState ss;
    ss.magic=SAVE_MAGIC;
    ss.map_idx=(uint8_t)g_gs.map_idx;
    ss.player_x=(uint8_t)g_gs.player.x;
    ss.player_y=(uint8_t)g_gs.player.y;
    ss.player_facing=(uint8_t)g_gs.player.facing;

    static uint8_t raw[18*127+18];
    mm_build_roster_raw(raw);

    fat_batch_begin(FAT_DRIVE_B);
    bool ok1=fat_save_file(FAT_DRIVE_B,g_mm_cluster,SAVE_FNAMES[slot],
                            (const char*)&ss,sizeof(ss));
    bool ok2=fat_save_file(FAT_DRIVE_B,g_mm_cluster,ROSTR_FNAMES[slot],
                            (const char*)raw,sizeof(raw));
    fat_batch_end(FAT_DRIVE_B);

    if(ok1&&ok2){
        g_save_exists=true;
        g_slot_info[slot].exists=true;
        g_slot_info[slot].map_idx=(uint8_t)g_gs.map_idx;
        player_set_message(&g_gs.player,"GAME SAVED.",90);
    } else {
        player_set_message(&g_gs.player,"SAVE FAILED - check disk.",90);
    }
}

static bool mm_load_game_slot(int slot)
{
    if(slot<0||slot>2) return false;
    SaveState ss; uint32_t ssz=0; bool st=false;
    if(!fat_load_file(FAT_DRIVE_B,g_mm_cluster,SAVE_FNAMES[slot],
                      (char*)&ss,sizeof(ss),&ssz,&st)) return false;
    if(ssz<sizeof(ss)||ss.magic!=SAVE_MAGIC) return false;

    uint8_t *rbuf=(uint8_t*)heap_alloc(4096);
    if(rbuf){
        uint32_t rsz=0; bool rt=false;
        if(fat_load_file(FAT_DRIVE_B,g_mm_cluster,ROSTR_FNAMES[slot],
                         (char*)rbuf,4096,&rsz,&rt))
            roster_load_buf(rbuf,rsz,&g_gs.party);
        heap_free(rbuf);
    }
    mm_apply_equip_bonuses(&g_gs.party);
    g_gs.map_idx=ss.map_idx;
    player_init(&g_gs.player,ss.player_x,ss.player_y,ss.player_facing);
    mm_load_ovr_for_map(g_gs.map_idx);
    return true;
}

/* ---- Apply ItemExtras stat bonuses from equipped items ---- */
static void mm_apply_equip_bonuses(Party *p)
{
    int i, s;
    for(i=0;i<p->count;i++){
        Character *c=&p->members[i];
        for(s=0;s<7;s++) c->equip_bonus[s]=0;
        for(s=0;s<6;s++){
            if(!c->equipped[s]) continue;
            const ItemExtras *ex=item_get_extras(c->equipped[s]);
            if(ex && ex->stat_idx>=1 && ex->stat_idx<=7)
                c->equip_bonus[ex->stat_idx-1]+=ex->stat_bonus;
        }
        for(s=0;s<7;s++){
            c->stats[s]=c->stats_base[s]+c->equip_bonus[s];
            if(c->stats[s]<1) c->stats[s]=1;
        }
    }
}

/* Keep mm_load_game wrapper — used by town_select_confirm Continue option */
static bool mm_load_game(void) { return mm_load_game_slot(0); }

/* Enter save-slot picker */
static void mm_enter_save_slot(int mode)
{
    g_save_slot_mode=mode;
    g_save_slot_cursor=0;
    int ss2;
    for(ss2=0;ss2<3;ss2++){
        g_slot_info[ss2].exists=false;
        g_slot_info[ss2].map_idx=0;
        SaveState sst; uint32_t ssz=0; bool st2=false;
        if(fat_load_file(FAT_DRIVE_B,g_mm_cluster,SAVE_FNAMES[ss2],
                         (char*)&sst,sizeof(sst),&ssz,&st2)
           && ssz>=sizeof(sst) && sst.magic==SAVE_MAGIC){
            g_slot_info[ss2].exists=true;
            g_slot_info[ss2].map_idx=sst.map_idx;
        }
    }
    g_prev_mode=g_mode; g_mode=GM_SAVE_SLOT; g_needs_redraw=true;
}

/* ---- Start combat ---- */
static void mm_start_combat(void){
    if(!g_game_ready) return;
    extern int monster_roll(int sides);
    monster_rng_seed((unsigned)pit_millis());
    combat_init(&g_combat,&g_gs);
    g_mode=GM_PRE_COMBAT; g_needs_redraw=true;
}

/* ---- App lifecycle ---- */
void mm_port_init(void){
    wnd_init(&g_frame,WIN_X,WIN_Y,WIN_W,WIN_H,WIN_MIN_W,WIN_MIN_H);
}
void mm_port_open(void){
    g_open=true;g_active=true;g_needs_redraw=true;
    set_game_mode(true);
    serial_str("MM: open\n");
    mm_load_game_data();
}
void mm_port_close(void){
    set_game_mode(false);
    sb16_stop(); speaker_note_off(); g_mus_seq=NULL; g_sfx_seq=NULL;
    wallpix_free(); monpix_free();
    if(g_monpix_buf){heap_free(g_monpix_buf);g_monpix_buf=NULL;}
    g_maps_loaded=0;g_wallpix_loaded=false;g_monpix_loaded=false;
    g_game_ready=false;g_ovr_loaded=false;g_open=false;g_active=false;
    serial_str("MM: closed\n");
}
bool mm_port_is_open(void)      { return g_open; }
bool mm_port_is_active(void)    { return g_active; }
bool mm_port_has_modal(void)    { return false; }
void mm_port_set_active(bool a) { g_active=a; }
void mm_port_bounds(int16_t *x,int16_t *y,int16_t *w,int16_t *h){
    wnd_display_bounds(&g_frame,x,y,w,h);
}

/* ---- Screen decoders ---- */
static void draw_title_screen(void){
    mm_memset(g_render_buf,0,sizeof(g_render_buf));
    {
        int tidx=(g_title_idx>=0&&g_title_idx<10)?g_title_idx:0;
        if(screen_dta_sizes[tidx]>0){
            /* Decode 320x200 and centre in 640x440 (double each pixel 2x2) */
            static uint8_t scr_raw[16000];
            extern int rle_decode_stream(const uint8_t*,int,int,uint8_t*,int,int);
            mm_memset(scr_raw,0,sizeof(scr_raw));
            rle_decode_stream(screen_dta_data+screen_dta_offsets[tidx],
                              (int)screen_dta_sizes[tidx],2,scr_raw,320,200);
        static const uint8_t PAL_NORMAL[4]={0,2,4,15};
        static const uint8_t PAL_SCREEN2[4]={0,3,5,15};
        const uint8_t *pal=(g_title_idx==2)?PAL_SCREEN2:PAL_NORMAL;
        int by,bx; /* offset to centre 640x400 in 640x440 */
        int ox=0,oy=20;
        for(by=0;by<200;by++) for(bx=0;bx<320;bx++){
            int raw_byte=by*(320/4)+bx/4;
            uint8_t c=pal[(scr_raw[raw_byte]>>(6-((bx%4)*2)))&3];
            int dx=ox+bx*2,dy=oy+by*2;
            g_render_buf[dy*RENDER_W+dx]=c;
            g_render_buf[dy*RENDER_W+dx+1]=c;
            g_render_buf[(dy+1)*RENDER_W+dx]=c;
            g_render_buf[(dy+1)*RENDER_W+dx+1]=c;
        }
        }  /* end if screen_dta_sizes */
    }  /* end tidx block */
    font_print(g_render_buf,RENDER_W,"Press any key or click to continue",
               (RENDER_W-font_str_width("Press any key or click to continue",1))/2,
               428,15,1);
}

static void draw_town_select_screen(void){
    static const char*const TOWNS[6]={
        "  Continue Saved Game",
        "1. Town of Sorpigal  (levels 1-3)",
        "2. Portsmith         (levels 2-4)",
        "3. Algary            (levels 2-4)",
        "4. Dusk              (levels 3-5)",
        "5. Erliquin          (levels 3-5)",
    };
    int i; uint16_t r;
    for(r=0;r<RENDER_H;r++) mm_memset(&g_render_buf[r*RENDER_W],0,RENDER_W);

    {const char*hdr="SELECT STARTING TOWN";
     int tw=font_str_width(hdr,2);
     font_print(g_render_buf,RENDER_W,hdr,(RENDER_W-tw)/2,30,15,2);}

    /* Always show all 6 options — Continue greyed out when no save */
    for(i=0;i<6;i++){
        int y=90+i*30;
        bool sel=(i==g_town_cursor);
        uint8_t col;
        if(i==0) col=g_save_exists?(sel?14:11):8; /* cyan if save, dark if no save */
        else     col=sel?14:7;
        if(sel){
            int by2;
            for(by2=y-2;by2<y+12;by2++)
                if(by2>=0&&by2<RENDER_H)
                    mm_memset(&g_render_buf[by2*RENDER_W+20],1,RENDER_W-40);
        }
        font_print(g_render_buf,RENDER_W,TOWNS[i],30,y,col,1);
        if(i==0&&!g_save_exists)
            font_print(g_render_buf,RENDER_W,"(no save)",220,y,8,1);
    }

    font_print(g_render_buf,RENDER_W,"UP/DOWN or 1-5  ENTER/CLICK to confirm  [N] New Party",
               20,420,8,1);
}

static void draw_combat_screen(void){
    int i,g;
    /* 3D backdrop */
    if(g_game_ready){
        const struct Map *m=&g_maps[g_gs.map_idx];
        int outdoor=(g_gs.map_idx>=14&&g_gs.map_idx<=33);
        render_3d_view(g_view3d_buf,VIEW3D_W,0,0,m,
                       g_gs.player.x,g_gs.player.y,g_gs.player.facing,outdoor,0);
    } else mm_memset(g_view3d_buf,0,sizeof(g_view3d_buf));

    /* Monster sprites — one per alive group, side-by-side */
    if(g_monpix_loaded&&g_combat.n_groups>0){
        /* X offsets for 1/2/3/4 alive groups */
        static const int SPR_DX[4][4]={
            {68,  0,   0,   0},
            {16,120,   0,   0},
            { 0, 68, 136,   0},
            { 0, 45,  90, 136}
        };
        int alive_cnt=0, gi2;
        for(gi2=0;gi2<g_combat.n_groups;gi2++) if(g_combat.groups[gi2].alive>0) alive_cnt++;
        if(alive_cnt<1) alive_cnt=1;
        if(alive_cnt>4) alive_cnt=4;
        int row=alive_cnt-1, drawn=0;
        int dy_spr=(VIEW3D_H-MONPIX_H)/2;
        for(gi2=0;gi2<g_combat.n_groups&&drawn<4;gi2++){
            if(g_combat.groups[gi2].alive<=0) continue;
            int sid=monster_sprite_id(g_combat.groups[gi2].type->id);
            if(sid>=0&&sid<MONPIX_NUM_MONSTERS&&monpix_decode(sid,g_mon_sprite))
                monpix_blit(g_mon_sprite,sid,g_view3d_buf,VIEW3D_W,SPR_DX[row][drawn],dy_spr);
            drawn++;
        }
    }

    /* Clear main buffer */
    uint16_t rr;
    for(rr=0;rr<RENDER_H;rr++) mm_memset(&g_render_buf[rr*RENDER_W],0,RENDER_W);

    /* Combat header */
    font_print(g_render_buf,RENDER_W,"=COMBAT=",2,2,15,1);

    /* Monster list with inline HP bars */
    for(g=0;g<g_combat.n_groups;g++){
        MonsterGroup *mg=&g_combat.groups[g];
        char mline[40]; int ml=0;
        mline[ml++]=(char)('A'+g);mline[ml++]=':';mline[ml++]=' ';
        const char *mn=mg->type->name; while(*mn&&ml<26) mline[ml++]=*mn++;
        mline[ml++]=' ';mline[ml++]='x';
        if(mg->alive>=10) mline[ml++]=(char)('0'+mg->alive/10);
        mline[ml++]=(char)('0'+mg->alive%10);
        if(mg->sleep_rounds>0){mline[ml++]='(';mline[ml++]='Z';mline[ml++]='z';mline[ml++]=')';}
        mline[ml]='\0';
        int gy=14+g*10;
        font_print(g_render_buf,RENDER_W,mline,RENDER_W/2,gy,mg->alive>0?4:8,1);
        /* HP bar: 40px wide, 4px tall, inline right of text */
        if(mg->alive>0){
            int total_hp=mg->type->hp*mg->count;
            int cur_hp=0,k2; for(k2=0;k2<mg->type->max_group;k2++) if(mg->hp[k2]>0) cur_hp+=mg->hp[k2];
            int bar_w=40;
            int filled=total_hp>0?(cur_hp*bar_w)/total_hp:0;
            int bx=RENDER_W/2+font_str_width(mline,1)+3;
            uint8_t fcol=(uint8_t)(filled>bar_w*2/3?2:(filled>bar_w/3?14:4));
            int bri,bci;
            for(bri=gy+1;bri<=gy+4;bri++)
                for(bci=bx;bci<bx+bar_w&&bci<RENDER_W;bci++)
                    g_render_buf[bri*RENDER_W+bci]=(uint8_t)(bci-bx<filled?fcol:8);
        }
    }

    /* Combat log */
    for(i=0;i<COMBAT_LOG_LINES&&i<g_combat.log_count;i++){
        int li=(g_combat.log_head-1-i+COMBAT_LOG_LINES)%COMBAT_LOG_LINES;
        font_print(g_render_buf,RENDER_W,g_combat.log[li],RENDER_W/2,54+i*9,7,1);
    }

    /* Party HP — two columns of 3, right panel */
    for(i=0;i<g_gs.party.count&&i<6;i++){
        Character *c=&g_gs.party.members[i];
        char line[32]; int li=0;
        const char *nm=c->name; while(*nm&&li<8) line[li++]=*nm++;
        while(li<8) line[li++]=' ';
        line[li++]=' ';
        if(c->hp>=100) line[li++]=(char)('0'+c->hp/100);
        if(c->hp>=10)  line[li++]=(char)('0'+(c->hp/10)%10);
        line[li++]=(char)('0'+c->hp%10); line[li]='\0';
        uint8_t col=IS_DEADLIKE(c->condition)?8:(c->hp<=0?4:2);
        /* Two columns of 3 in right panel (x=480+) */
        int col_x = (i<3) ? RENDER_W/2 : RENDER_W/2+80;
        int row_y = 160 + (i<3?i:i-3)*10;
        font_print(g_render_buf,RENDER_W,line,col_x,row_y,col,1);
    }

    const char *prompt=g_combat.over?"ENTER to continue":
        (g_combat.bless_rounds>0?"[A]ttack [F]lee [S]pell [I]tem [G]bribe *BLESSED*":
                                  "[A]ttack [F]lee [S]pell [I]tem [G]bribe");
    font_print(g_render_buf,RENDER_W,prompt,RENDER_W/2,174,14,1);
}

/* ---- Town entry message on map change ---- */
static void mm_on_map_change(int new_map) {
    if (g_game_ready) {
        const char *nm = map_display_name(new_map);
        if (nm && *nm) player_set_message(&g_gs.player, nm, 90);
    }
}

/* ---- Blacksmith / Shop screen ---- */
static void draw_shop_screen(void)
{
    int x, y; uint16_t r;
    for(r=0;r<RENDER_H;r++) mm_memset(&g_render_buf[r*RENDER_W],0,RENDER_W);
    {const char *hdr="BLACKSMITH"; int tw=font_str_width(hdr,2);
     font_print(g_render_buf,RENDER_W,hdr,(RENDER_W-tw)/2,10,14,2);}

    int count=0;
    const int *stock=shop_stock(g_shop_town,&count);
    if (!stock||count==0){
        font_print(g_render_buf,RENDER_W,"No items in stock.",20,60,8,1); return;
    }

    font_print(g_render_buf,RENDER_W,"Item                    Cost  [B]uy  [S]ell backpack  [E]quipped  [ESC]close",
               10,38,7,1);
    mm_memset(&g_render_buf[48*RENDER_W+10],11,620);

    int vis=20, start=g_shop_scroll, i;
    if(start+vis>count) vis=count-start;
    for(i=0;i<vis;i++){
        int id=stock[start+i];
        const ItemDef *it=item_get(id); if(!it) continue;
        int row_y=56+i*18;
        font_print(g_render_buf,RENDER_W,it->name,10,row_y,15,1);
        char cost[10]; int ci=0;
        int c2=it->cost;
        if(c2>=1000){cost[ci++]=(char)('0'+c2/1000);c2%=1000;}
        if(c2>=100){cost[ci++]=(char)('0'+c2/100);c2%=100;}
        if(c2>=10){cost[ci++]=(char)('0'+c2/10);c2%=10;}
        cost[ci++]=(char)('0'+c2); cost[ci++]='g'; cost[ci]='\0';
        font_print(g_render_buf,RENDER_W,cost,230,row_y,14,1);
    }
    font_print(g_render_buf,RENDER_W,"Gold:",10,RENDER_H-18,7,1);
    {
        char gl[12]; int gi=0; int gv=(int)g_gs.party.members[0].gold;
        if(gv>=10000) gl[gi++]=(char)('0'+gv/10000);
        if(gv>=1000)  gl[gi++]=(char)('0'+(gv/1000)%10);
        if(gv>=100)   gl[gi++]=(char)('0'+(gv/100)%10);
        if(gv>=10)    gl[gi++]=(char)('0'+(gv/10)%10);
        gl[gi++]=(char)('0'+gv%10); gl[gi++]='g'; gl[gi]='\0';
        font_print(g_render_buf,RENDER_W,gl,44,RENDER_H-18,14,1);
    }
    (void)x; (void)y;
}

/* ---- Full-screen map overlay ---- */
static void draw_map_screen(void)
{
    int x, y; uint16_t r;
    for(r=0;r<RENDER_H;r++) mm_memset(&g_render_buf[r*RENDER_W],0,RENDER_W);

    if (!g_game_ready) return;
    const struct Map *m=&g_maps[g_gs.map_idx];
    const char *mname=map_display_name(g_gs.map_idx);
    {int tw=font_str_width(mname,1); font_print(g_render_buf,RENDER_W,mname,(RENDER_W-tw)/2,2,15,1);}

    int ox=20, oy=14;
    int cw=(RENDER_W-40)/MAP_SIZE;   /* ~37 */
    int ch=(RENDER_H-50)/MAP_SIZE;   /* ~23 */

    for(y=0;y<MAP_SIZE;y++) for(x=0;x<MAP_SIZE;x++){
        const Cell *c=map_cell(m,x,y); if(!c) continue;
        int dy=MAP_SIZE-1-y;
        int px=ox+x*cw, py=oy+dy*ch;
        /* Floor */
        int rx,ry;
        for(ry=py+1;ry<py+ch-1;ry++) for(rx=px+1;rx<px+cw-1;rx++)
            if(rx>=0&&rx<RENDER_W&&ry>=0&&ry<RENDER_H) g_render_buf[ry*RENDER_W+rx]=1;
        /* Walls */
        if(c->wall[0]&&py>oy)          {int k;for(k=px;k<px+cw;k++) if(k>=0&&k<RENDER_W&&py>=0&&py<RENDER_H) g_render_buf[py*RENDER_W+k]=c->wall[0]==2?2:7;}
        if(c->wall[2]&&py+ch<oy+MAP_SIZE*ch){int k;for(k=px;k<px+cw;k++) if(k>=0&&k<RENDER_W&&py+ch-1>=0&&py+ch-1<RENDER_H) g_render_buf[(py+ch-1)*RENDER_W+k]=c->wall[2]==2?2:7;}
        if(c->wall[3]&&px>ox)          {int k;for(k=py;k<py+ch;k++) if(px>=0&&px<RENDER_W&&k>=0&&k<RENDER_H) g_render_buf[k*RENDER_W+px]=c->wall[3]==2?2:7;}
        if(c->wall[1]&&px+cw<ox+MAP_SIZE*cw){int k;for(k=py;k<py+ch;k++) if(px+cw-1>=0&&px+cw-1<RENDER_W&&k>=0&&k<RENDER_H) g_render_buf[k*RENDER_W+(px+cw-1)]=c->wall[1]==2?2:7;}
        /* Service */
        ServiceType svc=tile_service_at(g_gs.map_idx,x,y);
        uint8_t sc2=0;
        switch(svc){case SVC_INN:sc2=14;break;case SVC_TEMPLE:sc2=11;break;case SVC_TRAINING:sc2=10;break;
                    case SVC_BLACKSMITH:sc2=6;break;case SVC_FOOD:sc2=2;break;case SVC_TAVERN:sc2=4;break;
                    case SVC_STAIRS:case SVC_STAIRS_UP:sc2=9;break;default:break;}
        if(sc2){int mx=px+cw/2,my=py+ch/2; if(mx>=0&&mx<RENDER_W&&my>=0&&my<RENDER_H) g_render_buf[my*RENDER_W+mx]=sc2;}
    }

    /* Player dot */
    {int dy=MAP_SIZE-1-g_gs.player.y;
     int px=ox+g_gs.player.x*cw+cw/2, py=oy+dy*ch+ch/2;
     if(px>=0&&px<RENDER_W&&py>=0&&py<RENDER_H){
         int r2,c2; for(r2=py-1;r2<=py+1;r2++) for(c2=px-1;c2<=px+1;c2++)
             if(c2>=0&&c2<RENDER_W&&r2>=0&&r2<RENDER_H) g_render_buf[r2*RENDER_W+c2]=14;
     }}

    font_print(g_render_buf,RENDER_W,"[ESC] or [M] to close  White=wall  Green=door  Coloured=service",
               10,RENDER_H-12,8,1);
}

/* ---- Spell select screen ---- */
static void draw_spell_screen(void)
{
    int i; uint16_t r;
    for(r=0;r<RENDER_H;r++) mm_memset(&g_render_buf[r*RENDER_W],0,RENDER_W);
    font_print(g_render_buf,RENDER_W,"CAST SPELL",4,4,14,1);
    mm_memset(&g_render_buf[14*RENDER_W],11,RENDER_W);

    for(i=0;i<g_spell_count&&i<20;i++){
        const SpellDef *sp=spell_get(g_spell_ids[i]); if(!sp) continue;
        int y=18+i*14;
        char line[50]; int li=0;
        line[li++]=(char)('1'+(i%9)); line[li++]='.'; line[li++]=' ';
        const char *sn=sp->name; while(*sn&&li<30) line[li++]=(char)*sn++;
        line[li++]=' '; line[li++]='(';
        if(sp->sp_cost>=10) line[li++]=(char)('0'+sp->sp_cost/10);
        line[li++]=(char)('0'+sp->sp_cost%10);
        line[li++]='S'; line[li++]='P'; line[li++]=')'; line[li]='\0';
        font_print(g_render_buf,RENDER_W,line,10,y,15,1);
    }
    font_print(g_render_buf,RENDER_W,"[1-9] cast  [ESC] cancel",10,RENDER_H-12,8,1);
}

/* ---- Combat item select screen ---- */
static void mm_collect_combat_items(void)
{
    g_citem_count=0;
    int pi,si;
    for(pi=0;pi<g_gs.party.count&&g_citem_count<9;pi++){
        Character *ch=&g_gs.party.members[pi];
        if(IS_DEADLIKE(ch->condition)) continue;
        for(si=0;si<6&&g_citem_count<9;si++){
            if(ch->backpack[si]==0) continue;
            const ItemDef *it=item_get(ch->backpack[si]);
            if(it&&it->spell_id>0){
                g_citem_ids [g_citem_count]=ch->backpack[si];
                g_citem_char[g_citem_count]=pi;
                g_citem_slot[g_citem_count]=si;
                g_citem_count++;
            }
        }
    }
}

static void draw_combat_item_screen(void)
{
    int i; uint16_t r;
    for(r=0;r<RENDER_H;r++) mm_memset(&g_render_buf[r*RENDER_W],0,RENDER_W);
    font_print(g_render_buf,RENDER_W,"USE ITEM",4,4,14,1);
    mm_memset(&g_render_buf[14*RENDER_W],11,RENDER_W);
    if(g_citem_count==0){
        font_print(g_render_buf,RENDER_W,"No usable items.",10,24,8,1);
    } else {
        for(i=0;i<g_citem_count;i++){
            const ItemDef *it=item_get(g_citem_ids[i]); if(!it) continue;
            char line[50]; int li=0;
            line[li++]=(char)('1'+i); line[li++]='.'; line[li++]=' ';
            const char *in=it->name; while(*in&&li<46) line[li++]=(char)*in++;
            line[li]='\0';
            font_print(g_render_buf,RENDER_W,line,10,18+i*14,15,1);
        }
    }
    font_print(g_render_buf,RENDER_W,"[1-9] use  [ESC] cancel",10,RENDER_H-12,8,1);
}

/* ---- Level-up check (call after combat rewards) ---- */
static void check_level_ups(void)
{
    static const int XP_T[6][10]={
        {1500,3000,6000,12000,24000,48000,96000,192000,384000,768000},
        {2000,4000,8000,16000,32000,64000,128000,256000,512000,1024000},
        {2000,4000,8000,16000,32000,64000,128000,256000,512000,1024000},
        {2000,4000,8000,16000,32000,64000,128000,256000,512000,1024000},
        {2500,5000,10000,20000,40000,80000,160000,320000,640000,1280000},
        {1000,2000,4000,8000,16000,32000,64000,128000,256000,512000}
    };
    static const int HP_DIE[7]={0,10,8,6,6,4,6};
    extern int monster_roll(int);
    int i;
    for(i=0;i<g_gs.party.count;i++){
        Character *c=&g_gs.party.members[i];
        if(IS_DEADLIKE(c->condition)||c->level>=10) continue;
        int cls=c->cls; if(cls<1||cls>6) continue;
        int needed=XP_T[cls-1][c->level-1];
        if(needed<=0||c->xp<needed) continue;
        /* Level up! */
        c->level++;
        int hp_gain=monster_roll(HP_DIE[cls]);
        /* SP gain based on relevant stat */
        int sp_stat=0;
        if(cls==4||cls==2) sp_stat=c->stats_base[2]; /* Cleric/Paladin: PER */
        else if(cls==5||cls==3) sp_stat=c->stats_base[0]; /* Sorc/Archer: INT */
        int sp_factor=(sp_stat>=40)?10:(sp_stat>=35)?9:(sp_stat>=30)?8:(sp_stat>=25)?7:
                      (sp_stat>=20)?6:(sp_stat>=16)?5:(sp_stat>=13)?4:(sp_stat>=10)?3:
                      (sp_stat>=7)?2:(sp_stat>=5)?1:0;
        int sp_gain=((cls==4||cls==2||cls==5||cls==3))?sp_factor:0;
        if((cls==2||cls==3)&&c->level<7) sp_gain=0; /* Paladin/Archer need lv7 */
        c->hp_max+=hp_gain; c->hp+=hp_gain;
        c->sp_max+=sp_gain;
        c->stats_base[3]+=1; /* END +1 per level */
        sfx_play(MM_CHORD,MM_CHORD_LEN);
        /* Notify */
        {
            char msg[48]; int mi=0;
            const char *nm=c->name; while(*nm&&mi<12) msg[mi++]=*nm++;
            msg[mi++]=' '; msg[mi++]='L'; msg[mi++]='v';
            msg[mi++]=(char)('0'+(c->level>=10?c->level/10:0));
            msg[mi++]=(char)('0'+c->level%10); msg[mi++]='!';
            msg[mi++]=' '; msg[mi++]='H'; msg[mi++]='P'; msg[mi++]='+';
            msg[mi++]=(char)('0'+hp_gain/10); msg[mi++]=(char)('0'+hp_gain%10);
            msg[mi]='\0';
            player_set_message(&g_gs.player,msg,180);
        }
    }
}

/* ---- Defeat screen ---- */
static void draw_defeat_screen(void)
{
    uint16_t r; int x;
    for(r=0;r<RENDER_H;r++) mm_memset(&g_render_buf[r*RENDER_W],0,RENDER_W);
    /* Red "YOUR PARTY HAS FALLEN" */
    {const char*t="YOUR PARTY HAS FALLEN";
     int tw=font_str_width(t,2);
     font_print(g_render_buf,RENDER_W,t,(RENDER_W-tw)/2,RENDER_H/3,4,2);}
    {const char*t="Press any key to continue";
     int tw=font_str_width(t,1);
     font_print(g_render_buf,RENDER_W,t,(RENDER_W-tw)/2,RENDER_H/2,15,1);}
    (void)x;
}

/* ---- Character sheet overlay ---- */
static void draw_charsheet(void)
{
    static const char*const RACE_NAMES[6]={"","Human","Elf","Dwarf","Gnome","Half-Orc"};
    static const char*const CLS_NAMES[7]={"","Knight","Paladin","Archer","Cleric","Sorcerer","Robber"};
    static const char*const STAT_NAMES[7]={"INT","MIG","PER","END","SPD","ACC","LCK"};
    static const int XP_T[6][10]={
        {1500,3000,6000,12000,24000,48000,96000,192000,384000,768000},
        {2000,4000,8000,16000,32000,64000,128000,256000,512000,1024000},
        {2000,4000,8000,16000,32000,64000,128000,256000,512000,1024000},
        {2000,4000,8000,16000,32000,64000,128000,256000,512000,1024000},
        {2500,5000,10000,20000,40000,80000,160000,320000,640000,1280000},
        {1000,2000,4000,8000,16000,32000,64000,128000,256000,512000}
    };
    int i;
    /* Dim background */
    uint16_t rr; int x;
    for(rr=0;rr<RENDER_H;rr++) for(x=0;x<RENDER_W;x++) if((rr+x)&1) g_render_buf[rr*RENDER_W+x]=0;

    if(g_charsheet_idx<0||g_charsheet_idx>=g_gs.party.count){g_charsheet_idx=0;}
    const Character *c=&g_gs.party.members[g_charsheet_idx];

    /* Panel */
    int px=20,py=10,pw=600,ph=420;
    for(i=py;i<py+ph;i++) mm_memset(&g_render_buf[i*RENDER_W+px],0,pw);
    {int y; for(y=py;y<py+ph;y++){g_render_buf[y*RENDER_W+px]=11;g_render_buf[y*RENDER_W+px+pw-1]=11;}
     for(x=px;x<px+pw;x++){g_render_buf[py*RENDER_W+x]=11;g_render_buf[(py+ph-1)*RENDER_W+x]=11;}}

    /* Party tabs */
    for(i=0;i<g_gs.party.count;i++){
        char tab[14]; int ti=0;
        const char*nm=g_gs.party.members[i].name; while(*nm&&ti<8) tab[ti++]=*nm++;
        tab[ti]='\0';
        font_print(g_render_buf,RENDER_W,tab,px+4+i*100,py+4,(i==g_charsheet_idx)?14:7,1);
    }
    font_print(g_render_buf,RENDER_W,"1-6:next  ESC:close",px+4,py+ph-12,8,1);

    int col1=px+8, col2=px+200, col3=px+380;
    int y=py+18;

    /* Name, Race, Class, Level */
    font_print(g_render_buf,RENDER_W,c->name,col1,y,15,1); y+=10;
    {
        const char*rn=(c->race>=1&&c->race<=5)?RACE_NAMES[c->race]:"?";
        const char*cn=(c->cls>=1&&c->cls<=6)?CLS_NAMES[c->cls]:"?";
        char line[40]; int li=0;
        while(*rn&&li<12) line[li++]=*rn++;
        line[li++]=' '; while(*cn&&li<24) line[li++]=*cn++;
        line[li]='\0';
        font_print(g_render_buf,RENDER_W,line,col1,y,7,1); y+=10;
    }
    {
        char lv[24];
        mm_snprintf(lv,sizeof(lv),"Level %d   XP: %d",c->level,(int)c->xp);
        font_print(g_render_buf,RENDER_W,lv,col1,y,14,1); y+=10;
        /* XP to next level */
        if(c->cls>=1&&c->cls<=6&&c->level>=1&&c->level<=9){
            int needed=XP_T[c->cls-1][c->level-1];
            char xpn[32]; mm_snprintf(xpn,sizeof(xpn),"Next: %d",needed);
            font_print(g_render_buf,RENDER_W,xpn,col1,y,c->xp>=needed?10:8,1);
        }
        y+=12;
    }

    /* Stats */
    mm_memset(&g_render_buf[y*RENDER_W+col1],11,180); y++;
    int s;
    for(s=0;s<7;s++){
        char sl[20];
        mm_snprintf(sl,sizeof(sl),"%s: %d/%d",STAT_NAMES[s],c->stats[s],c->stats_base[s]);
        font_print(g_render_buf,RENDER_W,sl,col1+(s>=4?100:0),y+(s>=4?s-4:s)*9,7,1);
    }
    y+=40;

    /* HP / SP / AC */
    {
        char hs[40];
        mm_snprintf(hs,sizeof(hs),"HP: %d/%d  SP: %d/%d  AC: %d",
                    c->hp,c->hp_max,c->sp,c->sp_max,c->ac);
        font_print(g_render_buf,RENDER_W,hs,col1,y,c->hp>0?2:4,1); y+=10;
    }
    /* Food / Gold */
    {
        char fg[30]; mm_snprintf(fg,sizeof(fg),"Food: %d  Gold: %d",c->food,(int)c->gold);
        font_print(g_render_buf,RENDER_W,fg,col1,y,14,1); y+=12;
    }

    /* Condition */
    mm_memset(&g_render_buf[y*RENDER_W+col1],11,180); y++;
    {
        const char*cond="Good";
        if(c->condition==0xFF) cond="ERADICATED";
        else if(c->condition==0xA0) cond="STONED";
        else if(c->condition==0xC0) cond="DEAD";
        else if(c->condition&COND_PARALYZED) cond="Paralyzed";
        else if(c->condition&COND_UNCONSCIOUS) cond="Unconscious";
        else if(c->condition&COND_POISONED) cond="Poisoned";
        else if(c->condition&COND_DISEASED) cond="Diseased";
        else if(c->condition&COND_BLINDED) cond="Blinded";
        char cline[20]; mm_snprintf(cline,sizeof(cline),"Condition: %s",cond);
        font_print(g_render_buf,RENDER_W,cline,col1,y,
                   c->condition?12:10,1); y+=12;
    }

    /* Equipment */
    mm_memset(&g_render_buf[y*RENDER_W+col1],11,180); y++;
    font_print(g_render_buf,RENDER_W,"Equipment:",col1,y,11,1); y+=9;
    for(s=0;s<6;s++){
        if(c->equipped[s]){
            char el[30]; int ei=0;
            const char *in=item_name(c->equipped[s]);
            while(*in&&ei<28) el[ei++]=(char)*in++;
            el[ei]='\0';
            font_print(g_render_buf,RENDER_W,el,col1+8,y,7,1); y+=9;
        }
    }
    /* Backpack */
    {
        bool any=false;
        for(s=0;s<6;s++) if(c->backpack[s]) any=true;
        if(any){
            font_print(g_render_buf,RENDER_W,"Backpack:",col2,py+20,11,1);
            int by=py+30;
            for(s=0;s<6;s++){
                if(c->backpack[s]){
                    char bl[30]; int bi=0;
                    const char *in=item_name(c->backpack[s]);
                    while(*in&&bi<28) bl[bi++]=(char)*in++;
                    bl[bi]='\0';
                    font_print(g_render_buf,RENDER_W,bl,col2+8,by,7,1); by+=9;
                }
            }
        }
    }
    (void)col3;
}

/* ---- Options menu overlay ---- */
static void draw_options_screen(void)
{
    /* Dark semi-transparent overlay — draw over existing game pixels */
    int y, x; uint16_t r;
    for(r=0;r<RENDER_H;r++)
        for(x=0;x<RENDER_W;x++)
            if(((int)r+x)&1) g_render_buf[r*RENDER_W+x]=0; /* checkerboard dim */

    /* Panel */
    int px=60, py=40, pw=520, ph=360;
    for(y=py;y<py+ph;y++) mm_memset(&g_render_buf[y*RENDER_W+px],1,pw);
    for(y=py+3;y<py+ph-3;y++) mm_memset(&g_render_buf[y*RENDER_W+px+3],0,pw-6);
    /* Cyan border */
    for(y=py;y<py+ph;y++){g_render_buf[y*RENDER_W+px]=11;g_render_buf[y*RENDER_W+px+pw-1]=11;}
    for(x=px;x<px+pw;x++){g_render_buf[py*RENDER_W+x]=11;g_render_buf[(py+ph-1)*RENDER_W+x]=11;}

    /* Title */
    font_print(g_render_buf,RENDER_W,"OPTIONS  (F1)",px+16,py+12,14,1);

    /* Status */
    char status[80];
    mm_snprintf(status,sizeof(status),"Music: %s   Sounds: %s   Cheat: %s   Auto-Search: %s",
        g_music_en?"ON ":"OFF",
        g_sfx_en?"ON ":"OFF",
        g_gs.cheat_combat?"ON ":"OFF",
        g_gs.auto_search?"ON ":"OFF");
    font_print(g_render_buf,RENDER_W,status,px+16,py+28,11,1);

    /* Separator */
    mm_memset(&g_render_buf[(py+40)*RENDER_W+px+3],11,pw-6);

    /* Items */
    static const char * const ITEMS[] = {
        "[M]  Toggle Music",
        "[S]  Toggle Sounds",
        "[C]  Toggle Cheat Mode (auto-win fights)",
        "[A]  Toggle Auto-Search after battle",
        "[P]  Save Game",
        "[L]  Load Game",
        "[Q]  Quit to Desktop",
        NULL
    };
    int iy=py+50;
    int i;
    for(i=0;ITEMS[i];i++,iy+=14)
        font_print(g_render_buf,RENDER_W,ITEMS[i],px+16,iy,15,1);

    mm_memset(&g_render_buf[(iy+4)*RENDER_W+px+3],11,pw-6);
    font_print(g_render_buf,RENDER_W,"[ ESC or X ]  Close",px+16,iy+10,8,1);
}

/* ---- Quit confirmation overlay ---- */
static void draw_quit_confirm(void)
{
    int y,x;
    for(y=0;y<RENDER_H;y++) for(x=0;x<RENDER_W;x++)
        if((y+x)&1) g_render_buf[y*RENDER_W+x]=0;

    int px=160,py=160,pw=320,ph=80;
    for(y=py;y<py+ph;y++) mm_memset(&g_render_buf[y*RENDER_W+px],0,pw);
    for(y=py+2;y<py+ph-2;y++){g_render_buf[y*RENDER_W+px]=4;g_render_buf[y*RENDER_W+px+pw-1]=4;}
    for(x=px;x<px+pw;x++){g_render_buf[py*RENDER_W+x]=4;g_render_buf[(py+ph-1)*RENDER_W+x]=4;}

    font_print(g_render_buf,RENDER_W,"Are you sure you want to quit?",px+20,py+16,15,1);
    font_print(g_render_buf,RENDER_W,"[Y] Yes — quit to HamsterOS",  px+20,py+34,14,1);
    font_print(g_render_buf,RENDER_W,"[N] No  — keep playing",        px+20,py+48,7, 1);
}

/* ---- Shared spell effect dispatcher ---- */
static void mm_cast_spell(const SpellDef *sp, Character *caster)
{
    extern int monster_roll(int);
    int t, gi, g2, k;
    switch(sp->effect){
    case SPELL_EFFECT_HEAL:
        for(t=0;t<g_gs.party.count;t++){
            if(!IS_DEADLIKE(g_gs.party.members[t].condition)){
                int heal=monster_roll(sp->param1)+sp->param2;
                g_gs.party.members[t].hp+=heal;
                if(g_gs.party.members[t].hp>g_gs.party.members[t].hp_max)
                    g_gs.party.members[t].hp=g_gs.party.members[t].hp_max;
            }
        }
        player_set_message(&g_gs.player,"PARTY HEALED!",90);
        sfx_play(MM_HIT,MM_HIT_LEN);
        break;
    case SPELL_EFFECT_DAMAGE:
        gi=(sp->target==SPELL_TARGET_ALL)?-1:(g_combat.target_group>=0?g_combat.target_group:0);
        for(g2=0;g2<g_combat.n_groups;g2++){
            if(gi>=0&&g2!=gi) continue;
            if(g_combat.groups[g2].alive<=0) continue;
            {
                int dmg=monster_roll(sp->param1)*((caster->level/2)+1);
                for(k=0;k<g_combat.groups[g2].type->max_group;k++)
                    if(g_combat.groups[g2].hp[k]>0){
                        g_combat.groups[g2].hp[k]-=dmg;
                        if(g_combat.groups[g2].hp[k]<=0){g_combat.groups[g2].hp[k]=0;g_combat.groups[g2].alive--;}
                    }
            }
        }
        player_set_message(&g_gs.player,"SPELL CAST!",90);
        sfx_play(MM_HIT,MM_HIT_LEN);
        break;
    case SPELL_EFFECT_CURE:
        for(t=0;t<g_gs.party.count;t++)
            g_gs.party.members[t].condition&=(uint8_t)(~sp->param1);
        player_set_message(&g_gs.player,"CONDITIONS CURED!",90);
        break;
    case SPELL_EFFECT_SLEEP:
        gi=(sp->target==SPELL_TARGET_ALL)?-1:(g_combat.target_group>=0?g_combat.target_group:0);
        for(g2=0;g2<g_combat.n_groups;g2++){
            if(gi>=0&&g2!=gi) continue;
            if(g_combat.groups[g2].alive>0) g_combat.groups[g2].sleep_rounds+=sp->param1;
        }
        player_set_message(&g_gs.player,"MONSTERS ASLEEP!",90);
        sfx_play(MM_HIT,MM_HIT_LEN);
        break;
    case SPELL_EFFECT_BLESS:
        g_combat.bless_bonus+=sp->param1;
        g_combat.bless_rounds=10+caster->level;
        player_set_message(&g_gs.player,"PARTY BLESSED!",90);
        break;
    case SPELL_EFFECT_RAISE:
        for(t=0;t<g_gs.party.count;t++){
            if(IS_DEADLIKE(g_gs.party.members[t].condition)){
                g_gs.party.members[t].condition=0; g_gs.party.members[t].hp=1; break;
            }
        }
        player_set_message(&g_gs.player,"RAISED FROM DEAD!",90);
        sfx_play(MM_CHORD,MM_CHORD_LEN);
        break;
    case SPELL_EFFECT_RESTORE:
        caster->sp+=sp->param1;
        if(caster->sp>caster->sp_max) caster->sp=caster->sp_max;
        player_set_message(&g_gs.player,"ENERGY RESTORED!",90);
        break;
    case SPELL_EFFECT_ESCAPE:
        if(g_mode==GM_COMBAT||g_mode==GM_SPELL_SELECT||g_mode==GM_COMBAT_ITEM){
            combat_flee(&g_combat,&g_gs);
            if(g_combat.over) g_mode=GM_EXPLORE;
            else player_set_message(&g_gs.player,"CAN'T ESCAPE!",60);
        } else {
            player_set_message(&g_gs.player,"SURFACE! (noncombat only)",60);
        }
        break;
    case SPELL_EFFECT_FOOD:
        g_gs.party.shared_food+=5+caster->level;
        if(g_gs.party.shared_food>99) g_gs.party.shared_food=99;
        player_set_message(&g_gs.player,"FOOD CREATED!",90);
        break;
    case SPELL_EFFECT_PORTAL: {
        extern void game_state_set_town(GameState*,int);
        game_state_set_town(&g_gs,0);
        mm_load_ovr_for_map(g_gs.map_idx);
        g_mode=GM_EXPLORE;
        player_set_message(&g_gs.player,"TOWN PORTAL!",90);
    } break;
    case SPELL_EFFECT_LIGHT: {
        char lmsg[48];
        mm_snprintf(lmsg,sizeof(lmsg),"Pos: %d,%d Facing %s",
            g_gs.player.x,g_gs.player.y,FACING_NAME[g_gs.player.facing]);
        player_set_message(&g_gs.player,lmsg,180);
    } break;
    case SPELL_EFFECT_PROTECT:
        if(sp->param1>=0&&sp->param1<18) g_gs.party.protections[sp->param1]+=sp->param2;
        player_set_message(&g_gs.player,"PROTECTION GRANTED!",90);
        break;
    default:
        player_set_message(&g_gs.player,"SPELL CAST!",90);
        break;
    }
    (void)gi; (void)g2; (void)k; (void)t;
}

/* ---- Character creation helpers ---- */
static void cc_roll_stats(void)
{
    extern int monster_roll(int);
    static const int RACE_BASE[5][7]={
        {10,10,10,10,10,10,10}, /* Human */
        {12, 7,10, 8,12,10,10}, /* Elf */
        { 6,14, 8,14, 8,10, 6}, /* Dwarf */
        { 7,10,10,10, 9,10,15}, /* Gnome */
        { 4,16, 5,16, 8,10, 5}, /* Half-Orc */
    };
    static const int CLS_BNS[6][7]={
        { 0,+4, 0,+2, 0,+2, 0}, /* Knight */
        { 0,+3,+2,+2, 0,+1, 0}, /* Paladin */
        {+2,+2, 0,+1,+2,+2, 0}, /* Archer */
        {+2, 0,+3,+1, 0, 0,+2}, /* Cleric */
        {+4, 0,+1, 0,+2, 0,+2}, /* Sorcerer */
        { 0,+2, 0,+1,+3,+2,+2}, /* Robber */
    };
    static const int HP_DIE[7]={0,12,10,8,8,6,8};
    static const int SP_ST[7] ={0, 0, 2,2,2,3,0};
    int s;
    for(s=0;s<7;s++){
        int v=RACE_BASE[g_cc_race-1][s]+CLS_BNS[g_cc_cls-1][s]
             +monster_roll(6)+monster_roll(6)+monster_roll(6);
        if(v<3) v=3;
        if(v>25) v=25;
        g_cc_stats[s]=v;
    }
    int end_mod=(g_cc_stats[3]-10)/2;
    g_cc_hp=1+monster_roll(HP_DIE[g_cc_cls])+end_mod;
    if(g_cc_hp<1) g_cc_hp=1;
    g_cc_sp=SP_ST[g_cc_cls];
}

static void cc_accept(void)
{
    extern int monster_roll(int);
    static const char *CLS_NM[7]={"","KNIGHT","PALADIN","ARCHER","CLERIC","SORCER","ROBBER"};
    static const char *RACE_NM[6]={"","HUM","ELF","DWF","GNM","HLF"};
    if(g_cc_member_idx>=MAX_PARTY) return;
    Character *c=&g_cc_party.members[g_cc_member_idx];
    mm_memset(c,0,sizeof(*c));
    c->cls=g_cc_cls; c->race=g_cc_race;
    mm_memcpy(c->stats,g_cc_stats,sizeof(c->stats));
    mm_memcpy(c->stats_base,g_cc_stats,sizeof(c->stats_base));
    c->level=1; c->hp=g_cc_hp; c->hp_max=g_cc_hp;
    c->sp=g_cc_sp; c->sp_max=g_cc_sp; c->ac=0; c->slot=g_cc_member_idx;
    c->gold=(monster_roll(6)+monster_roll(6)+monster_roll(6))*10;
    /* Name: CLSRACENUM e.g. KNIGHTHUM1 */
    const char *cn=CLS_NM[g_cc_cls]; int ni=0;
    while(*cn&&ni<6) c->name[ni++]=(char)*cn++;
    const char *rn=RACE_NM[g_cc_race]; while(*rn&&ni<9) c->name[ni++]=(char)*rn++;
    c->name[ni++]=(char)('1'+g_cc_member_idx); c->name[ni]='\0';
    g_cc_party.count++;
    g_cc_member_idx++;
    g_cc_phase=0; g_cc_cls=0; g_cc_race=0;
    mm_apply_equip_bonuses(&g_cc_party);
}

static void mm_enter_char_create(void)
{
    mm_memset(&g_cc_party,0,sizeof(g_cc_party));
    g_cc_member_idx=0; g_cc_phase=0; g_cc_cls=0; g_cc_race=0;
    g_mode=GM_CHAR_CREATE; g_needs_redraw=true;
}

/* ---- Save slot screen ---- */
static void draw_save_slot_screen(void)
{
    int i; uint16_t r;
    for(r=0;r<RENDER_H;r++) mm_memset(&g_render_buf[r*RENDER_W],0,RENDER_W);
    const char *hdr=g_save_slot_mode?"LOAD GAME":"SAVE GAME";
    {int tw=font_str_width(hdr,2);font_print(g_render_buf,RENDER_W,hdr,(RENDER_W-tw)/2,10,14,2);}
    for(i=0;i<3;i++){
        int y=60+i*50;
        bool sel=(i==g_save_slot_cursor);
        if(sel){int hy;for(hy=y-2;hy<y+12;hy++) mm_memset(&g_render_buf[hy*RENDER_W+20],1,RENDER_W-40);}
        char line[60];
        if(g_slot_info[i].exists){
            const char *mn=map_display_name(g_slot_info[i].map_idx);
            mm_snprintf(line,sizeof(line),"SLOT %d: %s",i+1,mn?mn:"Unknown");
        } else {
            mm_snprintf(line,sizeof(line),"SLOT %d: EMPTY",i+1);
        }
        font_print(g_render_buf,RENDER_W,line,30,y,sel?14:(g_slot_info[i].exists?7:8),1);
    }
    font_print(g_render_buf,RENDER_W,"[1-3]/UP-DN+ENTER to select  [ESC] cancel",
               20,RENDER_H-12,8,1);
}

/* ---- Character creation screen ---- */
static void draw_char_create_screen(void)
{
    static const char *CLS_ROWS[7]={"","1.Knight  ","2.Paladin ","3.Archer  ",
                                     "4.Cleric  ","5.Sorcerer","6.Robber  "};
    static const char *RCE_ROWS[6]={"","1.Human   ","2.Elf     ","3.Dwarf   ",
                                     "4.Gnome   ","5.Half-Orc"};
    static const char *SNM[7]={"INT","MIG","PER","END","SPD","ACC","LCK"};
    int i; uint16_t r;
    for(r=0;r<RENDER_H;r++) mm_memset(&g_render_buf[r*RENDER_W],0,RENDER_W);
    {const char *h="CREATE YOUR PARTY";int tw=font_str_width(h,2);
     font_print(g_render_buf,RENDER_W,h,(RENDER_W-tw)/2,4,14,2);}

    /* Party so far (left column) */
    if(g_cc_party.count>0){
        font_print(g_render_buf,RENDER_W,"Party:",4,26,11,1);
        for(i=0;i<g_cc_party.count;i++){
            char ml[24]; mm_snprintf(ml,sizeof(ml),"%d. %s",i+1,g_cc_party.members[i].name);
            font_print(g_render_buf,RENDER_W,ml,4,36+i*10,7,1);
        }
    }

    /* Right column — current step */
    int mx=RENDER_W/2, base_y=24;
    if(g_cc_member_idx>=MAX_PARTY){
        font_print(g_render_buf,RENDER_W,"PARTY COMPLETE!",mx,base_y,10,1);
        font_print(g_render_buf,RENDER_W,"[ENTER] Begin Adventure",mx,base_y+16,14,1);
    } else {
        char mhdr[30]; mm_snprintf(mhdr,sizeof(mhdr),"Member %d of 6:",g_cc_member_idx+1);
        font_print(g_render_buf,RENDER_W,mhdr,mx,base_y,11,1);
        if(g_cc_phase==0){
            font_print(g_render_buf,RENDER_W,"SELECT CLASS:",mx,base_y+14,15,1);
            for(i=1;i<=6;i++)
                font_print(g_render_buf,RENDER_W,CLS_ROWS[i],mx,base_y+24+i*10,
                           i==g_cc_cls?14:7,1);
        } else if(g_cc_phase==1){
            char cl[24]; mm_snprintf(cl,sizeof(cl),"Class: %s",CLS_ROWS[g_cc_cls]);
            font_print(g_render_buf,RENDER_W,cl,mx,base_y+14,7,1);
            font_print(g_render_buf,RENDER_W,"SELECT RACE:",mx,base_y+26,15,1);
            for(i=1;i<=5;i++)
                font_print(g_render_buf,RENDER_W,RCE_ROWS[i],mx,base_y+36+i*10,
                           i==g_cc_race?14:7,1);
        } else {
            char cl[40]; mm_snprintf(cl,sizeof(cl),"%s / %s",CLS_ROWS[g_cc_cls],RCE_ROWS[g_cc_race]);
            font_print(g_render_buf,RENDER_W,cl,mx,base_y+12,7,1);
            for(i=0;i<7;i++){
                char sl[14]; mm_snprintf(sl,sizeof(sl),"%s:%2d",SNM[i],g_cc_stats[i]);
                font_print(g_render_buf,RENDER_W,sl,mx+(i>=4?100:0),base_y+24+(i<4?i:i-4)*10,11,1);
            }
            char hs[16]; mm_snprintf(hs,sizeof(hs),"HP:%d  SP:%d",g_cc_hp,g_cc_sp);
            font_print(g_render_buf,RENDER_W,hs,mx,base_y+68,2,1);
            font_print(g_render_buf,RENDER_W,"[ENTER] Accept   [R] Reroll",mx,base_y+80,14,1);
        }
    }
    if(g_cc_party.count>=1)
        font_print(g_render_buf,RENDER_W,"[D] Done/Start  [ESC] Cancel",4,RENDER_H-12,8,1);
    else
        font_print(g_render_buf,RENDER_W,"Need at least 1 member.  [ESC] Cancel",4,RENDER_H-12,8,1);
}

/* ---- Pre-combat dialog ---- */
static void draw_pre_combat_screen(void)
{
    int g; uint16_t r;
    for(r=0;r<RENDER_H;r++) mm_memset(&g_render_buf[r*RENDER_W],0,RENDER_W);
    {const char *h="ENCOUNTER!"; int tw=font_str_width(h,2);
     font_print(g_render_buf,RENDER_W,h,(RENDER_W-tw)/2,6,4,2);}
    for(g=0;g<g_combat.n_groups;g++){
        MonsterGroup *mg=&g_combat.groups[g];
        char line[48]; int li=0;
        line[li++]=(char)('A'+g); line[li++]=':'; line[li++]=' ';
        const char *mn=mg->type->name; while(*mn&&li<32) line[li++]=*mn++;
        line[li++]=' '; line[li++]='x';
        if(mg->count>=10) line[li++]=(char)('0'+mg->count/10);
        line[li++]=(char)('0'+mg->count%10); line[li]='\0';
        font_print(g_render_buf,RENDER_W,line,60,50+g*14,12,1);
    }
    font_print(g_render_buf,RENDER_W,"[A]ttack  [F]lee  [G]old bribe",
               60,RENDER_H-30,14,1);
}

/* ---- Sage/help screen ---- */
static void draw_sage_screen(void)
{
    static const char * const SAGE_PAGES[7][14] = {
        { /* 1: Movement & Controls */
          "SAGE WISDOM  (1/7 - Controls)",
          "---",
          "MOVE:   W/Up=fwd  S/Dn=back",
          "        A=strafe L  D=strafe R",
          "        Q=turn L   E=turn R",
          "---",
          "ACTIONS:  B=bash  X=search",
          "          U=unlock  Z=rest",
          "          M=map overlay",
          "          P=save  F1=options",
          "          1-6=character sheet",
          "---",
          "H=this help  ESC=close",
          NULL
        },
        { /* 2: Combat */
          "SAGE WISDOM  (2/7 - Combat)",
          "---",
          "A=attack group A  B=group B  C=group C",
          "F=flee  G=bribe (costs gold, LCK check)",
          "S=cast spell  I=use backpack item",
          "---",
          "BLESS: +attack bonus for 10+ rounds",
          "SLEEP: monsters skip attacks each turn",
          "POISON: 1 HP/round until cured",
          "---",
          "Win: XP + gold + possible loot item",
          "Lose: revive at last town with 1 HP",
          "Cheat: F1 > C to auto-win battles",
          NULL
        },
        { /* 3: Spells */
          "SAGE WISDOM  (3/7 - Magic)",
          "---",
          "CLERIC spells (need SP + level):",
          "  Heal/Cure/Raise/Resurrect/Bless",
          "  Turn Undead  Holy Word  Portal",
          "  Protection From fire/cold/poison...",
          "---",
          "SORCERER spells:",
          "  Sleep  Haste  Fireball  Lightning",
          "  Fly/Escape  Teleport  Disintegrate",
          "---",
          "High-level spells cost gems (1-2)",
          "S in combat opens spell menu",
          NULL
        },
        { /* 4: Services */
          "SAGE WISDOM  (4/7 - Services)",
          "---",
          "INN:       rest (costs food + gold)",
          "TEMPLE:    cure 50g  raise 500g",
          "           resurrect eradicated 1000g",
          "TRAINING:  level up for level x 1000g",
          "BLACKSMITH: B=buy  S=sell  E=sell equip",
          "FOOD SHOP: 10 rations for 10g",
          "TAVERN:    rumours + stat drink (5g/+1)",
          "---",
          "Yellow=inn  Cyan=temple  Green=training",
          "Orange=blacksmith  Brown=tavern",
          "Step on tile then press Y or ENTER",
          NULL
        },
        { /* 5: Items & Equipment */
          "SAGE WISDOM  (5/7 - Items)",
          "---",
          "Equipment slots: weapon/ranged/staff",
          "                 armor/shield/misc",
          "---",
          "Equipped items give AC and damage bonus",
          "Special items give stat bonuses (+INT etc)",
          "Potions/wands: I in combat to use",
          "Wands have charges (spell_id effect)",
          "---",
          "SELL: Blacksmith E key (equipped)",
          "      Blacksmith S key (backpack)",
          "      Charsheet S key (equipped char)",
          NULL
        },
        { /* 6: Exploration */
          "SAGE WISDOM  (6/7 - World)",
          "---",
          "5 towns  ->  5 dungeons (caves/etc)",
          "Outdoor areas: 4x5 grid, walk off edge",
          "---",
          "TRAPS: pit (Levitate bypasses)",
          "       poison gas / acid / stalactites",
          "CHESTS: Robber can disarm (thievery)",
          "SHRINES: step+Y for stat/gem/heal bonus",
          "DOORS: U to unlock  B to bash",
          "DARK rooms: Lasting Light / Light spell",
          "---",
          "Starvation: 1 HP every 15 steps at 0 food",
          NULL
        },
        { /* 7: Tips */
          "SAGE WISDOM  (7/7 - Tips & Secrets)",
          "---",
          "The courier quest (Sorpigal->Erliquin)",
          "gives huge XP - do it early",
          "---",
          "Cave 1 arena: fight for 500 XP/member",
          "Pirate Cove (Area A-2, 15,7): Coral Key",
          "King's Pass (Erliquin 13,6): 5000g pass",
          "Pyramid 4 levels: 500 XP each first visit",
          "---",
          "Save often. Robber in party = 70% unlock",
          "Levitate protects from all pit traps",
          "Bribe works best with high LCK characters",
          NULL
        }
    };
    int i; uint16_t r;
    for(r=0;r<RENDER_H;r++) mm_memset(&g_render_buf[r*RENDER_W],0,RENDER_W);
    int pg = g_sage_page < 7 ? g_sage_page : 0;
    const char * const *lines = SAGE_PAGES[pg];
    for(i=0; lines[i] && i<14; i++){
        const char *ln = lines[i];
        uint8_t col = (i==0)?14:(ln[0]=='-')?8:7;
        if(i==0){ int tw=font_str_width(ln,1);
                  font_print(g_render_buf,RENDER_W,ln,(RENDER_W-tw)/2,4,col,1); }
        else if(ln[0]=='-') mm_memset(&g_render_buf[(14+i*12)*RENDER_W+10],8,RENDER_W-20);
        else font_print(g_render_buf,RENDER_W,ln,10,14+i*12,col,1);
    }
    font_print(g_render_buf,RENDER_W,"LEFT/RIGHT to turn pages  ESC or H to close",
               10,RENDER_H-12,8,1);
}

/* ---- Rendering ---- */
void mm_port_draw(void){
    int16_t cx,cy,cw,ch,bx,by,bw,bh;
    if(!g_open) return;

    wnd_draw_frame(&g_frame,"MIGHT & MAGIC I");
    wnd_content_rect(&g_frame,0,&cx,&cy,&cw,&ch);

    if(g_mode==GM_TITLE){
        draw_title_screen();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
    } else if(g_mode==GM_TOWN_SELECT){
        draw_town_select_screen();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
    } else if(g_mode==GM_DEFEAT){
        draw_defeat_screen();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
    } else if(g_mode==GM_CHARSHEET){
        /* Render explore view as background, then charsheet overlay */
        if(g_game_ready){
            const struct Map *m=&g_maps[g_gs.map_idx];
            render_3d_view(g_view3d_buf,VIEW3D_W,0,0,m,g_gs.player.x,g_gs.player.y,g_gs.player.facing,0,0);
            uint16_t rr;
            for(rr=0;rr<RENDER_H;rr++) mm_memset(&g_render_buf[rr*RENDER_W],0,RENDER_W);
            hud_draw_chrome(g_render_buf);
            hud_draw_minimap(g_render_buf,m,&g_gs.player);
            hud_draw_compass(g_render_buf,&g_gs.player,&g_gs,m);
            hud_draw_status(g_render_buf,&g_gs,m);
            hud_draw_party(g_render_buf,&g_gs);
        }
        draw_charsheet();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
    } else if(g_mode==GM_COMBAT){
        draw_combat_screen();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
        blit_opaque_pixels(cx,cy,g_view3d_buf,VIEW3D_W,0,0,VIEW3D_W,VIEW3D_H,2);
    } else if(g_mode==GM_OPTIONS||g_mode==GM_QUIT_CONFIRM){
        /* Render the underlying game state first, then overlay */
        if(g_game_ready&&g_prev_mode==GM_EXPLORE){
            const struct Map *m=&g_maps[g_gs.map_idx];
            int outdoor=(g_gs.map_idx>=14&&g_gs.map_idx<=33);
            int dark=m->cells[g_gs.player.y][g_gs.player.x].dark;
            render_3d_view(g_view3d_buf,VIEW3D_W,0,0,m,
                           g_gs.player.x,g_gs.player.y,g_gs.player.facing,outdoor,dark);
            uint16_t rr;
            for(rr=0;rr<RENDER_H;rr++) mm_memset(&g_render_buf[rr*RENDER_W],0,RENDER_W);
            hud_draw_chrome(g_render_buf);
            hud_draw_minimap(g_render_buf,m,&g_gs.player);
            hud_draw_compass(g_render_buf,&g_gs.player,&g_gs,m);
            hud_draw_status(g_render_buf,&g_gs,m);
            hud_draw_party(g_render_buf,&g_gs);
        }
        if(g_mode==GM_OPTIONS)      draw_options_screen();
        else                         draw_quit_confirm();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
        /* No 3D sub-buf blit — overlay must not be covered */
    } else if(g_mode==GM_SHOP){
        draw_shop_screen();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
    } else if(g_mode==GM_MAP){
        draw_map_screen();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
    } else if(g_mode==GM_SPELL_SELECT){
        draw_spell_screen();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
    } else if(g_mode==GM_COMBAT_ITEM){
        draw_combat_item_screen();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
    } else if(g_mode==GM_SAVE_SLOT){
        draw_save_slot_screen();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
    } else if(g_mode==GM_CHAR_CREATE){
        draw_char_create_screen();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
    } else if(g_mode==GM_PRE_COMBAT){
        draw_pre_combat_screen();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
    } else if(g_mode==GM_SAGE){
        draw_sage_screen();
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
    } else if(g_game_ready){
        const struct Map *m=&g_maps[g_gs.map_idx];
        int outdoor=(g_gs.map_idx>=14&&g_gs.map_idx<=33);
        int dark=m->cells[g_gs.player.y][g_gs.player.x].dark;

        /* Render 3D into sub-buffer */
        render_3d_view(g_view3d_buf,VIEW3D_W,0,0,m,
                       g_gs.player.x,g_gs.player.y,g_gs.player.facing,outdoor,dark);

        /* Clear main buffer, draw HUD chrome + panels */
        uint16_t rr;
        for(rr=0;rr<RENDER_H;rr++) mm_memset(&g_render_buf[rr*RENDER_W],0,RENDER_W);
        hud_draw_chrome(g_render_buf);
        hud_draw_minimap(g_render_buf,m,&g_gs.player);
        hud_draw_compass(g_render_buf,&g_gs.player,&g_gs,m);
        hud_draw_status(g_render_buf,&g_gs,m);
        hud_draw_party(g_render_buf,&g_gs);

        /* Blit HUD at scale=1 */
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
        blit_opaque_pixels(cx,cy+17,    /* V_Y=17: below title bar */
                           g_view3d_buf,VIEW3D_W,0,0,VIEW3D_W,VIEW3D_H,2);
    } else {
        uint16_t rr;
        for(rr=0;rr<RENDER_H;rr++) mm_memset(&g_render_buf[rr*RENDER_W],rr<RENDER_H/2?1:2,RENDER_W);
        font_print(g_render_buf,RENDER_W,"Loading B:/MM1/...",4,RENDER_H/2,8,1);
        blit_opaque_pixels(cx,cy,g_render_buf,RENDER_W,0,0,RENDER_W,RENDER_H,1);
    }

    wnd_display_bounds(&g_frame,&bx,&by,&bw,&bh);
    present_region(bx,by,bw,bh);
    g_needs_redraw=false;
}

bool mm_port_update(void){
    uint32_t now = pit_millis();

    /* SFX overrides background music */
    if (g_sfx_seq && g_sfx_idx < g_sfx_len) {
        if (now >= g_sfx_next) {
            const MusicNote *n = &g_sfx_seq[g_sfx_idx++];
            if (n->hz) speaker_note_on(n->hz); else speaker_note_off();
            g_sfx_next = now + n->ms;
        }
        if (g_sfx_idx >= g_sfx_len) { g_sfx_seq=NULL; speaker_note_off(); g_mus_next=now; }
    } else if (g_music_en && g_mus_seq) {
        if (now >= g_mus_next) {
            if (g_mus_idx >= g_mus_len) g_mus_idx = 0;
            const MusicNote *n = &g_mus_seq[g_mus_idx++];
            if (n->hz) speaker_note_on(n->hz); else speaker_note_off();
            g_mus_next = now + n->ms;
        }
    }
    /* Auto-start music on title screen only */
    if (g_music_en && !g_mus_seq && g_mode==GM_TITLE)
        music_start(MM_TITLE, MM_TITLE_LEN);

    if (g_mode == GM_TITLE) {
        if (g_title_next_change == 0) g_title_next_change = now + 4000;
        if (now >= g_title_next_change) {
            g_title_idx = (g_title_idx + 1) % 10;
            g_title_next_change = now + 4000;
        }
        return true;
    }
    if (g_mode==GM_EXPLORE&&g_game_ready&&g_gs.player.msg_ticks>0){
        player_tick(&g_gs.player); return true;
    }
    return g_needs_redraw;
}

/* ---- Title / town select helpers ---- */
static void title_exit(void){
    speaker_note_off(); g_sfx_seq=NULL; g_mus_idx=0;
    g_title_idx=-1;
    g_town_cursor=0;  /* always start at 0 — Continue is always option 0 */
    g_mode=GM_TOWN_SELECT; g_needs_redraw=true;
}

static void town_select_confirm(int idx){
    extern void game_state_set_town(GameState*,int);
    if(idx==0){
        /* Continue — only if save exists */
        if(g_save_exists && mm_load_game()){ g_mode=GM_EXPLORE; g_needs_redraw=true; return; }
        if(!g_save_exists){ return; } /* ignore if no save */
    }
    int town=idx-1; /* idx 1-5 → town 0-4 */
    if(town<0||town>4) town=0;
    game_state_set_town(&g_gs,town);
    mm_load_ovr_for_map(g_gs.map_idx);
    g_mode=GM_EXPLORE; g_needs_redraw=true;
}

/* ---- Combat input ---- */
static void do_combat_key(uint8_t sc){
    if(g_combat.over){
        if(sc==0x1C||sc==0x39||sc==0x3B){
            if(g_combat.result==CR_VICTORY){
                sfx_play(MM_VICTORY,MM_VICTORY_LEN);
                combat_reward(&g_combat,&g_gs);
                combat_loot(&g_combat,&g_gs);
                if(g_combat.n_found_items>0){
                    const char *iname=item_name(g_combat.found_items[0]);
                    char lmsg[40]; int li=0;
                    const char *fp="Found: "; while(*fp&&li<10) lmsg[li++]=(char)*fp++;
                    while(*iname&&li<38) lmsg[li++]=(char)*iname++;
                    lmsg[li]=0;
                    player_set_message(&g_gs.player,lmsg,120);
                }
                check_level_ups();
            }
            if(g_combat.result==CR_DEFEAT) g_mode=GM_DEFEAT;
            else {
                g_mode=GM_EXPLORE;
                /* Auto-search tile after victory */
                if(g_gs.auto_search && g_ovr_loaded){
                    const char *at=ovr_text_at(&g_ovr,g_gs.player.x,g_gs.player.y,g_gs.player.facing);
                    handle_tile_event(&g_gs,&g_ovr,at);
                }
            }
            g_needs_redraw=true;
        }
        return;
    }
    /* A=attack group A, 0x30=B, 0x2E=C */
    if(sc==0x1E||sc==0x1C||sc==0x30||sc==0x2E){
        int tgt=-1;
        if(sc==0x30&&g_combat.n_groups>1) tgt=1;
        else if(sc==0x2E&&g_combat.n_groups>2) tgt=2;
        g_combat.target_group=tgt;
        bool ended=combat_round_targeted(&g_combat,&g_gs,tgt);
        if(g_combat.n_hits_dealt>0) sfx_play(MM_HIT,MM_HIT_LEN);
        g_needs_redraw=true;
        if(ended&&g_combat.result==CR_DEFEAT){
            g_mode=GM_DEFEAT; sfx_play(MM_DEFEAT,MM_DEFEAT_LEN);
        }
    } else if(sc==0x21){ combat_flee(&g_combat,&g_gs); g_needs_redraw=true; }
    else if(sc==0x1F){ /* S: open spell menu */
        extern int spells_for_class_level(int,int,int*,int);
        g_spell_count=0; int i2;
        for(i2=0;i2<g_gs.party.count&&g_spell_count<20;i2++){
            Character *c=&g_gs.party.members[i2];
            if(c->sp<=0||IS_DEADLIKE(c->condition)) continue;
            int lv; int cls=c->cls;
            if(cls!=4&&cls!=5&&cls!=2&&cls!=3) continue;
            for(lv=1;lv<=c->level&&g_spell_count<20;lv++)
                g_spell_count+=spells_for_class_level(cls,lv,
                    g_spell_ids+g_spell_count,20-g_spell_count);
            break;
        }
        /* Filter out non-combat spells */
        { int new_cnt=0,zi;
          for(zi=0;zi<g_spell_count;zi++){
              const SpellDef *sp2=spell_get(g_spell_ids[zi]);
              if(sp2&&!(sp2->flags&SPELL_FLAG_NONCOMBAT))
                  g_spell_ids[new_cnt++]=g_spell_ids[zi];
          }
          g_spell_count=new_cnt; }
        if(g_spell_count>0){ g_mode=GM_SPELL_SELECT; g_needs_redraw=true; }
        else player_set_message(&g_gs.player,"NO SPELLS AVAILABLE.",60);
    } else if(sc==0x17){ /* I: open item use menu */
        mm_collect_combat_items();
        if(g_citem_count>0){ g_mode=GM_COMBAT_ITEM; g_needs_redraw=true; }
        else player_set_message(&g_gs.player,"NO USABLE ITEMS.",60);
    } else if(sc==0x22){ /* G: offer gold bribe */
        extern int monster_roll(int);
        int gi3=0; while(gi3<g_combat.n_groups&&!g_combat.groups[gi3].alive) gi3++;
        if(gi3<g_combat.n_groups){
            int cost=g_combat.groups[gi3].type->level*10+5;
            if(g_gs.party.count>0&&(int)g_gs.party.members[0].gold>=cost){
                g_gs.party.members[0].gold-=(uint32_t)cost;
                int lck=0,pi3;
                for(pi3=0;pi3<g_gs.party.count;pi3++)
                    if(!IS_DEADLIKE(g_gs.party.members[pi3].condition)&&
                       g_gs.party.members[pi3].stats[6]>lck)
                        lck=g_gs.party.members[pi3].stats[6];
                int lmod=(lck-10)/2; if(lmod<0)lmod=0;
                if(monster_roll(20)+lmod>=12){
                    combat_flee(&g_combat,&g_gs);
                    player_set_message(&g_gs.player,"BRIBED! MONSTERS FLEE.",90);
                } else {
                    player_set_message(&g_gs.player,"BRIBE FAILED! ATTACK!",90);
                }
            } else {
                char bmsg[32]; int bi=0;
                const char *bp="NEED "; while(*bp) bmsg[bi++]=(char)*bp++;
                int cv2=g_combat.groups[gi3].type->level*10+5;
                if(cv2>=100) bmsg[bi++]=(char)('0'+cv2/100);
                if(cv2>=10)  bmsg[bi++]=(char)('0'+(cv2/10)%10);
                bmsg[bi++]=(char)('0'+cv2%10);
                const char *bq="g TO BRIBE."; while(*bq) bmsg[bi++]=(char)*bq++;
                bmsg[bi]='\0';
                player_set_message(&g_gs.player,bmsg,60);
            }
        }
        g_needs_redraw=true;
    }
}

/* ---- Random encounter ---- */
static void maybe_encounter(void){
    extern int monster_roll(int sides);
    int map=g_gs.map_idx;
    if(map>=0&&map<=4) return;
    int rate=g_ovr_loaded?g_ovr.constants.encounter_rand:0;
    if(map>=5&&map<=13) rate=(rate<15)?15:rate;
    if(map>=14&&map<=33) rate=(rate<8)?8:rate;
    if(map>=34) rate=(rate<20)?20:rate;
    if(monster_roll(100)<=rate) mm_start_combat();
}

/* ---- Movement ---- */
static void do_move(uint8_t sc){
    if(!g_game_ready) return;
    const struct Map *m=&g_maps[g_gs.map_idx];
    int old_map=g_gs.map_idx,moved=0;
    switch(sc){
        case 0x11:case 0xC8: moved=player_forward(&g_gs.player,m);  break;
        case 0x1F:case 0xD0: moved=player_backward(&g_gs.player,m); break;
        case 0xCB:case 0x10: player_turn_l(&g_gs.player);moved=1;   break;
        case 0xCD:case 0x12: player_turn_r(&g_gs.player);moved=1;   break;
        case 0x20: moved=player_strafe_r(&g_gs.player,m);            break;
        case 0x1E: moved=player_strafe_l(&g_gs.player,m);            break;
        default: return;
    }
    if(!moved){ sfx_play(MM_BUMP,MM_BUMP_LEN); g_needs_redraw=true;return; }
    /* step sound for forward/back/strafe, not turns */
    if(sc==0x11||sc==0xC8||sc==0x1F||sc==0xD0||sc==0x20||sc==0x1E)
        sfx_play(MM_STEP,MM_STEP_LEN);

    if(g_ovr_loaded){
        const char *ovr_txt=ovr_text_at(&g_ovr,g_gs.player.x,g_gs.player.y,g_gs.player.facing);
        /* peek_tile_event covers BOTH OVR scripts AND tiles_all.inc manual entries */
        const char *display_txt=NULL;
        ServiceType svc=peek_tile_event(&g_gs,ovr_txt,&display_txt);
        if(display_txt&&*display_txt)
            player_set_message(&g_gs.player,display_txt,120);
        else if(ovr_txt&&*ovr_txt)
            player_set_message(&g_gs.player,ovr_txt,120);
        handle_floor_trap(&g_gs,&g_ovr);
        if(g_mode==GM_EXPLORE&&svc==SVC_ENCOUNTER){mm_start_combat();return;}
    }
    if(g_gs.map_idx>=14&&g_gs.map_idx<=33){
        int nx,ny;
        int nm=area_edge_transition(&g_gs,&g_maps[g_gs.map_idx],&nx,&ny);
        if(nm>=0){g_gs.map_idx=nm;player_init(&g_gs.player,nx,ny,g_gs.player.facing);}
    }
    if(g_gs.map_idx!=old_map) mm_load_ovr_for_map(g_gs.map_idx);
    /* Starvation */
    if(g_gs.party.shared_food==0){
        if(++g_hunger_steps>=15){
            g_hunger_steps=0;
            int i4;
            for(i4=0;i4<g_gs.party.count;i4++)
                if(!IS_DEADLIKE(g_gs.party.members[i4].condition))
                    g_gs.party.members[i4].hp--;
            player_set_message(&g_gs.player,"STARVING! PARTY HUNGERS!",60);
        }
    } else g_hunger_steps=0;

    if(g_mode==GM_EXPLORE) maybe_encounter();
    /* Shop trigger (set by blacksmith event via BROWSE_SHOP message) */
    if(mm_namecmp(g_gs.player.message,"BROWSE_SHOP")==0){
        player_set_message(&g_gs.player,"",0);
        g_shop_town=(g_gs.map_idx>=0&&g_gs.map_idx<=4)?g_gs.map_idx:0;
        g_shop_scroll=0; g_prev_mode=GM_EXPLORE; g_mode=GM_SHOP;
    }
    g_needs_redraw=true;
}

/* ---- Input ---- */
bool mm_port_scancode(uint8_t sc){
    if(!g_open) return false;
    if(g_mode==GM_TITLE){title_exit();return true;}

    /* Defeat screen — any key respawns in current town (or Sorpigal if in dungeon/outdoors) */
    if(g_mode==GM_DEFEAT){
        int sx=8, sy=3;
        if(g_gs.map_idx>4){
            g_gs.map_idx=0;  /* dungeon/outdoors → Sorpigal; sx/sy stay as defaults */
        } else {
            /* already in a town — use its loaded OVR safe coords */
            if(g_ovr_loaded&&g_ovr.constants.safe_x>0) sx=g_ovr.constants.safe_x;
            if(g_ovr_loaded&&g_ovr.constants.safe_y>0) sy=g_ovr.constants.safe_y;
        }
        player_init(&g_gs.player,sx,sy,0);
        /* Revive all party members with 1 HP (mercy respawn) */
        {int i; for(i=0;i<g_gs.party.count;i++){
            Character *c=&g_gs.party.members[i];
            if(IS_DEADLIKE(c->condition)) c->condition=0;
            if(c->hp<=0) c->hp=1;
        }}
        mm_load_ovr_for_map(g_gs.map_idx);
        g_mode=GM_EXPLORE; g_needs_redraw=true; return true;
    }

    if(g_mode==GM_CHARSHEET){
        if(sc==0x01||sc==0x3B){g_mode=g_prev_mode;g_needs_redraw=true;return true;}
        if(sc>=0x02&&sc<=0x07){ /* 1-6 */
            int idx=sc-0x02;
            if(idx<g_gs.party.count){g_charsheet_idx=idx;g_needs_redraw=true;}
            return true;
        }
        if(sc==0xC8&&g_charsheet_idx>0){g_charsheet_idx--;g_needs_redraw=true;return true;}
        if(sc==0xD0&&g_charsheet_idx<g_gs.party.count-1){g_charsheet_idx++;g_needs_redraw=true;return true;}
        if(sc==0x1F){ /* S: sell first equipped item of current character */
            Character *cc=&g_gs.party.members[g_charsheet_idx];
            int s2;
            for(s2=0;s2<6;s2++) if(cc->equipped[s2]>0){
                const ItemDef *it=item_get(cc->equipped[s2]);
                int val=it?it->cost/2:0;
                if(val>0) g_gs.party.members[0].gold+=(uint32_t)val;
                cc->equipped[s2]=0;
                mm_apply_equip_bonuses(&g_gs.party);
                player_set_message(&g_gs.player,"ITEM SOLD.",60);
                g_needs_redraw=true; return true;
            }
            player_set_message(&g_gs.player,"NOTHING EQUIPPED TO SELL.",60);
            g_needs_redraw=true; return true;
        }
        return true;
    }

    /* Shop mode */
    if(g_mode==GM_SHOP){
        if(sc==0x01||sc==0x4D){ g_mode=g_prev_mode; g_needs_redraw=true; return true; }
        if(sc==0xC8&&g_shop_scroll>0){g_shop_scroll--;g_needs_redraw=true;return true;}
        if(sc==0xD0){ int cnt2=0; shop_stock(g_shop_town,&cnt2);
            if(g_shop_scroll<cnt2-1){g_shop_scroll++;g_needs_redraw=true;} return true; }
        if(sc==0x30){ /* B: buy */
            int cnt3=0; const int *st=shop_stock(g_shop_town,&cnt3);
            if(st&&g_shop_scroll<cnt3){
                int id=st[g_shop_scroll]; const ItemDef *it=item_get(id);
                if(it&&g_gs.party.count>0&&(int)g_gs.party.members[0].gold>=it->cost){
                    /* Class restriction check */
                    if(it->class_mask!=0xFF){
                        int can_use=0,i2c;
                        for(i2c=0;i2c<g_gs.party.count;i2c++){
                            int cls=g_gs.party.members[i2c].cls;
                            if(cls>=1&&cls<=6&&(it->class_mask&(1<<(cls-1)))) {can_use=1;break;}
                        }
                        if(!can_use){player_set_message(&g_gs.player,"NO ONE CAN USE THIS.",60);g_needs_redraw=true;return true;}
                    }
                    int i2,s2;
                    for(i2=0;i2<g_gs.party.count;i2++){
                        for(s2=0;s2<6;s2++) if(g_gs.party.members[i2].backpack[s2]==0){
                            g_gs.party.members[i2].backpack[s2]=id;
                            g_gs.party.members[0].gold-=(uint32_t)it->cost;
                            player_set_message(&g_gs.player,"PURCHASED!",60);
                            g_needs_redraw=true; return true;
                        }
                    }
                    player_set_message(&g_gs.player,"BACKPACKS FULL!",60);
                } else player_set_message(&g_gs.player,"NOT ENOUGH GOLD.",60);
            }
            g_needs_redraw=true; return true;
        }
        if(sc==0x1F){ /* S: sell first backpack item */
            int i3,s3;
            for(i3=0;i3<g_gs.party.count;i3++){
                for(s3=0;s3<6;s3++) if(g_gs.party.members[i3].backpack[s3]>0){
                    const ItemDef *it=item_get(g_gs.party.members[i3].backpack[s3]);
                    int val=it?it->cost/2:0;
                    if(val>0) g_gs.party.members[0].gold+=(uint32_t)val;
                    g_gs.party.members[i3].backpack[s3]=0;
                    player_set_message(&g_gs.player,"ITEM SOLD.",60);
                    g_needs_redraw=true; return true;
                }
            }
            player_set_message(&g_gs.player,"NOTHING TO SELL.",60);
            g_needs_redraw=true; return true;
        }
        if(sc==0x12){ /* E: sell first equipped item */
            int i3e,s3e;
            for(i3e=0;i3e<g_gs.party.count;i3e++){
                for(s3e=0;s3e<6;s3e++) if(g_gs.party.members[i3e].equipped[s3e]>0){
                    const ItemDef *it=item_get(g_gs.party.members[i3e].equipped[s3e]);
                    int val=it?it->cost/2:0;
                    if(val>0) g_gs.party.members[0].gold+=(uint32_t)val;
                    g_gs.party.members[i3e].equipped[s3e]=0;
                    mm_apply_equip_bonuses(&g_gs.party);
                    player_set_message(&g_gs.player,"EQUIPPED ITEM SOLD.",60);
                    g_needs_redraw=true; return true;
                }
            }
            player_set_message(&g_gs.player,"NOTHING EQUIPPED TO SELL.",60);
            g_needs_redraw=true; return true;
        }
        return true;
    }
    /* Map mode */
    if(g_mode==GM_MAP){
        if(sc==0x01||sc==0x32){ g_mode=g_prev_mode; g_needs_redraw=true; return true; }
        return true;
    }
    /* Spell select */
    if(g_mode==GM_SPELL_SELECT){
        if(sc==0x01){ g_mode=GM_COMBAT; g_needs_redraw=true; return true; }
        if(sc>=0x02&&sc<=0x0A){
            int idx=sc-0x02;
            if(idx<g_spell_count){
                const SpellDef *sp=spell_get(g_spell_ids[idx]);
                if(sp){
                    int i4;
                    for(i4=0;i4<g_gs.party.count;i4++){
                        Character *c=&g_gs.party.members[i4];
                        if(c->sp<sp->sp_cost||IS_DEADLIKE(c->condition)) continue;
                        if(c->cls!=4&&c->cls!=5&&c->cls!=2&&c->cls!=3) continue;
                        c->sp-=sp->sp_cost;
                        if(sp->gem_cost>0&&c->gems>=sp->gem_cost) c->gems-=sp->gem_cost;
                        mm_cast_spell(sp,c);
                        break;
                    }
                }
            }
            /* portal/escape may have already set g_mode=GM_EXPLORE */
            if(g_mode==GM_SPELL_SELECT) g_mode=GM_COMBAT;
            g_needs_redraw=true; return true;
        }
        return true;
    }
    /* Combat item use */
    if(g_mode==GM_COMBAT_ITEM){
        if(sc==0x01){ g_mode=GM_COMBAT; g_needs_redraw=true; return true; }
        if(sc>=0x02&&sc<=0x0A){
            int idx=sc-0x02;
            if(idx<g_citem_count){
                int item_id=g_citem_ids[idx];
                int ci=g_citem_char[idx], cs2=g_citem_slot[idx];
                const ItemDef *it=item_get(item_id);
                if(it&&it->spell_id>0){
                    const SpellDef *sp=spell_get(it->spell_id);
                    if(sp){
                        mm_cast_spell(sp,&g_gs.party.members[ci]);
                        g_gs.party.members[ci].backpack[cs2]=0;
                    } else {
                        player_set_message(&g_gs.player,"NOTHING HAPPENS.",60);
                    }
                }
            }
            if(g_mode==GM_COMBAT_ITEM) g_mode=GM_COMBAT;
            g_needs_redraw=true; return true;
        }
        return true;
    }
    /* Pre-combat dialog */
    if(g_mode==GM_PRE_COMBAT){
        if(sc==0x1E||sc==0x1C||sc==0x39){ /* A or Enter: attack */
            g_mode=GM_COMBAT; g_needs_redraw=true; return true;
        }
        if(sc==0x21){ /* F: flee before combat */
            extern int monster_roll(int);
            combat_flee(&g_combat,&g_gs);
            if(g_combat.over) g_mode=GM_EXPLORE;
            else { player_set_message(&g_gs.player,"CAN'T ESCAPE!",60); g_mode=GM_COMBAT; }
            g_needs_redraw=true; return true;
        }
        if(sc==0x22){ /* G: bribe before combat */
            extern int monster_roll(int);
            int gi3=0;
            int cost=g_combat.groups[gi3].type->level*10+5;
            if(g_gs.party.count>0&&(int)g_gs.party.members[0].gold>=cost){
                g_gs.party.members[0].gold-=(uint32_t)cost;
                int lck=0,pi3;
                for(pi3=0;pi3<g_gs.party.count;pi3++)
                    if(!IS_DEADLIKE(g_gs.party.members[pi3].condition)&&
                       g_gs.party.members[pi3].stats[6]>lck)
                        lck=g_gs.party.members[pi3].stats[6];
                int lmod=(lck-10)/2; if(lmod<0)lmod=0;
                if(monster_roll(20)+lmod>=12){
                    g_combat.over=true; g_combat.result=CR_FLED;
                    player_set_message(&g_gs.player,"BRIBED! MONSTERS FLEE.",90);
                    g_mode=GM_EXPLORE;
                } else {
                    player_set_message(&g_gs.player,"BRIBE FAILED!",60);
                    g_mode=GM_COMBAT;
                }
            } else {
                player_set_message(&g_gs.player,"NOT ENOUGH GOLD TO BRIBE.",60);
                g_mode=GM_COMBAT;
            }
            g_needs_redraw=true; return true;
        }
        return true;
    }
    /* Sage help */
    if(g_mode==GM_SAGE){
        if(sc==0x01||sc==0x23){ g_mode=g_prev_mode; g_needs_redraw=true; return true; }
        if(sc==0xCB&&g_sage_page>0){ g_sage_page--; g_needs_redraw=true; return true; }
        if(sc==0xCD&&g_sage_page<6){ g_sage_page++; g_needs_redraw=true; return true; }
        if(sc==0xC8&&g_sage_page>0){ g_sage_page--; g_needs_redraw=true; return true; }
        if(sc==0xD0&&g_sage_page<6){ g_sage_page++; g_needs_redraw=true; return true; }
        return true;
    }
    /* Save slot picker */
    if(g_mode==GM_SAVE_SLOT){
        if(sc==0x01){ g_mode=g_prev_mode; g_needs_redraw=true; return true; }
        if(sc==0xC8&&g_save_slot_cursor>0){g_save_slot_cursor--;g_needs_redraw=true;return true;}
        if(sc==0xD0&&g_save_slot_cursor<2){g_save_slot_cursor++;g_needs_redraw=true;return true;}
        if(sc>=0x02&&sc<=0x04){ g_save_slot_cursor=sc-0x02; sc=0x1C; } /* 1-3 → select */
        if(sc==0x1C||sc==0x39){
            if(g_save_slot_mode==0){
                mm_save_game_slot(g_save_slot_cursor);
            } else {
                if(g_slot_info[g_save_slot_cursor].exists){
                    mm_load_game_slot(g_save_slot_cursor);
                    g_mode=g_prev_mode;
                } else {
                    player_set_message(&g_gs.player,"SLOT IS EMPTY.",60);
                }
            }
            if(g_mode==GM_SAVE_SLOT) g_mode=g_prev_mode;
            g_needs_redraw=true; return true;
        }
        return true;
    }
    /* Character creation */
    if(g_mode==GM_CHAR_CREATE){
        if(sc==0x01){ g_mode=GM_TOWN_SELECT; g_needs_redraw=true; return true; }
        if(sc==0x20&&g_cc_party.count>=1){ /* D: done */
            extern void game_state_set_town(GameState*,int);
            g_gs.party=g_cc_party; g_gs.party.shared_food=10;
            game_state_set_town(&g_gs,0);
            mm_load_ovr_for_map(g_gs.map_idx);
            g_mode=GM_EXPLORE; g_needs_redraw=true; return true;
        }
        if(g_cc_phase==0){
            if(sc>=0x02&&sc<=0x07){
                g_cc_cls=sc-0x01; g_cc_phase=1; g_needs_redraw=true;
            }
        } else if(g_cc_phase==1){
            if(sc>=0x02&&sc<=0x06){
                g_cc_race=sc-0x01; cc_roll_stats(); g_cc_phase=2; g_needs_redraw=true;
            }
        } else {
            if(sc==0x1C||sc==0x39){ /* Enter: accept */
                cc_accept();
                if(g_cc_member_idx>=MAX_PARTY){
                    extern void game_state_set_town(GameState*,int);
                    g_gs.party=g_cc_party; g_gs.party.shared_food=10;
                    game_state_set_town(&g_gs,0);
                    mm_load_ovr_for_map(g_gs.map_idx);
                    g_mode=GM_EXPLORE;
                }
                g_needs_redraw=true;
            } else if(sc==0x13){ /* R: reroll */
                cc_roll_stats(); g_needs_redraw=true;
            }
        }
        return true;
    }
    if(g_mode==GM_TOWN_SELECT){
        if(sc>=0x02&&sc<=0x06){ town_select_confirm(sc-0x01); } /* key 1→idx1, key2→idx2... */
        else if(sc==0xC8){g_town_cursor=(g_town_cursor>0)?g_town_cursor-1:5;g_needs_redraw=true;}
        else if(sc==0xD0){g_town_cursor=(g_town_cursor<5)?g_town_cursor+1:0;g_needs_redraw=true;}
        else if(sc==0x1C||sc==0x39) town_select_confirm(g_town_cursor);
        else if(sc==0x31){ mm_enter_char_create(); } /* N: new party */
        return true;
    }
    if(g_mode==GM_COMBAT){do_combat_key(sc);return g_needs_redraw;}

    /* Options menu */
    if(g_mode==GM_OPTIONS){
        if(sc==0x01||sc==0x2D||sc==0x3B){ /* ESC/X/F1: close */
            g_mode=g_prev_mode; g_needs_redraw=true; return true;
        }
        if(sc==0x32){ /* M: toggle music */
            g_music_en=!g_music_en;
            if(!g_music_en){

                speaker_note_off();
            }
            g_needs_redraw=true; return true;
        }
        if(sc==0x1F){ /* S: toggle SFX */ g_sfx_en=!g_sfx_en; g_needs_redraw=true; return true; }
        if(sc==0x2E){ /* C: cheat */ g_gs.cheat_combat=!g_gs.cheat_combat; g_needs_redraw=true; return true; }
        if(sc==0x1E){ /* A: auto-search */ g_gs.auto_search=!g_gs.auto_search; g_needs_redraw=true; return true; }
        if(sc==0x19){ /* P: save */ mm_enter_save_slot(0); return true; }
        if(sc==0x26){ /* L: load */ mm_enter_save_slot(1); return true; }
        if(sc==0x10){ /* Q: quit → confirm */
            g_prev_mode=GM_OPTIONS; g_mode=GM_QUIT_CONFIRM; g_needs_redraw=true; return true;
        }
        return true;
    }

    /* Quit confirmation */
    if(g_mode==GM_QUIT_CONFIRM){
        if(sc==0x15||sc==0x59){ /* Y: yes (scancode Y or key y) */ mm_port_close(); return true; }
        if(sc==0x31||sc==0x01){ /* N or ESC: no */
            g_mode=g_prev_mode; g_needs_redraw=true; return true;
        }
        return true;
    }

    /* F1: open options from any in-game mode */
    if(sc==0x3B&&(g_mode==GM_EXPLORE||g_mode==GM_COMBAT)){
        g_prev_mode=g_mode; g_mode=GM_OPTIONS; g_needs_redraw=true; return true;
    }

    if(sc==0x01) return false; /* ESC: no quit */

    /* 1-6: open character sheet for that party member */
    if(sc>=0x02&&sc<=0x07&&g_game_ready){
        int idx=sc-0x02;
        if(idx<g_gs.party.count){
            g_charsheet_idx=idx; g_prev_mode=g_mode;
            g_mode=GM_CHARSHEET; g_needs_redraw=true; return true;
        }
    }

    /* Enter (0x1C) OR Y (0x15) — activate tile / confirm Y/N prompt */
    if((sc==0x1C||sc==0x15)&&g_game_ready&&g_ovr_loaded){
        /* Use the OVR script text; peek_tile_event covers both OVR and manual table */
        const char *ovr_txt=ovr_text_at(&g_ovr,g_gs.player.x,g_gs.player.y,g_gs.player.facing);
        int old_map=g_gs.map_idx;
        int new_map=handle_tile_event(&g_gs,&g_ovr,ovr_txt);
        if(new_map!=old_map){g_gs.map_idx=new_map;mm_load_ovr_for_map(new_map);}
        /* Service SFX */
        { const char *m2=g_gs.player.message;
          if(mm_strncmp(m2,"RESTED",6)==0)         sfx_play(MM_INN,MM_INN_LEN);
          else if(mm_strncmp(m2,"DEAD RAISED",11)==0||
                  mm_strncmp(m2,"CONDITIONS CURED",16)==0||
                  mm_strncmp(m2,"ERADICATED RES",14)==0||
                  mm_strncmp(m2,"RAISED FROM",11)==0) sfx_play(MM_CHORD,MM_CHORD_LEN);
        }
        if(g_mode==GM_EXPLORE){
            /* Also check if shop should open */
            if(mm_namecmp(g_gs.player.message,"BROWSE_SHOP")==0){
                player_set_message(&g_gs.player,"",0);
                g_shop_town=(g_gs.map_idx>=0&&g_gs.map_idx<=4)?g_gs.map_idx:0;
                g_shop_scroll=0; g_prev_mode=GM_EXPLORE; g_mode=GM_SHOP;
            }
        }
        g_needs_redraw=true;return true;
    }
    if(sc==0x30&&g_game_ready){ /* B: bash */
        const struct Map *m=&g_maps[g_gs.map_idx];
        if(m->cells[g_gs.player.y][g_gs.player.x].wall[g_gs.player.facing]==WT_DOOR)
            player_set_message(&g_gs.player,"BASH! You smash through!",120);
        else{
            extern int monster_roll(int);
            player_set_message(&g_gs.player,"You bash the wall -- OW!",120);
            if(g_gs.party.count>0){
                int dmg=1+monster_roll(4);
                g_gs.party.members[0].hp-=dmg;
                if(g_gs.party.members[0].hp<0) g_gs.party.members[0].hp=0;
            }
        }
        g_needs_redraw=true;return true;
    }
    if(sc==0x2D&&g_game_ready&&g_ovr_loaded){ /* X: search */
        const char *txt=ovr_text_at(&g_ovr,g_gs.player.x,g_gs.player.y,g_gs.player.facing);
        player_set_message(&g_gs.player,(txt&&*txt)?txt:"Nothing found.",120);
        g_needs_redraw=true;return true;
    }
    if(sc==0x19&&g_game_ready){ /* P: save → slot picker */
        mm_enter_save_slot(0); return true;
    }
    if(sc==0x2C&&g_game_ready){ /* Z: rest anywhere */
        if(g_gs.party.shared_food>0){
            extern int monster_roll(int);
            g_gs.party.shared_food--;
            int i5;
            for(i5=0;i5<g_gs.party.count;i5++){
                Character *c5=&g_gs.party.members[i5];
                if(!IS_DEADLIKE(c5->condition)&&!(c5->condition&COND_UNCONSCIOUS)){
                    int heal=1+c5->level/2;
                    c5->hp+=heal; if(c5->hp>c5->hp_max) c5->hp=c5->hp_max;
                    if(c5->sp<c5->sp_max) c5->sp++;
                    c5->condition&=(uint8_t)(~(COND_ASLEEP|COND_UNCONSCIOUS));
                }
            }
            player_set_message(&g_gs.player,"RESTED BRIEFLY.",90);
            if(g_gs.map_idx>=5&&monster_roll(100)<=30) mm_start_combat();
        } else {
            player_set_message(&g_gs.player,"NO FOOD TO REST WITH.",60);
        }
        g_needs_redraw=true; return true;
    }
    if(sc==0x23&&g_game_ready){ /* H: sage help */
        g_sage_page=0; g_prev_mode=g_mode; g_mode=GM_SAGE; g_needs_redraw=true; return true;
    }
    if(sc==0x32&&g_game_ready){ /* M: map overlay */
        g_prev_mode=g_mode; g_mode=GM_MAP; g_needs_redraw=true; return true;
    }
    if(sc==0x16&&g_game_ready){ /* U: unlock door */
        extern int monster_roll(int);
        int dir=g_gs.player.facing;
        Cell *cell=&g_maps[g_gs.map_idx].cells[g_gs.player.y][g_gs.player.x];
        if(cell->wall[dir]==WT_DOOR){
            int has_rob=0,i3;
            for(i3=0;i3<g_gs.party.count;i3++) if(g_gs.party.members[i3].cls==6){has_rob=1;break;}
            if(monster_roll(100)<=(has_rob?70:20)){
                cell->pass[dir]=1;
                int nx=g_gs.player.x+FDX[dir],ny=g_gs.player.y+FDY[dir];
                if(nx>=0&&nx<16&&ny>=0&&ny<16)
                    g_maps[g_gs.map_idx].cells[ny][nx].pass[(dir+2)%4]=1;
                player_set_message(&g_gs.player,"DOOR UNLOCKED!",90);
            } else player_set_message(&g_gs.player,"FAILED TO PICK LOCK!",90);
        } else player_set_message(&g_gs.player,"NO DOOR TO UNLOCK.",60);
        g_needs_redraw=true; return true;
    }
    do_move(sc);
    return g_needs_redraw;
}

bool mm_port_ptr_down(int16_t x,int16_t y){
    if(!g_open) return false;
    WindowHit hit=wnd_hit_test(&g_frame,x,y);
    switch(hit){
        case WINDOW_HIT_TITLE:  wnd_begin_drag(&g_frame,x,y);   return true;
        case WINDOW_HIT_RESIZE: wnd_begin_resize(&g_frame,x,y); return true;
        case WINDOW_HIT_CLOSE:  mm_port_close();                return true;
        case WINDOW_HIT_BODY:
            if(g_mode==GM_TITLE){title_exit();return true;}
            if(g_mode==GM_DEFEAT){mm_port_scancode(0x1C);return true;}
            if(g_mode==GM_CHARSHEET){g_mode=g_prev_mode;g_needs_redraw=true;return true;}
            if(g_mode==GM_TOWN_SELECT){
                int16_t cx,cy,cw,ch;
                wnd_content_rect(&g_frame,0,&cx,&cy,&cw,&ch);
                int rel_y=(int)(y-cy);
                int idx=(rel_y-90)/30;
                if(idx>=0&&idx<=5) town_select_confirm(idx);
                return true;
            }
            /* HUD button clicks during exploration */
            if(g_mode==GM_EXPLORE){
                int16_t cx2,cy2,cw2,ch2;
                wnd_content_rect(&g_frame,0,&cx2,&cy2,&cw2,&ch2);
                int rx=(int)(x-cx2), ry=(int)(y-cy2);
                /* Movement buttons: 3x2 grid in right panel */
                if(rx>=MB_X&&rx<MB_X+MB_W&&ry>=MB_Y&&ry<MB_Y+MB_H){
                    static const uint8_t MOVE_SC[6]={0x10,0xC8,0x12,0x1E,0xD0,0x20};
                    int col=(rx-MB_X)/MB_BW, row=(ry-MB_Y)/MB_BH;
                    int btn=row*3+col;
                    if(btn>=0&&btn<6){ do_move(MOVE_SC[btn]); return true; }
                }
                /* Command buttons: 3x3 grid */
                if(rx>=CB_X&&rx<CB_X+CB_W&&ry>=CB_Y&&ry<CB_Y+CB_H){
                    /* bash=3, search=4, unlock=5, map=6, quikref(F1)=7, save=8 */
                    static const uint8_t CMD_SC[9]={0,0,0, 0x30,0x2D,0x16, 0x32,0x3B,0x19};
                    int col=(rx-CB_X)/CB_BW, row=(ry-CB_Y)/CB_BH;
                    int btn=row*3+col;
                    if(btn>=0&&btn<9&&CMD_SC[btn]) mm_port_scancode(CMD_SC[btn]);
                    return true;
                }
            }
            break;
        default:break;
    }
    return false;
}
bool mm_port_ptr_move(int16_t x,int16_t y){
    if(!g_open) return false;
    return wnd_update_pointer(&g_frame,x,y);
}
bool mm_port_ptr_up(int16_t x,int16_t y){
    (void)x;(void)y;
    if(!g_open) return false;
    wnd_end_interaction(&g_frame);
    return true;
}