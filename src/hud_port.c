/* hud_port.c — MM1 HUD ported from MM_C_Port/src/hud.c.
 * Renders into a 640x440 pixel buffer (scale=1 blit).
 * 3D view region (0-479, 17-281) is left black — mm_port.c blits
 * the 240x132 sub-buffer at scale=2 into that area afterward. */

#include "hud_port.h"
#include "font.h"
#include "mm_util.h"
#include "mazedata.h"
#include "events_port.h"   /* ServiceType, tile_service_at */
#include <stddef.h>
#include <stdbool.h>

/* ── Layout constants ────────────────────────────────────────────── */
/* Title bar */
#define TB_X   0
#define TB_Y   0
#define TB_W 480
#define TB_H  17

/* 3D view area (reserved; filled by sub-buf blit in mm_port.c) */
#define V_X    0
#define V_Y   17
#define V_W  480
#define V_H  264

/* Right panel split */
#define RP_X 481
#define RP_Y   0
#define RP_W 159
#define RP_H 284

/* Minimap */
#define MM_X (RP_X)
#define MM_Y   0
#define MM_W (RP_W)
#define MM_H 108

/* Command buttons (3×3 grid) */
#define CB_X (RP_X)
#define CB_Y (MM_H+1)
#define CB_W (RP_W)
#define CB_H 113
#define CB_COLS 3
#define CB_ROWS 3
#define CB_BW  (CB_W/CB_COLS)     /* ~53 px */
#define CB_BH  (CB_H/CB_ROWS)     /* ~37 px */

/* Move buttons (3×2 grid) */
#define MB_X (RP_X)
#define MB_Y (CB_Y+CB_H+1)
#define MB_W (RP_W)
#define MB_H  62
#define MB_COLS 3
#define MB_ROWS 2
#define MB_BW  (MB_W/MB_COLS)
#define MB_BH  (MB_H/MB_ROWS)

/* Status bar */
#define SB_X   0
#define SB_Y (V_Y+V_H+1)
#define SB_W 480
#define SB_H  38

/* Party strip */
#define PT_X   0
#define PT_Y (SB_Y+SB_H+1)
#define PT_W  RENDER_W
#define PT_H (RENDER_H - PT_Y)
#define PT_ROW  ((PT_H) / MAX_PARTY)

/* ── Primitive helpers ───────────────────────────────────────────── */
static void bf(uint8_t *b, int x, int y, int w, int h, uint8_t c) {
    int r, col;
    for (r=0; r<h; r++) {
        int py=y+r; if(py<0||py>=RENDER_H) continue;
        for (col=0; col<w; col++) {
            int px=x+col; if(px<0||px>=RENDER_W) continue;
            b[py*RENDER_W+px]=c;
        }
    }
}
/* Horizontal line */
static void hl(uint8_t *b,int x,int y,int w,uint8_t c){bf(b,x,y,w,1,c);}
/* Vertical line */
static void vl(uint8_t *b,int x,int y,int h,uint8_t c){bf(b,x,y,1,h,c);}
/* Rectangle outline */
static void outline_r(uint8_t *b,int x,int y,int w,int h,uint8_t c){
    hl(b,x,y,w,c); hl(b,x,y+h-1,w,c);
    vl(b,x,y,h,c); vl(b,x+w-1,y,h,c);
}

/* ── Stone / bevel (EGA adaptation of hud.c stone_rect / bevel_rect) ── */
/* EGA: 0=black, 7=light_gray, 8=dark_gray */
static void stone_rect_b(uint8_t *b, int x, int y, int w, int h) {
    bf(b,x,y,w,h, 8);  /* dark gray base */
    /* stone pattern: lighter and darker spots */
    int yy,xx;
    for (yy=y+2; yy<y+h-2; yy+=5)
        for (xx=x+2; xx<x+w-2; xx+=9) {
            uint8_t c = (((xx*17+yy*23)&1)) ? 7 : 0;
            bf(b,xx,yy,2,1,c);
        }
}
static void bevel_b(uint8_t *b, int x, int y, int w, int h) {
    stone_rect_b(b,x,y,w,h);
    outline_r(b,x,y,w,h,0);                /* black border */
    hl(b,x+1,y+1,w-2,7);                   /* top highlight */
    vl(b,x+1,y+1,h-2,7);                   /* left highlight */
    hl(b,x+1,y+h-2,w-2,0);                 /* bottom shadow */
    vl(b,x+w-2,y+1,h-2,0);                 /* right shadow */
    bf(b,x+3,y+3,w-6,h-6,8);              /* interior */
}

