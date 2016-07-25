#include <stdio.h>
#include <conio.h>
#include <math.h>

#include <allegro.h>

BITMAP* buffer;

#define PIXEL(bmp, x, y) ((long*)(bmp)->line[(y)])[(x)]

void init()
{
    allegro_init();
    
    install_mouse();
    install_keyboard();
    set_color_depth(32);
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, 1024, 768, 0, 0);
    
    buffer = create_bitmap(SCREEN_W, SCREEN_H);
    
    srand(time(NULL));
}

void deinit()
{
    destroy_bitmap(buffer);
}

typedef struct {
    float x, y;
}VEC2F;

VEC2F vec2f(float x, float y)
{
    VEC2F v;
    v.x = x;
    v.y = y;
    return v;
}

float vec2f_dot(VEC2F a, VEC2F b)
{
    return a.x * b.x + a.y * b.y;
}

VEC2F vec2f_diff(VEC2F a, VEC2F b)
{
    return vec2f(a.x - b.x, a.y - b.y);
}

VEC2F vec2f_sum(VEC2F a, VEC2F b)
{
    return vec2f(a.x + b.x, a.y + b.y);
}

typedef struct {
    float k, t;
}RAY_VS_SEGMENT_RESULT;

RAY_VS_SEGMENT_RESULT ray_vs_segment(VEC2F a, VEC2F b, VEC2F o, VEC2F dir)
{
    VEC2F n = vec2f(-dir.y, dir.x);
    VEC2F a1 = vec2f_diff(a, o);
    VEC2F b1 = vec2f_diff(b, o);
    
    float p1 = vec2f_dot(a1, n);
    float p2 = vec2f_dot(b1, n);
    
    float u1 = vec2f_dot(a1, dir);
    float u2 = vec2f_dot(b1, dir);
    
    RAY_VS_SEGMENT_RESULT r;
    r.k = -p1 / (p2 - p1);
    r.t = u1 + (u2 - u1) * r.k;
    return r;
}

void vec2f_line(BITMAP* buffer, VEC2F a, VEC2F b, int col)
{
    line(buffer, (int)a.x, (int)a.y, (int)b.x, (int)b.y, col);
}

void vec2f_circlefill(BITMAP* buffer, VEC2F c, float r, int col)
{
    circlefill(buffer, (int)c.x, (int)c.y, (int)r, col);
}

int main()
{
    int exit = 0;
    
    init();
    
    VEC2F a = vec2f(100, 300);
    VEC2F b = vec2f(400, 100);
    VEC2F o = vec2f(SCREEN_W / 2, SCREEN_H / 2);
    VEC2F d = vec2f(0, 0);
    
    while(!exit)
    {
        if(key[KEY_ESC]) exit = 1;
        
        if(mouse_b == 2)
        {
            o.x = mouse_x;
            o.y = mouse_y;
        }
        
        d.x = mouse_x - o.x;
        d.y = mouse_y - o.y;
        
        clear_to_color(buffer, makecol(128, 128, 128));
        
        vec2f_circlefill(buffer, o, 3, makecol(255, 0, 0));
        vec2f_line(buffer, a, b, 0);
        vec2f_line(buffer, o, vec2f_sum(o, d), makecol(255, 0, 0));
        
        
        float len = sqrt(d.x*d.x + d.y*d.y);
        d.x /= len;
        d.y /= len;
        
        RAY_VS_SEGMENT_RESULT r = ray_vs_segment(a, b, o, d);
        circlefill(buffer, (int)(a.x + (b.x - a.x) * r.k), (int)(a.y + (b.y - a.y) * r.k), 3, makecol(0, 255, 0));
        circlefill(buffer, (int)(o.x + d.x * r.t), (int)(o.y + d.y * r.t), 2, makecol(0, 0, 255));
        
        draw_sprite(buffer, mouse_sprite, mouse_x, mouse_y);
        blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    }
    
    deinit();
    return 0;
}END_OF_MAIN()
