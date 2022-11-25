#include <stdio.h>
#include <math.h>
#include "firstperson.h"

struct FirstPersonPrivate {

    FirstPersonPrivate() {}

    ~FirstPersonPrivate() {
        delete zBuffer;
        delete pixels;
    }
    
        /*
            Members used, unmodified for the entirety of the life cycle
        */

        Bitmap *bitmap; // Pointer to entire screen's bitmap
        Bitmap *textures; // Pointer to bitmap of all textures
        uint8_t *pixels; // Buffer to write new pixels to bitmap
        uint8_t bytesPerPixel;
        size_t pixelsSize;

        int screenHeight;
        int screenWidth;
        int resolution;

        VALUE worldA; //Pointer to 2D array for world grid
        int worldXLength, worldYLength;
        VALUE playerPosA;
        VALUE playerDirA;
        VALUE planeA;
        VALUE npcsA;

        /*
            Members used for the entire lifecycle that will be modified
        */

        double *zBuffer;

        Color pixel;
        Color color;

        double playerX, playerY;
        double playerDirX, playerDirY;
        double planeX, planeY;

        int texWidth;
        int texHeight;
        int textureId;
        int texX, texY;
            
        double fogDist;
        double fogWeight;
        double wallFloorCeilWeight;
        const float fogDistCutoff = 6.0; // Distance after which the fog is factored into color
        float shade;
        unsigned char fogRed = 31; // TODO: Consider changing this to reading from the game's own color values
        unsigned char fogGreen = 31;
        unsigned char fogBlue = 31;

};

FirstPerson::FirstPerson() {
    p = new FirstPersonPrivate();
}

FirstPerson::~FirstPerson() { delete p; }

void FirstPerson::initialize(Bitmap *screen, Bitmap *textures, VALUE world, VALUE position,
                            VALUE direction, VALUE plane, int resolution) {
    // TODO: Work around for the VALUE arguments so we don't need to include ruby.h here
    p->bitmap = screen;
    p->screenWidth = p->bitmap->width();
    p->screenHeight = p->bitmap->height();
    p->textures = textures;
	p->texHeight = p->textures->height();
	p->texWidth = p->textures->height(); // Assume square textures
    p->worldA = world;
    p->worldXLength = RARRAY_LEN(world);
    p->worldYLength = RARRAY_LEN(rb_ary_entry(world, 0));
    p->playerPosA = position;
    p->playerDirA = direction;
    p->planeA = plane;
    p->resolution = resolution;
    p->zBuffer = new double[p->screenWidth/p->resolution];
    // We're cheating and saying 4 here since BytesPerPixel is hidden in the private format field of Bitmap
    p->bytesPerPixel = 4;
    p->pixelsSize = p->bytesPerPixel * p->screenWidth * p->screenHeight;
    p->pixels = new uint8_t[p->pixelsSize];
}

void FirstPerson::terminate() {
    delete p->zBuffer;
    delete p->pixels;
}