/* ── Tiny 3×5 pixel font (from hud.c tiny_bits) ─────────────────── */
static unsigned tiny_bits(char ch, int row) {
    static const unsigned G[26][5]={
        {2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},
        {6,5,5,5,6},{7,4,6,4,7},{7,4,6,4,4},
        {3,4,5,5,3},{5,5,7,5,5},{7,2,2,2,7},
        {1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},
        {5,7,7,5,5},{5,7,7,7,5},{2,5,5,5,2},
        {6,5,6,4,4},{2,5,5,7,3},{6,5,6,5,5},
        {3,4,2,1,6},{7,2,2,2,2},{5,5,5,5,7},
        {5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},
        {5,5,2,2,2},{7,1,2,4,7}
    };
    if (ch>='a'&&ch<='z') ch=(char)(ch-'a'+'A');
    if (ch<'A'||ch>'Z') return 0;
    return G[ch-'A'][row];
}
static void tiny_str(uint8_t *b, int x, int y, const char *s, uint8_t c) {
    while (s&&*s) {
        if (*s==' ') { x+=2; }
        else {
            int row,col;
            for (row=0;row<5;row++) {
                unsigned bits=tiny_bits(*s,row);
                for (col=0;col<3;col++)
                    if (bits&(1u<<(2-col))) bf(b,x+col,y+row,1,1,c);
            }
            x+=4;
        }
        s++;
    }
}
static int tiny_width(const char *s) {
    int n=0; while(s&&*s){n+=(*s==' ')?2:4;s++;} return n?n-1:0;
}
static void tiny_center(uint8_t *b,int x,int y,int w,const char*s,uint8_t c){
    int tw=tiny_width(s); tiny_str(b,x+(w-tw)/2,y,s,c);
}

/* ── Command button icons (adapted from hud.c draw_command_icon) ─── */
static void cmd_icon(uint8_t *b, const char *cmd, int x, int y) {
    if (!cmd) return;
    if (mm_namecmp(cmd,"cast")==0) {
        vl(b,x+9,y+6,8,0); hl(b,x+5,y+10,8,0);
        bf(b,x+6,y+7,2,2,0); bf(b,x+14,y+10,2,2,0);
    } else if (mm_namecmp(cmd,"sleep")==0) {
        font_print(b,RENDER_W,"Zz",x+8,y+5,0,1);
    } else if (mm_namecmp(cmd,"bash")==0) {
        vl(b,x+12,y+5,8,0); vl(b,x+13,y+5,8,0);
        hl(b,x+5,y+5,8,0); outline_r(b,x+5,y+8,8,8,0);
    } else if (mm_namecmp(cmd,"search")==0) {
        outline_r(b,x+8,y+4,8,8,0);
        vl(b,x+15,y+11,5,0); hl(b,x+16,y+14,5,0);
    } else if (mm_namecmp(cmd,"unlock")==0) {
        hl(b,x+6,y+12,12,0);
        outline_r(b,x+5,y+9,4,4,0);
        vl(b,x+16,y+12,5,0); hl(b,x+16,y+14,3,0);
    } else if (mm_namecmp(cmd,"map")==0) {
        outline_r(b,x+5,y+5,14,10,0);
        vl(b,x+10,y+5,10,0); vl(b,x+14,y+5,10,0);
    } else if (mm_namecmp(cmd,"save")==0) {
        outline_r(b,x+6,y+5,12,11,0);
        outline_r(b,x+9,y+6,6,4,0);
        outline_r(b,x+9,y+12,6,3,0);
    }
}

