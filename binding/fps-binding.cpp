/*
 ** fps-binding.cpp
 */

#include "fps/firstperson.h"
#include "sharedstate.h"
#include "binding-util.h"
#include "binding-types.h"
#include "exception.h"
#include "util/debugwriter.h"

RB_METHOD(fpsInitialize)
{
    RB_UNUSED_PARAM;

    Bitmap *screen;
    Bitmap *textures;
    VALUE screenObj, texturesObj, world,
            position, direction, plane;
    int resolution;

    rb_get_args(argc, argv, "ooooooi", &screenObj, &texturesObj, &world,
                &position, &direction, &plane, &resolution RB_ARG_END);
                
    screen = getPrivateDataCheck<Bitmap>(screenObj, BitmapType);
    textures = getPrivateDataCheck<Bitmap>(texturesObj, BitmapType);
    shState->firstPerson().initialize(screen, textures, world, position,
                                    direction, plane, resolution);
    
    return Qnil;
}

RB_METHOD(fpsTerminate)
{
    RB_UNUSED_PARAM;
    shState->firstPerson().terminate();    
    return Qnil;
}

RB_METHOD(fpsRender3dWalls)
{
    RB_UNUSED_PARAM;
    shState->firstPerson().render3dWalls();    
    return Qnil;
}

RB_METHOD(fpsRenderSprite)
{
    RB_UNUSED_PARAM;

    Bitmap *sprite;
    VALUE spriteObj;
    double x, y, z;
    double scaleX, scaleY;
    int characterIndex, direction, pattern;
    int dw, dh;
    int flags;

    rb_get_args(argc, argv, "offfffiiiiii", &spriteObj, &x, &y,
                &z, &scaleX, &scaleY, &characterIndex, &direction,
                &pattern, &dw, &dh, &flags RB_ARG_END);
    sprite = getPrivateDataCheck<Bitmap>(spriteObj, BitmapType);

    shState->firstPerson().renderSprite(sprite, x, y, z, scaleX, scaleY,
                            characterIndex, direction, pattern, dw, dh, flags);    
    return Qnil;
}

RB_METHOD(fpsCastSingleRay)
{
    RB_UNUSED_PARAM;

    double objectX, objectY, spriteScaleX;
    VALUE coord = rb_ary_new();

    rb_get_args(argc, argv, "fff", &objectX, &objectY, &spriteScaleX RB_ARG_END);
    shState->firstPerson().castSingleRay(objectX, objectY, spriteScaleX, coord);

    return coord;
}

#define INIT_GRA_PROP_BIND(PropName, prop_name_s) \
{ \
_rb_define_module_function(module, prop_name_s, graphics##Get##PropName); \
_rb_define_module_function(module, prop_name_s "=", graphics##Set##PropName); \
}

void fpsBindingInit()
{
    VALUE module = rb_define_module("FirstPerson");
    
    _rb_define_module_function(module, "initialize", fpsInitialize);
    _rb_define_module_function(module, "terminate", fpsTerminate);
    _rb_define_module_function(module, "render_3d_walls", fpsRender3dWalls);
    _rb_define_module_function(module, "render_sprite", fpsRenderSprite);
    _rb_define_module_function(module, "cast_single_ray", fpsCastSingleRay);
}