void FirstPerson::render3dWalls() {
    
	/*
		Coordinate Info
		TODO: Find some way to use pointers and avoid reassignment.
        This should now be possible in mkxp-z. Maybe.
	*/

	p->playerX = RFLOAT_VALUE(rb_ary_entry(p->playerPosA, 0));
	p->playerY = RFLOAT_VALUE(rb_ary_entry(p->playerPosA, 1));
	
	p->playerDirX = RFLOAT_VALUE(rb_ary_entry(p->playerDirA, 0));
	p->playerDirY = RFLOAT_VALUE(rb_ary_entry(p->playerDirA, 1));
	
	p->planeX = RFLOAT_VALUE(rb_ary_entry(p->planeA, 0));
	p->planeY = RFLOAT_VALUE(rb_ary_entry(p->planeA, 1));

    double cameraX;
    double rayDirX, rayDirY;
    double currentFloorX, currentFloorY;
    double wallX, floorXWall, floorYWall;

    double distX, distY;
    double sideDistX, sideDistY;
    double perpWallDist, currentDist;
    
    double drawStart, drawEnd;
    double weight;
    int lineHeight;

    int side;
    int mapX, mapY;
    int stepX, stepY;
    int floorTexX, floorTexY;    
    int pixel;
    int textureFormat, textureShift;
    float wallFloorShade;

	for(int column = 0; column < p->screenWidth; column += p->resolution) {

        // Logically, these could be calculated before the loop, but trying to do
        // so leads to a segfault. Why?
        mapX = int(p->playerX);
        mapY = int(p->playerY);

		cameraX = 2*column/double(p->screenWidth)-1;
		rayDirX = p->playerDirX + p->planeX*cameraX;
		rayDirY = p->playerDirY + p->planeY*cameraX;
		
		distX = (rayDirX == 0) ? 1e30 : abs(1 / rayDirX);
		distY = (rayDirY == 0) ? 1e30 : abs(1 / rayDirY);
		
		if(rayDirX < 0) {
			stepX = -1;
			sideDistX = (p->playerX - mapX) * distX;
		} else {
			stepX = 1;
			sideDistX = (mapX + 1.0 - p->playerX) * distX;
		}
		
		if(rayDirY < 0) {
			stepY = -1;
			sideDistY = (p->playerY - mapY) * distY;
		} else {
			stepY = 1;
			sideDistY = (mapY + 1.0 - p->playerY) * distY;
		}
		
		while(1){ // DDA
			// Jump to next map square, or in xdir, or in ydir
			if(sideDistX < sideDistY){
				sideDistX += distX;
				mapX += stepX;
				side = 0;
			}else{
				sideDistY += distY;
				mapY += stepY;
				side = 1;
			}
			
			p->textureId = ARRAY_2D_GET(p->worldA, mapX, mapY);
			if(p->textureId > 0 && p->textureId < 255) break; // Ray has hit a wall
		}
        
		if(side == 0) {
			perpWallDist = sideDistX - distX;
			wallX = p->playerY + perpWallDist * rayDirY;			
		} else {
			perpWallDist = sideDistY - distY;
			wallX = p->playerX + perpWallDist * rayDirX;
		}
 		p->zBuffer[column/p->resolution] = perpWallDist;
		wallX -= floor(wallX);
		// If wallX is a whole number, we get 0, which throws off calculations
		if(wallX == 0) wallX = 0.999;
		p->texX = int(double(p->texWidth) * (1 - wallX));
		// Calculate direction hitting wall for floor purposes
		if(side == 0 && rayDirX > 0) {
			floorXWall = mapX;
			floorYWall = mapY + wallX;
			p->texX = p->texWidth - p->texX - 1; // Necessary to keep texture from flipping
		} else if(side == 0 && rayDirX < 0) {
			floorXWall = mapX + 1.0;
			floorYWall = mapY + wallX;
		} else if(side == 1 && rayDirY > 0) {
			floorXWall = mapX + wallX;
			floorYWall = mapY;
		} else {
			floorXWall = mapX + wallX;
			floorYWall = mapY + 1.0;
			p->texX = p->texWidth - p->texX - 1; // Necessary to keep texture from flipping
		}
		// Up to this point, p->texX is relative to a grid. Set it to the absolute position
		// in the textures bitmap.
		p->texX += p->texWidth * p->textureId;

		// The height of the "wall" section of this vertical strip
		lineHeight = int(p->screenHeight/(perpWallDist+0.00001));
		
		drawStart = (p->screenHeight - lineHeight) / 2;
		drawEnd = (p->screenHeight + lineHeight) / 2 + 1;
		
		/*
		#--------------------------------------------------------------------------
		# * Draw level to the bitmap
		#--------------------------------------------------------------------------
		*/
		int endX = column + p->resolution;
		
		p->fogDist = std::min(float(std::max(perpWallDist*0.75, 1.0)), p->fogDistCutoff);
		p->fogWeight = (p->fogDist-1.0)/(p->fogDistCutoff-1.0);
		p->wallFloorCeilWeight = 1.0-p->fogWeight;
		p->shade = side == 1 ? 0.8 : 1.0;
        
		for(int y = 0; y < p->screenHeight; y++) {
            if(y <= drawEnd && y >= drawStart) { // Wall
				p->texY = std::min(p->texHeight - 1, int((float(y-drawStart) / lineHeight) * p->texHeight));
				p->color = p->textures->getPixel(p->texX, p->texY);
                wallFloorShade = p->shade;
            } else { // Floor or Ceiling
                wallFloorShade = 1.0;
                if(y > drawEnd) { // Floor
				    currentDist = p->screenHeight / (2.0 * y - p->screenHeight);
                    textureFormat = 0x00FF00;
                    textureShift = 8;
                } else { // Ceiling
				    currentDist = p->screenHeight / (p->screenHeight - 2.0 * y );
                    textureFormat = 0xFF0000;
                    textureShift = 16;
                }
				weight = currentDist / perpWallDist;
				currentFloorX = weight*floorXWall + (1.0 - weight) * p->playerX;
				currentFloorY = weight*floorYWall + (1.0 - weight) * p->playerY;
				
				p->fogDist = std::min(float(std::max(currentDist*0.75, 1.0)), p->fogDistCutoff);
				p->fogWeight = (p->fogDist - 1.0)/(p->fogDistCutoff - 1.0);
				p->wallFloorCeilWeight = 1.0 - p->fogWeight;
				floorTexX = int(abs(currentFloorX * p->texWidth)) % p->texWidth; // Need abs, else negative % will crash
				floorTexY = int(abs(currentFloorY * p->texHeight)) % p->texHeight; // Need abs, else negative % will crash

				// Get floor texture ID
				if (currentFloorX >= p->worldXLength || currentFloorY >= p->worldYLength) {
					p->textureId = 0;
				} else {
					p->textureId = ARRAY_2D_GET(p->worldA, int(currentFloorX), int(currentFloorY));
					// Texture format is 0xCCFFWW
					p->textureId &= textureFormat;
					p->textureId >>= textureShift;
				}

				floorTexX += (p->texWidth * p->textureId);
				p->color = p->textures->getPixel(floorTexX, floorTexY);
			}

            // Draw pixels
            pixel = (column + (y * p->screenWidth)) * p->bytesPerPixel;
            for(int x = column; x < endX; x++) {
                // TODO: Not strictly needed if screenWidth is divisible by resolution
                if(x >= p->screenWidth) {
                    break; 
                }
                p->pixels[pixel++] = (p->color.red * wallFloorShade) * p->wallFloorCeilWeight + (p->fogRed * p->fogWeight);
                p->pixels[pixel++] = (p->color.green * wallFloorShade) * p->wallFloorCeilWeight + (p->fogGreen * p->fogWeight);
                p->pixels[pixel++] = (p->color.blue * wallFloorShade) * p->wallFloorCeilWeight + (p->fogBlue * p->fogWeight);
                p->pixels[pixel++] = p->color.alpha;
            }
		}
	}
    // Write the pixel data directly to the buffer
    p->bitmap->replaceRaw(p->pixels, p->pixelsSize);
}