/* ── Draw one command button ─────────────────────────────────────── */
static void draw_cmd_btn(uint8_t *b, int x, int y, int w, int h,
                          const char *cmd, const char *label) {
    bevel_b(b,x,y,w,h);
    cmd_icon(b,cmd,x,y);
    tiny_center(b,x+1,y+h-8,w-2,label,14); /* yellow label */
}

/* ── Draw one move button (arrow shape) ─────────────────────────── */
static void draw_arrow_btn(uint8_t *b, int x, int y, int w, int h,
                            const char *cmd) {
    bevel_b(b,x,y,w,h);
    int cx=x+w/2, cy=y+h/2;
    int i;
    if (mm_namecmp(cmd,"forward")==0) {
        /* Up arrow */
        for (i=0;i<8;i++) hl(b,cx-i,cy-4+i,i*2+1,0);
        bf(b,cx-3,cy+4,7,4,0);
    } else if (mm_namecmp(cmd,"backward")==0) {
        /* Down arrow */
        for (i=0;i<8;i++) hl(b,cx-i,cy+4-i,i*2+1,0);
        bf(b,cx-3,cy-8,7,4,0);
    } else if (mm_namecmp(cmd,"turn_l")==0||mm_namecmp(cmd,"strafe_l")==0) {
        for (i=0;i<7;i++) vl(b,cx-4+i,cy-i,i*2+1,0);
        bf(b,cx+3,cy-3,4,7,0);
    } else if (mm_namecmp(cmd,"turn_r")==0||mm_namecmp(cmd,"strafe_r")==0) {
        for (i=0;i<7;i++) vl(b,cx+4-i,cy-i,i*2+1,0);
        bf(b,cx-7,cy-3,4,7,0);
    }
}

/* ═══════════════════════════════════════════════════════════════════
 * Public API
 * ═══════════════════════════════════════════════════════════════════ */

void hud_draw_chrome(uint8_t *b)
{
    /* Title bar */
    bf(b,TB_X,TB_Y,TB_W,TB_H,0);
    outline_r(b,TB_X,TB_Y,TB_W,TB_H,8);

    /* Separator above 3D view */
    hl(b,0,V_Y-1,RENDER_W,8);
    /* Right divider */
    vl(b,RP_X-1,0,V_Y+V_H,8);
    /* Separator below 3D view / right panel */
    hl(b,0,SB_Y-1,RENDER_W,8);
    /* Separator below status */
    hl(b,0,PT_Y-1,RENDER_W,8);

    /* Minimap box */
    bevel_b(b,MM_X,MM_Y,MM_W,MM_H);
    /* Corner gem decorations */
    bf(b,MM_X+3,MM_Y+3,5,5,4);   bf(b,MM_X+MM_W-8,MM_Y+3,5,5,4);
    bf(b,MM_X+3,MM_Y+MM_H-8,5,5,4); bf(b,MM_X+MM_W-8,MM_Y+MM_H-8,5,5,4);
    outline_r(b,MM_X+3,MM_Y+3,5,5,0); outline_r(b,MM_X+MM_W-8,MM_Y+3,5,5,0);
    outline_r(b,MM_X+3,MM_Y+MM_H-8,5,5,0); outline_r(b,MM_X+MM_W-8,MM_Y+MM_H-8,5,5,0);
    /* Side skull-dots */
    outline_r(b,MM_X+1,MM_Y+MM_H/2-5,10,10,0); bf(b,MM_X+2,MM_Y+MM_H/2-4,8,8,0);
    outline_r(b,MM_X+MM_W-11,MM_Y+MM_H/2-5,10,10,0); bf(b,MM_X+MM_W-10,MM_Y+MM_H/2-4,8,8,0);

    /* Command buttons */
    {
        static const char *cmds[9] = {"cast","active_spells","sleep","bash","search","unlock","map","quikref","save"};
        static const char *lbls[9] = {"Cast","Active","Sleep","Bash","Search","Unlock","Map","QuikRef","Save"};
        bevel_b(b,CB_X,CB_Y,CB_W,CB_H);
        int i;
        for (i=0;i<9;i++) {
            int col=i%CB_COLS, row=i/CB_COLS;
            int bx=CB_X+1+col*(CB_BW), by=CB_Y+1+row*(CB_BH);
            draw_cmd_btn(b,bx,by,CB_BW-1,CB_BH-1,cmds[i],lbls[i]);
        }
    }

    /* Move buttons */
    {
        static const char *moves[6]={"turn_l","forward","turn_r","strafe_l","backward","strafe_r"};
        bevel_b(b,MB_X,MB_Y,MB_W,MB_H);
        int i;
        for (i=0;i<6;i++) {
            int col=i%MB_COLS, row=i/MB_COLS;
            int bx=MB_X+1+col*(MB_BW), by=MB_Y+1+row*(MB_BH);
            draw_arrow_btn(b,bx,by,MB_BW-1,MB_BH-1,moves[i]);
        }
    }

    /* Status bar */
    bf(b,SB_X,SB_Y,SB_W,SB_H,0);
    outline_r(b,SB_X,SB_Y,SB_W,SB_H,8);

    /* Party strip */
    bf(b,PT_X,PT_Y,PT_W,PT_H,0);
    outline_r(b,PT_X,PT_Y,PT_W,PT_H,8);
}

