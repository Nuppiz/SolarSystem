#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dos.h>
#include <math.h>

#define VIDEO_INT           0x10      /* the BIOS video interrupt. */
#define SET_MODE            0x00      /* BIOS func to set the video mode. */
#define VGA_256_COLOR_MODE  0x13      /* use to set 256-color mode. */
#define TEXT_MODE           0x03      /* use to set 80x25 text mode. */

#define SCREEN_WIDTH        320       /* width in pixels of mode 0x13 */
#define SCREEN_HEIGHT       200       /* height in pixels of mode 0x13 */
#define SCREEN_SIZE         64000
#define NUM_COLORS          256       /* number of colors in mode 0x13 */

#define SET_PIXEL(x,y,color)      screen_buf[(y)*SCREEN_WIDTH + (x)] = color

//#define M_PI                3.14159265358979323846264338327950288
#define degToRad(degree)    (degree * (M_PI / 180.0))

#define SP_MER              degToRad(41.49)
#define SP_VEN              degToRad(16.26)
#define SP_EAR              degToRad(10.0)
#define SP_MOO              degToRad(133.6)
#define SP_PRO              degToRad(60.0)
#define SP_MAR              degToRad(5.32)

typedef struct
{
    int x, y;
} Vec2;

typedef struct
{
    Vec2 pos; // offset from orbit center
    float angle; // which way the body itself is facing
    float angular_vel; // rotation speed
    uint8_t radius;
    uint8_t color;
    
    Vec2* orbit_center; // pointer to the "parent body's" position; set to NULL for sun
    float orbital_angle; // angle on the orbit
    float orbital_ang_vel; // rotation speed around whatever it orbits
    uint16_t orbit_radius; // distance to the "parent body"
    uint8_t orbit_color;
} CelestialBody;

enum CelestialBodies
{
    Sun,
    Mercury,
    Venus,
    Earth,
    Moon,
    MoonProbe,
    Mars,
    MarsProbe,
    Num_CelBodies
};

CelestialBody celbodies[Num_CelBodies] = { /*
  pos     angle    angvel  radius  color   orb. center           orb.ang orb.angvel   orb.radius, orb.color*/
{{0, 0},  0.0,     0.0,    10,     44,     NULL,                 0.0,    0.0,         0,          0},  // Sun
{{15, 0}, 0.0,     10.0,   5,      66,     &celbodies[Sun].pos,  0.0,    SP_MER,      20,         1}, // Mercury
{{30, 0}, 0.0,     10.0,   6,      68,     &celbodies[Sun].pos,  0.0,    SP_VEN,      40,         1}, // Venus
{{45, 0}, 0.0,     10.0,   6,      54,     &celbodies[Sun].pos,  0.0,    SP_EAR,      65,         1}, // Earth
{{5, 0},  0.0,     10.0,   4,      7,      &celbodies[Earth].pos,0.0,    SP_MOO,      11,         8}, // Moon
{{3, 0},  0.0,     10.0,   2,      92,     &celbodies[Moon].pos, 0.0,    SP_PRO,      4,          19}, // Moon probe
{{60, 0}, 0.0,     10.0,   6,      41,     &celbodies[Sun].pos,  0.0,    SP_MAR,      92,         1}, // Mars
{{3, 0},  0.0,     10.0,   2,      92,     &celbodies[Mars].pos, 0.0,    SP_PRO,      7,          19}  // Mars probe
};

uint8_t far *VGA=(uint8_t *)0xA0000000L;        /* this points to video memory. */
uint8_t far screen_buf [64000]; // double buffer

float angle_scale = 0.01;

void set_mode(uint8_t mode)
{
    union REGS regs;

    regs.h.ah = SET_MODE;
    regs.h.al = mode;
    int86(VIDEO_INT, &regs, &regs);
}

void draw_orbit(Vec2* orbit_center, uint16_t radius, uint8_t color)
{
    int rad_x;
    int rad_y;

    int center_x = orbit_center->x;
    int center_y = orbit_center->y;

    rad_y = 0;
    do
    {
        rad_x = sqrt((float)radius*radius - (float)rad_y*rad_y);
        SET_PIXEL(center_x + rad_x, center_y + rad_y, color); // lower right octant
        SET_PIXEL(center_x - rad_x, center_y + rad_y, color); // lower left octant
        SET_PIXEL(center_x + rad_x, center_y - rad_y, color); // upper right octant
        SET_PIXEL(center_x - rad_x, center_y - rad_y, color); // upper left octant
        SET_PIXEL(center_x + rad_y, center_y + rad_x, color); // bottom right octant
        SET_PIXEL(center_x - rad_y, center_y + rad_x, color); // bottom left octant
        SET_PIXEL(center_x + rad_y, center_y - rad_x, color); // top right octant
        SET_PIXEL(center_x - rad_y, center_y - rad_x, color); // top left octant

        rad_y++;
    }
    while (rad_y < rad_x);
}