void FirstPerson::renderSprite(Bitmap *sprite, double spriteX, double spriteY, double spriteZ, double spriteScaleX, double spriteScaleY, int characterIndex, int direction, int pattern, int dw, int dh, int flags) {
	// TODO: May want to have pointers for these values to prevent needing to fetch them every time
	double playerX = RFLOAT_VALUE(rb_ary_entry(p->playerPosA, 0));
	double playerY = RFLOAT_VALUE(rb_ary_entry(p->playerPosA, 1));

	double playerDirX = RFLOAT_VALUE(rb_ary_entry(p->playerDirA, 0));
	double playerDirY = RFLOAT_VALUE(rb_ary_entry(p->playerDirA, 1));
	
	double planeX = RFLOAT_VALUE(rb_ary_entry(p->planeA, 0));
	double planeY = RFLOAT_VALUE(rb_ary_entry(p->planeA, 1));

    int bitmapWidth = sprite->width();
    int bitmapHeight = sprite->height();

	spriteX = spriteX - playerX + 0.5; // Add 0.5 to center sprite in its tile
	spriteY = spriteY - playerY + 0.5; // Add 0.5 to center sprite in its tile
	int sx, sy;

	if ((flags & IS_ANIMATION) == IS_ANIMATION) {
		// Rendering animation sprite, ignore angle and character logic
		sx = (pattern % 5) * (bitmapWidth / dw); //(pattern) * (sprite->width() / dw);
		sy = (pattern / 5) * (bitmapHeight / dh); //(sprite->height() / dh);
	} else {

		// 8-dir spritesheet format (with stepping animation):
		/*
			|	111	|	666	|
			|	222	|	777	|
			|	333	|	888	|
			|	444	|	999	|
		*/
		// Without animation:
		/*
			|	1	|	6	|
			|	2	|	7	|
			|	3	|	8	|
			|	4	|	9	|
		*/

		if ((flags & DIRECTION_FIX) != DIRECTION_FIX) {
			double spriteAngle = atan2(-spriteY, -spriteX);
			// Range: -PI to PI
			if(ANGLE_4_7 < spriteAngle && ANGLE_4_6 >= spriteAngle) {
				direction = directions[direction][4];
			} else if(ANGLE_4_6 < spriteAngle && ANGLE_2_6 >= spriteAngle) {
				direction = directions[direction][1];
			} else if(ANGLE_2_6 < spriteAngle && ANGLE_2_8 >= spriteAngle) {
				direction = directions[direction][2];
			} else if((ANGLE_2_8 < spriteAngle && ANGLE_8_A >= spriteAngle) || (ANGLE_8_B < spriteAngle && ANGLE_8_9 >= spriteAngle)) {
				direction = directions[direction][3];
			} else if(ANGLE_8_9 < spriteAngle && ANGLE_3_9 >= spriteAngle) {
				direction = directions[direction][9];
			} else if(ANGLE_3_9 < spriteAngle && ANGLE_3_7 >= spriteAngle) {
				direction = directions[direction][8];
			} else if(ANGLE_3_7 < spriteAngle && ANGLE_4_7 >= spriteAngle) {
				direction = directions[direction][7];
			} else { // +-pi
				direction = directions[direction][6];
			}
		}

		// Loop stepping animation
		pattern = pattern < STEPPING_ANIMATION_FRAMES ? pattern : 1;

		int frames = (flags & NO_ANIMATION) == NO_ANIMATION ? 1 : STEPPING_ANIMATION_FRAMES;
		sx = ((characterIndex % 4 * frames + pattern) + (direction / 5) * frames) * (sprite->width() / dw);
		sy = (characterIndex / 4 * 4) + ((direction - 1) % 5) * (sprite->height() / dh);
	}
	
	//transform sprite with the inverse camera matrix
	float invDet = 1.0 / (planeX * playerDirY - playerDirX * planeY);
	
	float transformX = invDet * (playerDirY * spriteX - playerDirX * spriteY);
	float transformY = invDet * (-planeY * spriteX + planeX * spriteY);
	
	int zMoveScreen = int(spriteZ / transformY);
	
	int spriteScreenX = int((p->screenWidth / 2) * (1 + transformX / transformY));
	
	int spriteHeight = abs(int(p->screenHeight / (transformY))) * spriteScaleY; // Prevents fisheye effect
	
	int drawStartY = (p->screenHeight - spriteHeight) / 2 + zMoveScreen;
	if (drawStartY > p->screenHeight) return;
	if(drawStartY < 0) drawStartY = 0;
	int drawEndY = (p->screenHeight + spriteHeight) / 2 + zMoveScreen;
	if(drawEndY < 0) return;
	if(drawEndY >= p->screenHeight) drawEndY = p->screenHeight; //-1
	
	int spriteWidth = abs(int (p->screenHeight / (transformY))) * spriteScaleX;
	int drawStartX = spriteScreenX - spriteWidth/2;
	if (drawStartX > p->screenWidth) return;
	if(drawStartX < 0) drawStartX = 0;
	int drawEndX = spriteWidth/2 + spriteScreenX;
	if(drawEndX < 0) return;
	if(drawEndX >= p->screenWidth) drawEndX = p->screenWidth; //-1
	
    int pixel;
	
	p->fogDist = std::min(float(std::max(transformY * 0.75, 1.0)), p->fogDistCutoff);
	p->fogWeight = (p->fogDist - 1.0) / (p->fogDistCutoff - 1.0); //5.0
	p->wallFloorCeilWeight = 1.0 - p->fogWeight;
	
	double spriteTexHeight = double(bitmapHeight * 2 / (dh*2));
	double spriteTexWidth = double(bitmapWidth / dw);
	int d;

	for(int y=drawStartY; y<drawEndY; y++) {
		d = (y - zMoveScreen) * 256 - p->screenHeight*128 + spriteHeight * 128;
		if((flags & FLIP_VERTICAL) == FLIP_VERTICAL) {
			p->texY = spriteTexHeight - abs(((d * spriteTexHeight) / spriteHeight) / 256) + sy;
		} else {
			p->texY = abs(((d * spriteTexHeight) / spriteHeight) / 256) + sy;
		}
		
		
		for(int stripe = drawStartX; stripe < drawEndX; stripe++) {
            pixel = (stripe + (y * p->screenWidth)) * p->bytesPerPixel;
 			p->texX = abs(int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * spriteTexWidth / spriteWidth) / 256) + sx;
			
			if(transformY > 0 && stripe >= 0 && stripe < p->screenWidth && transformY < p->zBuffer[stripe/p->resolution]) {
				p->color = sprite->getPixel(p->texX, p->texY);
				// If totally transparent, ignore
				if(p->color.alpha > 0) {
                    p->pixels[pixel] = (p->pixels[pixel]*(1.0-(p->color.alpha/255.0)) + p->color.red*(p->color.alpha/255.0))*p->wallFloorCeilWeight + (p->fogRed*p->fogWeight);
                    p->pixels[pixel+1] = (p->pixels[pixel+1]*(1.0-(p->color.alpha/255.0)) + p->color.green*(p->color.alpha/255.0))*p->wallFloorCeilWeight + (p->fogGreen*p->fogWeight);
                    p->pixels[pixel+2] = (p->pixels[pixel+2]*(1.0-(p->color.alpha/255.0)) + p->color.blue*(p->color.alpha/255.0))*p->wallFloorCeilWeight + (p->fogBlue*p->fogWeight);
                    p->pixels[pixel+3] = 255;
				}
				
			}
            pixel += 4;
		}
	}
    // TODO: May want to only write the pixels that were changed
    // drawStartX to drawEndX, drawStartY to drawEndY
    p->bitmap->replaceRaw(p->pixels, p->pixelsSize);
}