void hud_draw_minimap(uint8_t *b, const struct Map *m, const Player *p)
{
    int bx=MM_X+10, by=MM_Y+10;
    int bw=MM_W-20, bh=MM_H-20;
    bf(b,bx,by,bw,bh,1); /* dark blue floor */
    if (!m) return;

    int cw=bw/MAP_SIZE, ch=bh/MAP_SIZE;
    if(cw<1)cw=1; if(ch<1)ch=1;
    int ox=bx+(bw-cw*MAP_SIZE)/2, oy=by+(bh-ch*MAP_SIZE)/2;
    int x,y;
    for (y=0;y<MAP_SIZE;y++) for (x=0;x<MAP_SIZE;x++) {
        const Cell *c=map_cell(m,x,y); if(!c) continue;
        int dy=MAP_SIZE-1-y, px=ox+x*cw, py=oy+dy*ch;
        if (c->wall[0]&&py>by)          bf(b,px,py,cw,1,c->wall[0]==2?2:7);
        if (c->wall[1]&&px+cw<bx+bw)   bf(b,px+cw-1,py,1,ch,c->wall[1]==2?2:7);
        if (c->wall[2]&&py+ch<by+bh)   bf(b,px,py+ch-1,cw,1,c->wall[2]==2?2:7);
        if (c->wall[3]&&px>bx)          bf(b,px,py,1,ch,c->wall[3]==2?2:7);
        /* Service dots */
        ServiceType svc=tile_service_at(m->map_idx,x,y);
        uint8_t sc2=0;
        switch(svc){
        case SVC_INN:        sc2=14; break;
        case SVC_TEMPLE:     sc2=11; break;
        case SVC_TRAINING:   sc2=10; break;
        case SVC_BLACKSMITH: sc2=6;  break;
        case SVC_FOOD:       sc2=2;  break;
        case SVC_TAVERN:     sc2=4;  break;
        case SVC_STAIRS:     sc2=9;  break;
        case SVC_STAIRS_UP:  sc2=3;  break;
        default: break;
        }
        if (sc2) bf(b,ox+x*cw+cw/2,oy+dy*ch+ch/2,2,2,sc2);
    }
    if (p) {
        int dy=MAP_SIZE-1-p->y;
        bf(b,ox+p->x*cw+cw/4,oy+dy*ch+ch/4,cw/2+1,ch/2+1,14);
    }
}