void draw_celbody(Vec2 pos, uint16_t radius, uint8_t color)
{
    int rad_x;
    int rad_y;

    int fill_x = 0;
    int fill_y = 0;

    int x = pos.x;
    int y = pos.y;

    rad_y = 0;
    
    while (rad_y < rad_x)
    {
        rad_x = sqrt((float)radius*radius - (float)rad_y*rad_y);
        while (fill_y < rad_y)
        {
            while (fill_x < rad_x)
            {
                SET_PIXEL(x + fill_x, y + fill_y, color); // lower right octant
                SET_PIXEL(x - fill_x, y + fill_y, color); // lower left octant
                SET_PIXEL(x + fill_x, y - fill_y, color); // upper right octant
                SET_PIXEL(x - fill_x, y - fill_y, color); // upper left octant
                SET_PIXEL(x + fill_y, y + fill_x, color); // bottom right octant
                SET_PIXEL(x - fill_y, y + fill_x, color); // bottom left octant
                SET_PIXEL(x + fill_y, y - fill_x, color); // top right octant
                SET_PIXEL(x - fill_y, y - fill_x, color); // top left octant
                fill_x++;
            }
            fill_x = 0;
            fill_y++;
        }
        rad_y++;
    }
}

void draw_celbodies()
{
    int i = 0;
    
    while (i < Num_CelBodies)
    {
        draw_celbody(celbodies[i].pos, celbodies[i].radius, celbodies[i].color);
        i++;
    }
}

void draw_orbits()
{
    int i = 0;
    
    while (i < Num_CelBodies)
    {
        if (celbodies[i].orbit_center == NULL) // don't draw an orbit for the Sun
            i++;
        else
        {
            draw_orbit(celbodies[i].orbit_center, celbodies[i].orbit_radius, celbodies[i].orbit_color);
            i++;
        }
    }
}
        
void updateCelBody(CelestialBody* CelBody)
{
    float radians = CelBody->orbital_angle + CelBody->orbital_ang_vel * angle_scale;

    if (radians < 0 || radians > 2*M_PI)
        CelBody->orbital_angle = 0.0;

    CelBody->pos.x = CelBody->orbit_center->x + ((cos(radians))*CelBody->orbit_radius);
    CelBody->pos.y = CelBody->orbit_center->y + ((sin(radians))*CelBody->orbit_radius);

    CelBody->orbital_angle = radians;
}

void move_celbodies()
{
    int i = 0;

    CelestialBody* CelBody;
    
    while (i < Num_CelBodies)
    {
        if (celbodies[i].orbit_center == NULL) // the Sun doesn't move
            i++;
        else
        {
            CelBody = &celbodies[i];
            updateCelBody(CelBody);
            i++;
        }
    }
}

void set_celestial_pos()
{
    celbodies[Sun].pos.x = SCREEN_WIDTH / 2;
    celbodies[Sun].pos.y = SCREEN_HEIGHT / 2;

    celbodies[Mercury].pos.x = celbodies[Mercury].orbit_center->x + celbodies[Mercury].orbit_radius;
    celbodies[Mercury].pos.y = celbodies[Mercury].orbit_center->y;
    
    celbodies[Venus].pos.x = celbodies[Venus].orbit_center->x + celbodies[Venus].orbit_radius;
    celbodies[Venus].pos.y = celbodies[Venus].orbit_center->y;

    celbodies[Earth].pos.x = celbodies[Earth].orbit_center->x + celbodies[Earth].orbit_radius;
    celbodies[Earth].pos.y = celbodies[Earth].orbit_center->y;

    celbodies[Moon].pos.x = celbodies[Moon].orbit_center->x + celbodies[Moon].orbit_radius;
    celbodies[Moon].pos.y = celbodies[Moon].orbit_center->y;

    celbodies[MoonProbe].pos.x = celbodies[MoonProbe].orbit_center->x + celbodies[MoonProbe].orbit_radius;
    celbodies[MoonProbe].pos.y = celbodies[MoonProbe].orbit_center->y;

    celbodies[Mars].pos.x = celbodies[Mars].orbit_center->x + celbodies[Mars].orbit_radius;
    celbodies[Mars].pos.y = celbodies[Mars].orbit_center->y;

    celbodies[MarsProbe].pos.x = celbodies[MarsProbe].orbit_center->x + celbodies[MarsProbe].orbit_radius;
    celbodies[MarsProbe].pos.y = celbodies[MarsProbe].orbit_center->y;
}

int main()
{   
    int ticks = 0;
    float speed = 0.0;

    printf("Enter the speed at which you want to run the simulation.\n");
    printf("Enter a negative value for a reverse simulation.\n");
    scanf("%f", &speed);

    angle_scale = angle_scale * speed;

    set_celestial_pos();
    set_mode(VGA_256_COLOR_MODE);
    
    _fmemset(screen_buf, 0, SCREEN_SIZE);

    draw_orbits();
    draw_celbodies();
    memcpy(VGA,screen_buf,SCREEN_SIZE);

    while (ticks < 16385)
    {
        _fmemset(screen_buf, 0, SCREEN_SIZE);
        move_celbodies();
        draw_orbits();
        draw_celbodies();
        memcpy(VGA,screen_buf,SCREEN_SIZE);
        ticks++;
        delay(10);
    }
    return 0;
}