void FirstPerson::castSingleRay(double objectX, double objectY, double spriteScaleX, VALUE coord) {
    // TODO: May want to have pointers for these values to prevent needing to fetch them every time
	double playerX = RFLOAT_VALUE(rb_ary_entry(p->playerPosA, 0));
	double playerY = RFLOAT_VALUE(rb_ary_entry(p->playerPosA, 1));
	
	double playerDirX = RFLOAT_VALUE(rb_ary_entry(p->playerDirA, 0));
	double playerDirY = RFLOAT_VALUE(rb_ary_entry(p->playerDirA, 1));
	
	double planeX = RFLOAT_VALUE(rb_ary_entry(p->planeA, 0));
	double planeY = RFLOAT_VALUE(rb_ary_entry(p->planeA, 1));
	
	// Translate sprite position to be relative to camera
	double spriteX = objectX - playerX + 0.5; // Add 0.5 to center sprite in its tile
	double spriteY = objectY - playerY + 0.5; // Add 0.5 to center sprite in its tile
	
	// Transform sprite with the inverse camera matrix
	float invDet = 1.0 / (planeX * playerDirY - playerDirX * planeY);
	
	float transformX = invDet * (playerDirY * spriteX - playerDirX * spriteY);
	float transformY = invDet * (-planeY * spriteX + planeX * spriteY);
	
	int spriteScreenX = int((p->screenWidth / 2) * (1 + transformX / transformY));
	
	int spriteWidth = abs( int (p->screenHeight / (transformY))) * spriteScaleX;
	int drawStartX = spriteScreenX - spriteWidth/2;
	if(drawStartX < 0) drawStartX = 0;
	int drawEndX = spriteWidth/2 + spriteScreenX;
	if(drawEndX >= p->screenWidth) drawEndX = p->screenWidth; //-1
	
	for(int stripe = drawStartX; stripe < drawEndX; stripe++){
 		p->texX = abs(int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * p->texWidth / spriteWidth) / 256);
		
        // Object is on screen
		if(transformY > 0 && stripe >= 0 && stripe < p->screenWidth && transformY < p->zBuffer[stripe/p->resolution]){
            rb_ary_push(coord, INT2FIX(drawStartX));
            rb_ary_push(coord, INT2FIX(drawEndX));
            return;
		}
	}
	// Object was not hit
    rb_ary_push(coord, INT2FIX(-1));
    rb_ary_push(coord, INT2FIX(-1));
}