void hud_draw_compass(uint8_t *b, const Player *p,
                      const GameState *gs, const struct Map *m)
{
    static const char * const DIRS[4]={"N","E","S","W"};
    static const char * const ARROWS[4]={"^",">","v","<"};
    bf(b,RP_X,SB_Y,RP_W,SB_H+PT_H,0); /* clear right bottom area */

    if (!p) return;

    /* Facing arrow scale=2 in bottom-right corner */
    int ax=RP_X+RP_W/2-8, ay=SB_Y+4;
    font_print(b,RENDER_W,ARROWS[p->facing],ax,ay,14,2);
    font_print(b,RENDER_W,DIRS[p->facing],ax+8,ay+18,15,1);

    /* Position */
    {
        char pos[14]; int i=0;
        pos[i++]='X'; pos[i++]=':';
        if(p->x>=10) pos[i++]=(char)('0'+p->x/10);
        pos[i++]=(char)('0'+p->x%10);
        pos[i++]=' '; pos[i++]='Y'; pos[i++]=':';
        if(p->y>=10) pos[i++]=(char)('0'+p->y/10);
        pos[i++]=(char)('0'+p->y%10);
        pos[i]='\0';
        font_print(b,RENDER_W,pos,RP_X+2,SB_Y+30,7,1);
    }

    /* Food counter */
    if (gs) {
        int fv = gs->party.shared_food;
        char food[16]; int fi=0;
        food[fi++]='F'; food[fi++]='o'; food[fi++]='o'; food[fi++]='d'; food[fi++]=':'; food[fi++]=' ';
        if(fv>=10) food[fi++]=(char)('0'+fv/10);
        food[fi++]=(char)('0'+fv%10); food[fi]='\0';
        font_print(b,RENDER_W,food,RP_X+2,SB_Y+42,fv>0?10:4,1);
    }
    (void)m;
}

void hud_draw_status(uint8_t *b, const GameState *gs, const struct Map *m)
{
    bf(b,SB_X+1,SB_Y+1,SB_W-2,SB_H-2,0);
    if (!gs) return;

    /* Title bar: map name + position + facing */
    if (m) {
        char tbuf[64]; int ti=0;
        const char *mn=map_display_name(gs->map_idx);
        while(*mn&&ti<20) tbuf[ti++]=*mn++;
        tbuf[ti++]=' '; tbuf[ti++]='|'; tbuf[ti++]=' ';
        tbuf[ti++]='(';
        if(gs->player.x>=10) tbuf[ti++]=(char)('0'+gs->player.x/10);
        tbuf[ti++]=(char)('0'+gs->player.x%10);
        tbuf[ti++]=',';
        if(gs->player.y>=10) tbuf[ti++]=(char)('0'+gs->player.y/10);
        tbuf[ti++]=(char)('0'+gs->player.y%10);
        tbuf[ti++]=')'; tbuf[ti]='\0';
        font_print(b,RENDER_W,tbuf,TB_X+3,TB_Y+5,15,1);
    }

    /* Status message */
    if (gs->player.msg_ticks>0 && gs->player.message[0]) {
        /* Word-wrap into 2 lines, 8px font, max ~60 chars/line */
        const char *msg=gs->player.message;
        int len=(int)mm_strlen(msg);
        const int MAX=59;
        if (len<=MAX) {
            font_print(b,RENDER_W,msg,SB_X+4,SB_Y+4,14,1);
        } else {
            int cut=MAX-1;
            while(cut>0&&msg[cut]!=' ') cut--;
            if(!cut) cut=MAX-1;
            char line1[64]; mm_memcpy(line1,msg,(uint32_t)cut); line1[cut]='\0';
            font_print(b,RENDER_W,line1,SB_X+4,SB_Y+4,14,1);
            font_print(b,RENDER_W,msg+cut+(msg[cut]==' '?1:0),SB_X+4,SB_Y+14,14,1);
        }
    }
}

