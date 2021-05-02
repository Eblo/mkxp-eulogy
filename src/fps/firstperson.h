#ifndef FIRSTPERSON_H
#define FIRSTPERSON_H

#include <ruby.h>
#include "bitmap.h"

#define ARRAY_2D_GET(a, x, y) FIX2INT(rb_ary_entry(rb_ary_entry(a, x), y));

#define SPRITE_TRANSLUCENCY 1
#define STEPPING_ANIMATION_FRAMES 3

// Sprite flags
#define FLIP_VERTICAL	0b00000001
#define FLIP_HORIZONTAL	0b00000010
#define DIRECTION_FIX	0b00000100
#define NO_ANIMATION	0b00001000
#define IS_ANIMATION	0b00010000

// Sprite angles
#define ANGLE_2_6 1.1780972
#define ANGLE_2_8 1.9634954
#define ANGLE_3_7 -ANGLE_2_6
#define ANGLE_3_9 -ANGLE_2_8
#define ANGLE_4_6 0.3926991
#define ANGLE_4_7 -ANGLE_4_6
#define ANGLE_8_A 2.7488936
#define ANGLE_8_B 0
#define ANGLE_8_9 -ANGLE_8_A

struct FirstPersonPrivate;

class FirstPerson
{
    public:

        FirstPerson();
        ~FirstPerson();

        void initialize(Bitmap *screen, Bitmap *textures, VALUE world, VALUE position,
                        VALUE direction, VALUE plane, int resolution);
        void terminate();
        void render3dWalls();
        void renderSprite(Bitmap *sprite, double spriteX, double spriteY, double spriteZ, double spriteScaleX, double spriteScaleY, int characterIndex, int direction, int pattern, int dw, int dh, int flags);
        void castSingleRay(double objectX, double objectY, double spriteScaleX, VALUE coord);

    private:
	
        FirstPersonPrivate *p;

        // Format: directions[direction][angle]
        // 0 and 5 are unused, hence the 0s
        const char directions[10][10] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 0
            {0, 4, 1, 2, 7, 0, 3, 8, 9, 6}, // 1
            {0, 1, 2, 3, 4, 0, 6, 7, 8, 9}, // 2
            {0, 2, 3, 6, 1, 0, 9, 4, 7, 8}, // 3
            {0, 7, 4, 1, 8, 0, 2, 9, 6, 3}, // 4
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 5
            {0, 3, 6, 9, 2, 0, 8, 1, 4, 7}, // 6	
            {0, 8, 7, 4, 9, 0, 1, 6, 3, 2}, // 7
            {0, 9, 8, 7, 6, 0, 4, 3, 2, 1}, // 8	
            {0, 6, 9, 8, 3, 0, 7, 2, 1, 4}, // 9
        };
        
        
};

#endif // FIRSTPERSON_H