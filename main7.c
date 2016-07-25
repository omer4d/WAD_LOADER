#include <stdio.h>
#include <conio.h>
#include <math.h>

#include <allegro.h>

#pragma pack(push, 1)

typedef struct {
    char id[4];
    int lump_num;
    int dir_offs;
}WAD_HEADER;

typedef struct {
    int lump_offs;
    int lump_size;
    char name[8];
}WAD_DIRENT;

typedef struct {
    short x, y;
}VERTEX;
 
typedef struct {
    short floor_height, ceil_height;
    char floor_tex_name[8], ceil_tex_name[8];
    short light, type, tag;
}SECTOR;
   
typedef struct {
    short x_offs, y_offs;
    char hi_tex_name[8], low_tex_name[8], mid_tex_name[8];
    short sector_idx;
}SIDEDEF;

typedef struct {
    short v1_idx, v2_idx;
    short flags, special_flag, sector_tag;
    short pos_sidedef_idx, neg_sidedef_idx;
}LINEDEF;

typedef struct {
    short v1_idx, v2_idx;
    short angle;
    short linedef_idx;
    short dir;
    short offs_on_linedef;
}SEG;

typedef struct {
    short x1, y1, x2, y2;
    short left_aabb[4], right_aabb[4];
    unsigned short left_child_idx, right_child_idx;
}NODE;

typedef struct {
    short seg_num;
    short first_seg_idx;
}SSECTOR;

#pragma pack(pop)

typedef struct {
    int vertex_num;
    VERTEX* vertexes;
    
    int linedef_num;
    LINEDEF* linedefs;
    
    int sidedef_num;
    SIDEDEF* sidedefs;
    
    int sector_num;
    SECTOR* sectors;
    
    int node_num;
    NODE* nodes;
    
    int seg_num;
    SEG* segs;
    
    int ssector_num;
    SSECTOR* ssectors;
}MAP;

void* read_lump(FILE* file, WAD_DIRENT* dirent)
{
    void* lump = malloc(dirent->lump_size);
    fseek(file, dirent->lump_offs, SEEK_SET);
    int size = fread(lump, dirent->lump_size, 1, file);
    return lump;
}

MAP* load_map(char const* fn)
{
    MAP* map = malloc(sizeof(MAP));
    FILE* file = fopen(fn, "rb");
    WAD_HEADER header;
    WAD_DIRENT dirent;
    WAD_DIRENT vertex_dirent, linedef_dirent, sidedef_dirent, sector_dirent;
    WAD_DIRENT node_dirent, seg_dirent, ssector_dirent;
    
    fread(&header, sizeof(header), 1, file);
    fseek(file, header.dir_offs, SEEK_SET);
    
    printf("ID: %c%c%c%c\n", header.id[0], header.id[1], header.id[2], header.id[3]);
    printf("Lump num: %d\n", header.lump_num);
    
    while(!feof(file))
    {
        fread(&dirent, sizeof(dirent), 1, file);
        char name[9] = {0};
        strncpy(name, dirent.name, 8);
        printf("%s\n", name);
        
        if(!strncmp("VERTEXES", dirent.name, 8))
            vertex_dirent = dirent;
        else if(!strncmp("LINEDEFS", dirent.name, 8))
            linedef_dirent = dirent;
        else if(!strncmp("SIDEDEFS", dirent.name, 8))
            sidedef_dirent = dirent;
        else if(!strncmp("SECTORS", dirent.name, 8))
            sector_dirent = dirent;
        else if(!strncmp("NODES", dirent.name, 8))
            node_dirent = dirent;
        else if(!strncmp("SEGS", dirent.name, 8))
            seg_dirent = dirent;
        else if(!strncmp("SSECTORS", dirent.name, 8))
            ssector_dirent = dirent;
    }
    
    map->vertex_num = vertex_dirent.lump_size / sizeof(VERTEX);
    map->vertexes = read_lump(file, &vertex_dirent);
    
    map->linedef_num = linedef_dirent.lump_size / sizeof(LINEDEF);
    map->linedefs = read_lump(file, &linedef_dirent);
    
    map->sidedef_num = sidedef_dirent.lump_size / sizeof(SIDEDEF);
    map->sidedefs = read_lump(file, &sidedef_dirent);
    
    map->sector_num = sector_dirent.lump_size / sizeof(SECTOR);
    map->sectors = read_lump(file, &sector_dirent);
    
    map->node_num = node_dirent.lump_size / sizeof(NODE);
    map->nodes = read_lump(file, &node_dirent);
    
    map->seg_num = seg_dirent.lump_size / sizeof(SEG);
    map->segs = read_lump(file, &seg_dirent);
    
    map->ssector_num = ssector_dirent.lump_size / sizeof(SSECTOR);
    map->ssectors = read_lump(file, &ssector_dirent);
    
    printf("%d vertexes\n", map->vertex_num); 
    printf("%d linedefs\n", map->linedef_num);
    printf("%d sidedefs\n", map->sidedef_num); 
    printf("%d sectors\n", map->sector_num);
    printf("%d nodes\n", map->node_num);
    printf("%d segs\n", map->seg_num); 
    printf("%d ssectors\n", map->ssector_num);
    
    fclose(file);
    return map;
}