void hud_draw_party(uint8_t *b, const GameState *gs)
{
    bf(b,PT_X+1,PT_Y+1,PT_W-2,PT_H-2,0);
    if (!gs||gs->party.count==0) {
        font_print(b,RENDER_W,"No party loaded",PT_X+4,PT_Y+4,8,1);
        return;
    }
    int i;
    for (i=0;i<gs->party.count&&i<MAX_PARTY;i++) {
        const Character *c=&gs->party.members[i];
        int ry=PT_Y+2+i*PT_ROW;
        uint8_t col=IS_DEADLIKE(c->condition)?8:(c->hp<=0?4:2);

        /* Name */
        char line[14]; int li=0;
        const char *nm=c->name; while(*nm&&li<12) line[li++]=*nm++;
        while(li<12) line[li++]=' '; line[li]='\0';
        font_print(b,RENDER_W,line,PT_X+4,ry,col,1);

        /* Level */
        {
            char lv[6]; lv[0]='L'; lv[1]='v';
            lv[2]=(char)('0'+(c->level>=10?c->level/10:0));
            lv[3]=(char)('0'+c->level%10); lv[4]='\0';
            font_print(b,RENDER_W,(c->level<10?lv+2:lv),PT_X+100,ry,7,1);
        }

        /* HP bar (120px wide, 5px tall) */
        {
            int barx=PT_X+130, bary=ry+1, barw=120, barh=5;
            bf(b,barx,bary,barw,barh,4); /* dark red background */
            if (c->hp_max>0&&c->hp>0) {
                int filled=(c->hp*barw)/c->hp_max;
                if(filled<1)filled=1; if(filled>barw)filled=barw;
                uint8_t bc=(c->hp*3>=c->hp_max*2)?10:(c->hp*2>=c->hp_max)?14:12;
                bf(b,barx,bary,filled,barh,bc);
            }
            outline_r(b,barx,bary,barw,barh,0);
        }

        /* HP numbers */
        {
            char hp[16]; int hi=0;
            if(c->hp>=100) hp[hi++]=(char)('0'+c->hp/100);
            if(c->hp>=10)  hp[hi++]=(char)('0'+(c->hp/10)%10);
            hp[hi++]=(char)('0'+c->hp%10);
            hp[hi++]='/';
            if(c->hp_max>=100) hp[hi++]=(char)('0'+c->hp_max/100);
            if(c->hp_max>=10)  hp[hi++]=(char)('0'+(c->hp_max/10)%10);
            hp[hi++]=(char)('0'+c->hp_max%10);
            hp[hi]='\0';
            font_print(b,RENDER_W,hp,PT_X+256,ry,7,1);
        }

        /* Condition indicator — coloured dot + short code */
        {
            const char *cstr=""; uint8_t ccol=0;
            if (IS_DEADLIKE(c->condition)){cstr="DEAD";ccol=4;}
            else if (c->condition&COND_POISONED)   {cstr="PSN";ccol=2;}
            else if (c->condition&COND_PARALYZED)  {cstr="PAR";ccol=5;}
            else if (c->condition&COND_UNCONSCIOUS){cstr="KO"; ccol=6;}
            else if (c->condition&COND_DISEASED)   {cstr="DIS";ccol=6;}
            else if (c->condition&COND_BLINDED)    {cstr="BLN";ccol=8;}
            else if (c->condition&COND_ASLEEP)     {cstr="SLP";ccol=9;}
            else if (c->condition&COND_SILENCED)   {cstr="SIL";ccol=8;}
            if (ccol) { bf(b,PT_X+302,ry+1,5,5,ccol); font_print(b,RENDER_W,cstr,PT_X+310,ry,ccol,1); }
        }

        /* Gold (first member only — shared) */
        if (i==0 && c->gold>0) {
            char gold[12]; int gi=0;
            int g2=(int)c->gold;
            if(g2>=10000) gold[gi++]=(char)('0'+g2/10000);
            if(g2>=1000)  gold[gi++]=(char)('0'+(g2/1000)%10);
            if(g2>=100)   gold[gi++]=(char)('0'+(g2/100)%10);
            if(g2>=10)    gold[gi++]=(char)('0'+(g2/10)%10);
            gold[gi++]=(char)('0'+g2%10);
            gold[gi++]='g'; gold[gi]='\0';
            font_print(b,RENDER_W,gold,PT_X+370,ry,14,1);
        }

        /* Separator line between members */
        if (i<gs->party.count-1)
            hl(b,PT_X+1,ry+PT_ROW-1,PT_W-2,8);
    }
}