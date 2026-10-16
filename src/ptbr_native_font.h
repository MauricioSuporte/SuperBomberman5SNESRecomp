/* Native menu lettering is read from the player's ROM at draw time.
 * Location metadata contains no extracted pixels. Accent marks and the three
 * missing banner letters (J/Q/V) are authored; all other shapes stay native. */
#include "ptbr_font_locations.h"

static unsigned native_pixel(uint32_t tile,unsigned x,unsigned y) {
    const uint8_t *p=g_rom+tile+(y&7)*2;
    unsigned color=0;
    for(unsigned plane=0;plane<4;++plane)
        color|=((p[(plane/2)*16+plane%2]>>(7-x))&1u)<<plane;
    return color;
}
static unsigned native_base(unsigned c,unsigned *accent) {
    *accent=0;
    switch(c) {
    case 0xc7:case 0xe7:*accent=1;return c==0xc7?'C':'c';
    case 0xc3:case 0xe3:*accent=2;return c==0xc3?'A':'a';
    case 0xd5:case 0xf5:*accent=2;return c==0xd5?'O':'o';
    case 0xc1:case 0xe1:*accent=3;return c==0xc1?'A':'a';
    case 0xc9:case 0xe9:*accent=3;return c==0xc9?'E':'e';
    case 0xcd:case 0xed:*accent=3;return c==0xcd?'I':'i';
    case 0xd3:case 0xf3:*accent=3;return c==0xd3?'O':'o';
    case 0xda:case 0xfa:*accent=3;return c==0xda?'U':'u';
    case 0xc2:case 0xe2:*accent=4;return c==0xc2?'A':'a';
    case 0xca:case 0xea:*accent=4;return c==0xca?'E':'e';
    case 0xd4:case 0xf4:*accent=4;return c==0xd4?'O':'o';
    }
    return c;
}
static void native_accent(uint8_t p[16][24],unsigned width,unsigned accent,unsigned ink,unsigned outline) {
    unsigned x=width/2;
    if(accent==1) {p[14][x]=ink;p[15][x?x-1:0]=ink;return;}
    /* Reserve the native upper margin, keeping the baseline and glyph weight. */
    if(accent) {
        p[0][x]=ink;
        if(accent==2) {p[0][x+2]=ink;p[1][x-1]=ink;p[1][x+1]=ink;}
        if(accent==3) p[1][x-1]=ink;
        if(accent==4) {p[1][x-1]=ink;p[1][x+1]=ink;}
        if(x>1 && !p[0][x-1]) p[0][x-1]=outline;
    }
}
static unsigned native_small(unsigned character,uint8_t p[16][24]) {
    unsigned accent,c=native_base(character,&accent);memset(p,0,16*24);
    if(c==' ') return 5;
    for(unsigned n=0;n<sizeof(kNativeSmall)/sizeof(kNativeSmall[0]);++n) if(kNativeSmall[n].character==c) {
        for(unsigned y=0;y<16;++y) for(unsigned x=0;x<8;++x)
            p[y][x]=(uint8_t)native_pixel(kNativeSmall[n].tiles[y/8],x,y);
        native_accent(p,7,accent,4,1);
        return c=='I'||c=='i'||c=='l'?4:c=='f'||c=='r'||c=='t'?6:c=='M'||c=='W'||c=='m'||c=='w'?8:c=='!'?3:7;
    }
    /* Digits, percent and cursor hints keep authored shapes in the same
     * native-sized cell; the cartridge's complete alphabet supplies letters. */
    char text[2]={(char)c,0};PtbrLabel label={.width=1,.height=16,.ink=4,.text=text};
    uint8_t fallback[32][224];render_authored(fallback,&label);
    for(unsigned y=0;y<16;++y) memcpy(p[y],fallback[y],8);
    return 7;
}
static unsigned native_white(unsigned character,uint8_t p[16][24]) {
    unsigned accent,c=native_base(character,&accent),source=c;
    memset(p,0,16*24);if(c==' ') return 8;
    if(c=='C'||c=='D'||c=='Q') source='O';
    if(c=='J') source='U';
    if(c=='F') source='E';
    bool found=false;
    for(unsigned n=0;n<sizeof(kNativeWhite)/sizeof(kNativeWhite[0]);++n) if(kNativeWhite[n].character==source) {
        for(unsigned y=0;y<16;++y) for(unsigned x=0;x<8;++x) {
            const uint8_t *raw=g_rom+kNativeWhite[n].tiles[y/8]+y%8*2;
            p[y][x]=(uint8_t)(((raw[0]>>(7-x))&1)|(((raw[1]>>(7-x))&1)<<1));
        }
        found=true;break;
    }
    if(!found && c>='3' && c<='5') {
        /* Authored extensions for the extra co-op rows. Match the native
         * 1/2: six ink columns, ten rows, two-pixel stems, full outline.
         * The original copyright digits share this title font page. */
        static const uint8_t digits[3][10]={
            {30,51,3,3,14,3,3,3,51,30},
            {6,14,14,30,54,54,63,6,6,6},
            {63,48,48,48,62,3,3,3,51,30}
        };
        for(unsigned y=0;y<10;++y) for(unsigned x=0;x<6;++x)
            if(digits[c-'3'][y] & (1u<<(5-x)))
                for(unsigned yy=y+3;yy<=y+5;++yy)
                    for(unsigned xx=x;xx<=x+2;++xx) p[yy][xx]=1;
        for(unsigned y=0;y<10;++y) for(unsigned x=0;x<6;++x)
            if(digits[c-'3'][y] & (1u<<(5-x))) p[y+4][x+1]=2;
        found=true;
    }
    if(!found) {
        uint8_t small[16][24];native_small(c,small);
        for(unsigned y=4;y<14;++y) for(unsigned x=0;x<8;++x)
            p[y][x]=small[(y-4)*16/10][x]==4?2:small[(y-4)*16/10][x]?1:0;
    }
    /* The native T cell includes the left outline of its following U.
     * Keep the crossbar, but crop that neighboring stroke below it. */
    if(c=='T') for(unsigned y=6;y<15;++y) p[y][7]=0;
    if(c=='C') for(unsigned y=6;y<11;++y) for(unsigned x=4;x<8;++x) p[y][x]=0;
    if(c=='J') for(unsigned y=3;y<11;++y) for(unsigned x=0;x<4;++x) p[y][x]=0;
    if(c=='D') for(unsigned y=4;y<14;++y) {p[y][0]=2;p[y][1]=2;}
    if(c=='F') for(unsigned y=12;y<16;++y) for(unsigned x=3;x<8;++x) p[y][x]=0;
    if(c=='Q') {p[12][5]=2;p[13][6]=2;p[14][7]=2;}
    native_accent(p,7,accent,2,1);return 8;
}
static unsigned native_blue_uncached(unsigned character,uint8_t p[16][24]) {
    unsigned accent,c=native_base(character,&accent);memset(p,0,16*24);
    if(c==' ') return 7;
    if(c>='a' && c<='z') c-=32;
    unsigned width=12;bool found=false;uint8_t fill[16][24]={{0}};
    for(unsigned n=0;n<sizeof(kNativeBlue)/sizeof(kNativeBlue[0]);++n) if(kNativeBlue[n].character==c) {
        const PtbrNativeBlue *g=&kNativeBlue[n];width=g->width;found=true;
        unsigned crop_width=width+4;
        uint8_t raw[16][24]={{0}},background[16][24]={{0}};
        for(unsigned y=0;y<16;++y) for(unsigned x=0;x<crop_width;++x) {
            unsigned sx=x+g->offset;raw[y][x]=(uint8_t)native_pixel(g->tiles[(y/8)*4+sx/8],sx%8,y);
            unsigned v=raw[y][x];background[y][x]=(v==8||v==9||v==10||((!x||x+1==crop_width||!y||y==15)&&v==11));
        }
        /* Keep two columns of native outline outside the ink bounds.
         * Otherwise E/F/H crossbars touch the crop edge and a white bevel
         * is mistaken for background. Counters still connect to gray. */
        for(unsigned pass=0;pass<crop_width+16;++pass) for(unsigned y=0;y<16;++y) for(unsigned x=0;x<crop_width;++x)
            if(raw[y][x]==11 && !background[y][x] &&
               ((x&&background[y][x-1])||(x+1<crop_width&&background[y][x+1])||
                (y&&background[y-1][x])||(y<15&&background[y+1][x]))) background[y][x]=1;
        for(unsigned y=0;y<16;++y) for(unsigned x=0;x<width;++x) {
            unsigned v=raw[y][x+2];
            if(v==3||v==4||v==7||(v==11&&!background[y][x+2])) fill[y][x+1]=(uint8_t)v;
        }
        break;
    }
    if(!found) {
        /* Authored extensions to the native heading face, not a stretched
         * 5x7 alphabet. Native row colors provide the same bevel. */
        static const unsigned colors[14]={3,3,7,7,4,4,11,11,4,4,7,7,3,3};
        for(unsigned y=0;y<14;++y) for(unsigned x=0;x<12;++x) {
            bool on=false;
            if(c=='J') on=(y<3&&x>=2&&x<=10)||(x>=8&&x<=10&&y<11)||(y>=10&&y<=12&&x>=2&&x<=9)||(y>=8&&y<=10&&x>=1&&x<=3);
            else if(c=='V') {unsigned inset=y*4/13;on=(x>=inset&&x<inset+3)||(x>=9-inset&&x<12-inset);}
            else if(c=='Q') on=(y<3||y>=11)?(x>=2&&x<=9):((x<=2||x>=9)&&y>=2&&y<=11);
            if(c=='Q'&&y>=9&&x>=y-2&&x<=y) on=true;
            if(on) fill[y][x+1]=(uint8_t)colors[y];
        }
    }
    /* BG labels begin on a tile boundary that cuts off the native upper
     * outline. Move ink down one row before outlining; keep all ink rows,
     * including when these same glyphs are used on transparent OBJ. */
    for(unsigned y=15;y>0;--y) memcpy(fill[y],fill[y-1],24);
    memset(fill[0],0,24);
    for(unsigned y=0;y<16;++y) for(unsigned x=1;x<=width;++x) if(fill[y][x]) {
        if(y<15&&x+2<24) p[y+1][x+2]=1;
        for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx) {
            int xx=(int)x+dx,yy=(int)y+dy;if(xx>=0&&xx<24&&yy>=0&&yy<16) p[yy][xx]=6;
        }
    }
    for(unsigned y=0;y<16;++y) for(unsigned x=0;x<24;++x) if(fill[y][x]) p[y][x]=fill[y][x];
    native_accent(p,width,accent,7,6);
    return width+3;
}
/* The cartridge is immutable for these glyph ranges. Cache only native blue
 * shapes; avoid repeating the background flood fill on every rendered frame. */