void destroy_map(MAP* map)
{
    free(map->vertexes);
    free(map->linedefs);
    free(map->sidedefs);
    free(map->sectors);
    free(map->nodes);
    free(map->segs);
    free(map->ssectors);
    free(map);
}

BITMAP* buffer;

#define PIXEL(bmp, x, y) ((long*)(bmp)->line[(y)])[(x)]

void init()
{
    allegro_init();
    
    install_mouse();
    install_keyboard();
    set_color_depth(32);
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0);
    
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

VEC2F vec2f_uscale(VEC2F v, float k)
{
    return vec2f(v.x * k, v.y * k);
}

VEC2F normalize(VEC2F v)
{
    float len = sqrt(v.x * v.x + v.y * v.y);
    return vec2f(v.x / len, v.y / len);
}

VEC2F normalized_normal(VEC2F v)
{
    return normalize(vec2f(-v.y, v.x));
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

#define SHORT_SIGN_FLAG (1 << 15)

#define FOCAL_DIST 320
#define H_RES 320

VEC2F get_vertex(MAP* map, int idx)
{
    VERTEX* v = &map->vertexes[idx];
    return vec2f(v->x, v->y);
}

int clamp255(int x)
{
    return MAX(MIN(x, 255), 0);
}

void render_col_helper(int x, float y0, float y1, float z, int* minh, int* maxh, int col)
{
    float h1 = SCREEN_H / 2 - y0 * FOCAL_DIST / z;
    float h2 = SCREEN_H / 2 - y1 * FOCAL_DIST / z; 
    rectfill(buffer, x * 2, (int)h1, x * 2 + 2, (int)h2, col);
}

#define RANDOM_COLOR_NUM 256

int random_colors[RANDOM_COLOR_NUM];

void render_col(BITMAP* buffer, MAP* map, short idx, VEC2F vp, float cam_h, VEC2F ray_dir, int col, int* minh, int* maxh)
{   
    if(idx & SHORT_SIGN_FLAG)
    {
        //printf("ASDF = %d %d\n", (short)(idx & ~SHORT_SIGN_FLAG), idx);
        SSECTOR* ssector = &map->ssectors[(short)(idx & ~SHORT_SIGN_FLAG)];
        int i;
        for(i = 0; i < ssector->seg_num; ++i)
        {
            SEG* seg = &map->segs[ssector->first_seg_idx + i];
            VEC2F v1 = get_vertex(map, seg->v1_idx);
            VEC2F v2 = get_vertex(map, seg->v2_idx);
            RAY_VS_SEGMENT_RESULT res = ray_vs_segment(v1, v2, vp, ray_dir);
            VEC2F n = vec2f(v1.y - v2.y, v2.x - v1.x);
            LINEDEF* linedef = &map->linedefs[seg->linedef_idx];
            
            SIDEDEF *sidedef1, *sidedef2;
            SECTOR *sector1, *sector2;
            
            if(seg->dir)
            {
                sidedef1 = &map->sidedefs[linedef->neg_sidedef_idx];
                sector1 = &map->sectors[sidedef1->sector_idx];
                sidedef2 = linedef->pos_sidedef_idx == -1 ? NULL : &map->sidedefs[linedef->pos_sidedef_idx];
                sector2 = linedef->pos_sidedef_idx == -1 ? NULL : &map->sectors[sidedef2->sector_idx];
            }
            else
            {
                sidedef1 = &map->sidedefs[linedef->pos_sidedef_idx];
                sector1 = &map->sectors[sidedef1->sector_idx];
                sidedef2 = linedef->neg_sidedef_idx == -1 ? NULL : &map->sidedefs[linedef->neg_sidedef_idx];
                sector2 = linedef->neg_sidedef_idx == -1 ? NULL : &map->sectors[sidedef2->sector_idx];
            }
            
            //printf("%c%c%c%c%c%c%c%c\n", sidedef->mid_tex_name[0],  sidedef->mid_tex_name[1],
             //sidedef->mid_tex_name[2], sidedef->mid_tex_name[3], sidedef->mid_tex_name[4], sidedef->mid_tex_name[5],
              //sidedef->mid_tex_name[6], sidedef->mid_tex_name[7]);
            
            if(res.t > 0 && res.k >= 0 && res.k <= 1.0)
            {
               // VEC2F d = vec2f_diff(v2, v1);
                //VEC2F wall_normal = vec2f(-d.y, d.x);
                //float wall_len = sqrt(d.x * d.x + d.y * d.y);
                //wall_normal.x /= wall_len;
                //wall_normal.y /= wall_len;
                //int r = clamp255((int)(fabs(wall_normal.x) * 4000 / sqrt(res.t)));
                //int g = clamp255((int)(fabs(wall_normal.y) * 4000 / sqrt(res.t)));
                int color = random_colors[(ssector->first_seg_idx + i) % RANDOM_COLOR_NUM];//makecol(r, g, 0);
                
                if(sidedef1 && !sidedef2) {
                    //render_col_helper(col, sector1->ceil_height + cam_h, sector1->floor_height + cam_h, res.t, color);
                    float top_y = sector1->ceil_height - cam_h;
                    float bottom_y = sector1->floor_height - cam_h;
                    float z = res.t;
                    
                    float top_h = MAX(SCREEN_H / 2 + top_y / z  * FOCAL_DIST, *minh);
                    float bottom_h = MIN(SCREEN_H / 2 + bottom_y / z * FOCAL_DIST, *maxh);
                                printf("%f %f\n", top_h, bottom_h);
                    
                    //if(bottom_h >= top_h)
                    { 
                        if(vec2f_dot(n, vec2f_diff(vp, v1)) < 0)
                            rectfill(buffer, col * 2, (int)top_h/2*2, col * 2 + 2, (int)bottom_h/2*2, color);
                   }
                   
                                           *minh = MAX(*minh, bottom_h);
                        *maxh = MIN(*maxh, top_h);
                }
                    
                else
                {   
                    float y0 = sector1->ceil_height - cam_h;
                    float y1 = sector2->ceil_height - cam_h;
                    float z = res.t;
                    
                    float h1 = MAX(SCREEN_H / 2 + y0 * FOCAL_DIST / z, *minh);
                    float h2 = MIN(SCREEN_H / 2 + y1 * FOCAL_DIST / z, *maxh);
                    
                    if(h2 >= h1)
                    {
                        if(vec2f_dot(n, vec2f_diff(vp, v1)) < 0)
                            rectfill(buffer, col * 2, (int)h1/2*2, col * 2 + 2, (int)h2/2*2, color);
                    }
                    *minh = MAX(*minh, h1);
                    
                    
                    y0 = sector2->floor_height - cam_h;
                    y1 = sector1->floor_height - cam_h;
                    
                    h1 = MAX(SCREEN_H / 2 + y0 * FOCAL_DIST / z, *minh);
                    h2 = MIN(SCREEN_H / 2 + y1 * FOCAL_DIST / z, *maxh);
                    
                    if(h2 >= h1)
                    {
                        if(vec2f_dot(n, vec2f_diff(vp, v1)) < 0)
                            rectfill(buffer, col * 2, (int)h1/2*2, col * 2 + 2, (int)h2/2*2, color);
                    }
                    
                                            *maxh = MIN(*maxh, h2);
                }
            }
        }
        
        // render subsector...
    }
    else
    {
        NODE* node = &map->nodes[idx];
        
        VEC2F n = vec2f(-node->y2, node->x2);
        if(vec2f_dot(n, vec2f(vp.x - node->x1, vp.y - node->y1)) < 0)
        {
            render_col(buffer, map, node->left_child_idx, vp, cam_h, ray_dir, col, minh, maxh);
            render_col(buffer, map, node->right_child_idx, vp, cam_h, ray_dir, col, minh, maxh);
        }
        else
        {
            render_col(buffer, map, node->right_child_idx, vp, cam_h, ray_dir, col, minh, maxh);
            render_col(buffer, map, node->left_child_idx, vp, cam_h, ray_dir, col, minh, maxh);
        }
    }
}

SECTOR* find_sector(MAP* map, int idx, VEC2F vp)
{
    if(idx & SHORT_SIGN_FLAG)
    {
        SSECTOR* ssector = &map->ssectors[(short)(idx & ~SHORT_SIGN_FLAG)];
        SEG* seg = &map->segs[ssector->first_seg_idx];
        LINEDEF* linedef = &map->linedefs[seg->linedef_idx];
        SIDEDEF* sidedef = seg->dir ? &map->sidedefs[linedef->neg_sidedef_idx] : &map->sidedefs[linedef->pos_sidedef_idx];
        return &map->sectors[sidedef->sector_idx];
    }
    else
    {
        NODE* node = &map->nodes[idx];
        
        VEC2F n = vec2f(-node->y2, node->x2);
        if(vec2f_dot(n, vec2f(vp.x - node->x1, vp.y - node->y1)) >= 0)
            return find_sector(map, node->right_child_idx, vp);
        else
            return find_sector(map, node->left_child_idx, vp);
    }
    
    return NULL;
}

int main()
{
    int exit = 0;
    MAP* map = load_map("zaza.wad");
    VEC2F vp = vec2f(-6.f, -232.f);
    float ang = 1.6, move_speed = 200.f, rot_speed = 2.5f;
    
    init();

    int i;    
    for(i = 0; i < RANDOM_COLOR_NUM; ++i)
      random_colors[i] = makecol(rand() % 156 + 100, rand() % 156 + 100, rand() % 156 + 100);
    
    clock_t last_clock = clock();
    
    int baz = 0;
    
    while(!exit)
    {
        clock_t now = clock();
        float dt = (float)(now - last_clock) / CLOCKS_PER_SEC;
        last_clock = now;
        
        VEC2F view_dir = vec2f(cos(ang), sin(ang));
        VEC2F view_dir_norm = vec2f(-view_dir.y, view_dir.x);
        
        if(key[KEY_ESC]) exit = 1;
        if(key[KEY_S]) vp = vec2f_sum(vp, vec2f_uscale(view_dir, -move_speed * dt));
        if(key[KEY_W]) vp = vec2f_sum(vp, vec2f_uscale(view_dir, move_speed * dt));
        if(key[KEY_A]) vp = vec2f_sum(vp, vec2f_uscale(view_dir_norm, -move_speed * dt));
        if(key[KEY_D]) vp = vec2f_sum(vp, vec2f_uscale(view_dir_norm, move_speed * dt));
        if(key[KEY_LEFT]) ang -= rot_speed * dt;
        if(key[KEY_RIGHT]) ang += rot_speed * dt;
        
        //printf("(%f, %f) %f | %d\n", vp.x, vp.y, ang, mouse_x / 2);
        
        
        clear_to_color(buffer, makecol(64, 64, 64));
        
        SECTOR* s = find_sector(map, map->node_num - 1, vp);
        float h = s->floor_height - 45;
        
        int i = 0;
        
        for(i = 0; i < H_RES; ++i)
        {
            float t = i - H_RES / 2;
            float u = FOCAL_DIST;
            
            VEC2F ray_dir = vec2f_sum(vec2f_uscale(view_dir, u), vec2f_uscale(view_dir_norm, t)); //vec2f(view_dir.x * FOCAL_DIST + view_dir_norm.x * t, view_dir.y * FOCAL_DIST + view_dir_norm.y * t);
            float ray_len = sqrt(ray_dir.x * ray_dir.x + ray_dir.y * ray_dir.y);
            ray_dir.x /= ray_len;
            ray_dir.y /= ray_len;
            
            int minh = 0;
            int maxh = SCREEN_H;
            
            render_col(buffer, map, map->node_num - 1, vp, h, ray_dir, i, &minh, &maxh);
            printf("\n");
            //line(buffer, SCREEN_W / 2, SCREEN_H / 2, SCREEN_W / 2 + ray_dir.x * 200, SCREEN_H / 2 + ray_dir.y * 200, makecol(255, 255, 255));
        }

        
        
        if(key[KEY_TAB])
        for(i = 0; i < map->seg_num; ++i)
        {
            VERTEX v1t = map->vertexes[map->segs[i].v1_idx];
            VERTEX v2t = map->vertexes[map->segs[i].v2_idx];
            VEC2F v1 = vec2f(v1t.x, v1t.y);
            VEC2F v2 = vec2f(v2t.x, v2t.y);
            VEC2F mid = vec2f((v1.x + v2.x) / 2, (v1.y + v2.y) / 2);
            
            VEC2F n = normalized_normal(vec2f_diff(v2, v1));
            float scl = 1.f / 2.f;
            
            line(buffer, v1.x*scl + SCREEN_W / 2,
                         v1.y*scl + SCREEN_H / 2,
                         v2.x*scl + SCREEN_W / 2,
                         v2.y*scl + SCREEN_H / 2, makecol(255, 255, 255));
            circlefill(buffer, (int)vp.x*scl + SCREEN_W/2, (int)vp.y*scl +SCREEN_H/2, 3, makecol(255, 255, 255));
            
            
            line(buffer, SCREEN_W/2+ mid.x*scl,
                        SCREEN_H/2 +  mid.y*scl,
                        SCREEN_W/2+ (mid.x + n.x * 10)*scl,
                        SCREEN_H/2+ (mid.y + n.y * 10)*scl, makecol(255, 255, 255));
            
            
            line(buffer, SCREEN_W/2+vp.x*scl,
                        SCREEN_H/2+vp.y*scl,
                        SCREEN_W/2+(vp.x+view_dir.x*100)*scl,
                        SCREEN_H/2+(vp.y+view_dir.y*100)*scl, makecol(255, 255, 255));
        }
        
        //
        
        draw_sprite(buffer, mouse_sprite, mouse_x, mouse_y);
        blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    }
    
    destroy_map(map);
    deinit();
    return 0;
}END_OF_MAIN()