static uint8_t native_blue_cache[256][16][24];
static uint8_t native_blue_width[256];
static const uint8_t *native_font_rom;
static unsigned native_blue(unsigned character,uint8_t p[16][24]) {
    if(native_font_rom!=g_rom) {memset(native_blue_width,0,sizeof(native_blue_width));native_font_rom=g_rom;}
    if(character>=256) {memset(p,0,16*24);return 0;}
    if(!native_blue_width[character]) native_blue_width[character]=(uint8_t)native_blue_uncached(character,native_blue_cache[character]);
    memcpy(p,native_blue_cache[character],16*24);return native_blue_width[character];
}
static void render_text(uint8_t pixels[32][224],const PtbrLabel *label) {
    if(!g_rom) {render_authored(pixels,label);return;}
    memset(pixels,0,32*224);unsigned width=label->width*8;
    bool blue=label->style==2||label->style==4||label->style==3;
    uint8_t line[16][1024]={{0}};unsigned advance=0;
    const unsigned char *at=(const unsigned char *)label->text;
    while(*at) {
        uint8_t glyph[16][24];unsigned c=next_character(&at),w=blue?native_blue(c,glyph):label->style==1?native_white(c,glyph):native_small(c,glyph);
        unsigned scale=label->height==32?2:1;
        if(advance+24*scale>1024) return;
        for(unsigned y=0;y<16;++y) for(unsigned x=0;x<24*scale;++x) if(glyph[y][x/scale]) line[y][advance+x]=glyph[y][x/scale];
        advance+=(label->style==1?8:label->style==5?6:w)*scale;
    }
    if(!advance) return;
    unsigned target=advance>width?width:advance;
    unsigned left=(blue||(label->style==1&&label->ink==3))?(width-target)/2:0;
    if(label->style==2) {
        static const uint8_t backdrop[16]={8,9,10,10,11,11,11,11,11,11,11,10,10,10,9,9};
        for(unsigned y=0;y<16;++y) memset(pixels[y],backdrop[y],width);
    }
    unsigned top=0,height=label->height==32?30:16;
    for(unsigned y=0;y<height;++y) for(unsigned x=0;x<target;++x) {
        unsigned color=line[y*16/height][x*advance/target];
        if(!color) continue;
        if(blue) {
            if(label->style==4 && label->height==16) {if(color==6) color=5;else if(color==7)color=6;else if(color==11)color=7;}
            else if(label->style==3 || label->height==32) color=color==1||color==6?1:label->ink;
        } else color=(label->style==1?(color==2||color==3):color==4)?label->ink:1;
        pixels[top+y][left+x]=(uint8_t)color;
    }
    if(label->style==3) for(unsigned y=30;y<label->height;++y) memset(pixels[y],12,width);
}
