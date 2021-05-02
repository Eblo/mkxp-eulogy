/*
** bitmap-binding.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
**
** mkxp is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** mkxp is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "binding-types.h"
#include "binding-util.h"
#include "bitmap.h"
#include "disposable-binding.h"
#include "exception.h"
#include "font.h"
#include "sharedstate.h"

#if RAPI_FULL > 187
DEF_TYPE(Bitmap);
#else
DEF_ALLOCFUNC(Bitmap);
#endif

static const char *objAsStringPtr(VALUE obj) {
  VALUE str = rb_obj_as_string(obj);
  return RSTRING_PTR(str);
}

void bitmapInitProps(Bitmap *b, VALUE self) {
  /* Wrap properties */
  VALUE fontKlass = rb_const_get(rb_cObject, rb_intern("Font"));
  VALUE fontObj = rb_obj_alloc(fontKlass);
  rb_obj_call_init(fontObj, 0, 0);

  Font *font = getPrivateData<Font>(fontObj);
  b->setInitFont(font);

  rb_iv_set(self, "font", fontObj);
}

RB_METHOD(bitmapInitialize) {
  Bitmap *b = 0;

  if (argc == 1) {
    char *filename;
    rb_get_args(argc, argv, "z", &filename RB_ARG_END);

    GUARD_EXC(b = new Bitmap(filename);)
  } else {
    int width, height;
    rb_get_args(argc, argv, "ii", &width, &height RB_ARG_END);

    GUARD_EXC(b = new Bitmap(width, height);)
  }

  setPrivateData(self, b);
  bitmapInitProps(b, self);

  return self;
}

RB_METHOD(bitmapWidth) {
  RB_UNUSED_PARAM;

  Bitmap *b = getPrivateData<Bitmap>(self);

  int value = 0;
  GUARD_EXC(value = b->width(););

  return INT2FIX(value);
}

RB_METHOD(bitmapHeight) {
  RB_UNUSED_PARAM;

  Bitmap *b = getPrivateData<Bitmap>(self);

  int value = 0;
  GUARD_EXC(value = b->height(););

  return INT2FIX(value);
}

RB_METHOD(bitmapRect) {
  RB_UNUSED_PARAM;

  Bitmap *b = getPrivateData<Bitmap>(self);

  IntRect rect;
  GUARD_EXC(rect = b->rect(););

  Rect *r = new Rect(rect);

  return wrapObject(r, RectType);
}

RB_METHOD(bitmapBlt) {
  Bitmap *b = getPrivateData<Bitmap>(self);

  int x, y;
  VALUE srcObj;
  VALUE srcRectObj;
  int opacity = 255;

  Bitmap *src;
  Rect *srcRect;

  rb_get_args(argc, argv, "iioo|i", &x, &y, &srcObj, &srcRectObj,
              &opacity RB_ARG_END);

  src = getPrivateDataCheck<Bitmap>(srcObj, BitmapType);
  srcRect = getPrivateDataCheck<Rect>(srcRectObj, RectType);

  GUARD_EXC(b->blt(x, y, *src, srcRect->toIntRect(), opacity););

  return self;
}

RB_METHOD(bitmapStretchBlt) {
  Bitmap *b = getPrivateData<Bitmap>(self);

  VALUE destRectObj;
  VALUE srcObj;
  VALUE srcRectObj;
  int opacity = 255;

  Bitmap *src;
  Rect *destRect, *srcRect;

  rb_get_args(argc, argv, "ooo|i", &destRectObj, &srcObj, &srcRectObj,
              &opacity RB_ARG_END);

  src = getPrivateDataCheck<Bitmap>(srcObj, BitmapType);
  destRect = getPrivateDataCheck<Rect>(destRectObj, RectType);
  srcRect = getPrivateDataCheck<Rect>(srcRectObj, RectType);

  GUARD_EXC(b->stretchBlt(destRect->toIntRect(), *src, srcRect->toIntRect(),
                          opacity););

  return self;
}

RB_METHOD(bitmapFillRect) {
  Bitmap *b = getPrivateData<Bitmap>(self);

  VALUE colorObj;
  Color *color;

  if (argc == 2) {
    VALUE rectObj;
    Rect *rect;

    rb_get_args(argc, argv, "oo", &rectObj, &colorObj RB_ARG_END);

    rect = getPrivateDataCheck<Rect>(rectObj, RectType);
    color = getPrivateDataCheck<Color>(colorObj, ColorType);

    GUARD_EXC(b->fillRect(rect->toIntRect(), color->norm););
  } else {
    int x, y, width, height;

    rb_get_args(argc, argv, "iiiio", &x, &y, &width, &height,
                &colorObj RB_ARG_END);

    color = getPrivateDataCheck<Color>(colorObj, ColorType);

    GUARD_EXC(b->fillRect(x, y, width, height, color->norm););
  }

  return self;
}

RB_METHOD(bitmapClear) {
  RB_UNUSED_PARAM;

  Bitmap *b = getPrivateData<Bitmap>(self);

  GUARD_EXC(b->clear();)

  return self;
}

RB_METHOD(bitmapGetPixel) {
  Bitmap *b = getPrivateData<Bitmap>(self);

  int x, y;

  rb_get_args(argc, argv, "ii", &x, &y RB_ARG_END);

  Color value;
  GUARD_EXC(value = b->getPixel(x, y););

  Color *color = new Color(value);

  return wrapObject(color, ColorType);
}

RB_METHOD(bitmapSetPixel) {
  Bitmap *b = getPrivateData<Bitmap>(self);

  int x, y;
  VALUE colorObj;

  Color *color;

  rb_get_args(argc, argv, "iio", &x, &y, &colorObj RB_ARG_END);

  color = getPrivateDataCheck<Color>(colorObj, ColorType);

  GUARD_EXC(b->setPixel(x, y, *color););

  return self;
}

RB_METHOD(bitmapHueChange) {
  Bitmap *b = getPrivateData<Bitmap>(self);

  int hue;

  rb_get_args(argc, argv, "i", &hue RB_ARG_END);

  GUARD_EXC(b->hueChange(hue););

  return self;
}

RB_METHOD(bitmapDrawText) {
  Bitmap *b = getPrivateData<Bitmap>(self);

  const char *str;
  int align = Bitmap::Left;

  if (argc == 2 || argc == 3) {
    VALUE rectObj;
    Rect *rect;

    if (rgssVer >= 2) {
      VALUE strObj;
      rb_get_args(argc, argv, "oo|i", &rectObj, &strObj, &align RB_ARG_END);

      str = objAsStringPtr(strObj);
    } else {
      rb_get_args(argc, argv, "oz|i", &rectObj, &str, &align RB_ARG_END);
    }

    rect = getPrivateDataCheck<Rect>(rectObj, RectType);

    GUARD_EXC(b->drawText(rect->toIntRect(), str, align););
  } else {
    int x, y, width, height;

    if (rgssVer >= 2) {
      VALUE strObj;
      rb_get_args(argc, argv, "iiiio|i", &x, &y, &width, &height, &strObj,
                  &align RB_ARG_END);

      str = objAsStringPtr(strObj);
    } else {
      rb_get_args(argc, argv, "iiiiz|i", &x, &y, &width, &height, &str,
                  &align RB_ARG_END);
    }

    GUARD_EXC(b->drawText(x, y, width, height, str, align););
  }

  return self;
}

RB_METHOD(bitmapTextSize) {
  Bitmap *b = getPrivateData<Bitmap>(self);

  const char *str;

  if (rgssVer >= 2) {
    VALUE strObj;
    rb_get_args(argc, argv, "o", &strObj RB_ARG_END);

    str = objAsStringPtr(strObj);
  } else {
    rb_get_args(argc, argv, "z", &str RB_ARG_END);
  }

  IntRect value;
  GUARD_EXC(value = b->textSize(str););

  Rect *rect = new Rect(value);

  return wrapObject(rect, RectType);
}

DEF_PROP_OBJ_VAL(Bitmap, Font, Font, "font")

RB_METHOD(bitmapGradientFillRect) {
  Bitmap *b = getPrivateData<Bitmap>(self);

  VALUE color1Obj, color2Obj;
  Color *color1, *color2;
  bool vertical = false;

  if (argc == 3 || argc == 4) {
    VALUE rectObj;
    Rect *rect;

    rb_get_args(argc, argv, "ooo|b", &rectObj, &color1Obj, &color2Obj,
                &vertical RB_ARG_END);

    rect = getPrivateDataCheck<Rect>(rectObj, RectType);
    color1 = getPrivateDataCheck<Color>(color1Obj, ColorType);
    color2 = getPrivateDataCheck<Color>(color2Obj, ColorType);

    GUARD_EXC(b->gradientFillRect(rect->toIntRect(), color1->norm, color2->norm,
                                  vertical););
  } else {
    int x, y, width, height;

    rb_get_args(argc, argv, "iiiioo|b", &x, &y, &width, &height, &color1Obj,
                &color2Obj, &vertical RB_ARG_END);

    color1 = getPrivateDataCheck<Color>(color1Obj, ColorType);
    color2 = getPrivateDataCheck<Color>(color2Obj, ColorType);

    GUARD_EXC(b->gradientFillRect(x, y, width, height, color1->norm,
                                  color2->norm, vertical););
  }

  return self;
}

RB_METHOD(bitmapClearRect) {
  Bitmap *b = getPrivateData<Bitmap>(self);

  if (argc == 1) {
    VALUE rectObj;
    Rect *rect;

    rb_get_args(argc, argv, "o", &rectObj RB_ARG_END);

    rect = getPrivateDataCheck<Rect>(rectObj, RectType);

    GUARD_EXC(b->clearRect(rect->toIntRect()););
  } else {
    int x, y, width, height;

    rb_get_args(argc, argv, "iiii", &x, &y, &width, &height RB_ARG_END);

    GUARD_EXC(b->clearRect(x, y, width, height););
  }

  return self;
}

RB_METHOD(bitmapBlur) {
  RB_UNUSED_PARAM;

  Bitmap *b = getPrivateData<Bitmap>(self);

  b->blur();

  return Qnil;
}

RB_METHOD(bitmapRadialBlur) {
  Bitmap *b = getPrivateData<Bitmap>(self);

  int angle, divisions;
  rb_get_args(argc, argv, "ii", &angle, &divisions RB_ARG_END);

  b->radialBlur(angle, divisions);

  return Qnil;
}

RB_METHOD(bitmapGetRawData) {
  RB_UNUSED_PARAM;

  Bitmap *b = getPrivateData<Bitmap>(self);
  int size = b->width() * b->height() * 4;
  VALUE ret = rb_str_new(0, size);

  GUARD_EXC(b->getRaw(RSTRING_PTR(ret), size););

  return ret;
}

RB_METHOD(bitmapSetRawData) {
  RB_UNUSED_PARAM;

  VALUE str;
  rb_scan_args(argc, argv, "1", &str);
  SafeStringValue(str);

  Bitmap *b = getPrivateData<Bitmap>(self);

  GUARD_EXC(b->replaceRaw(RSTRING_PTR(str), RSTRING_LEN(str)););

  return self;
}

RB_METHOD(bitmapSaveToFile) {
  RB_UNUSED_PARAM;

  VALUE str;
  rb_scan_args(argc, argv, "1", &str);
  SafeStringValue(str);

  Bitmap *b = getPrivateData<Bitmap>(self);

  try {
    b->saveToFile(RSTRING_PTR(str));
  } catch (const Exception &e) {
    raiseRbExc(e);
  }

  return RUBY_Qnil;
}

RB_METHOD(bitmapGetMega){
  RB_UNUSED_PARAM;

  rb_check_argc(argc, 0);

  Bitmap *b = getPrivateData<Bitmap>(self);

  return rb_bool_new(b->isMega());
}

RB_METHOD(bitmapGetAnimated){
    RB_UNUSED_PARAM;
    
    rb_check_argc(argc, 0);
    
    Bitmap *b = getPrivateData<Bitmap>(self);
    
    return rb_bool_new(b->isAnimated());
}

RB_METHOD(bitmapGetPlaying){
    RB_UNUSED_PARAM;
    
    rb_check_argc(argc, 0);
    
    Bitmap *b = getPrivateData<Bitmap>(self);
    
    return rb_bool_new(b->isPlaying());
}

RB_METHOD(bitmapSetPlaying){
    RB_UNUSED_PARAM;
    
    bool play;
    
    rb_get_args(argc, argv, "b", &play RB_ARG_END);
    
    Bitmap *b = getPrivateData<Bitmap>(self);
    
    (play) ? b->play() : b->stop();
    
    return RUBY_Qnil;
}

RB_METHOD(bitmapGotoStop){
    RB_UNUSED_PARAM;
    
    int frame;
    
    rb_get_args(argc, argv, "i", &frame RB_ARG_END);
    
    Bitmap *b = getPrivateData<Bitmap>(self);
    
    b->gotoAndStop(frame);
    
    return RUBY_Qnil;
}

RB_METHOD(bitmapGotoPlay){
    RB_UNUSED_PARAM;
    
    int frame;
    
    rb_get_args(argc, argv, "i", &frame RB_ARG_END);
    
    Bitmap *b = getPrivateData<Bitmap>(self);
    
    b->gotoAndPlay(frame);
    
    return RUBY_Qnil;
}

RB_METHOD(bitmapFrames){
    RB_UNUSED_PARAM;
    
    rb_check_argc(argc, 0);
    
    Bitmap *b = getPrivateData<Bitmap>(self);
    
    return INT2NUM(b->numFrames());
}

RB_METHOD(bitmapCurrentFrame){
    RB_UNUSED_PARAM;
    
    rb_check_argc(argc, 0);
    
    Bitmap *b = getPrivateData<Bitmap>(self);
    
    return INT2NUM(b->currentFrameI());
}

RB_METHOD(bitmapSetFPS){
    RB_UNUSED_PARAM;
    
    VALUE fps;
    rb_scan_args(argc, argv, "1", &fps);
    
    Bitmap *b = getPrivateData<Bitmap>(self);
    
    if (RB_TYPE_P(fps, RUBY_T_FLOAT)) {
        b->setAnimationFPS(RFLOAT_VALUE(fps));
    }
    else {
        b->setAnimationFPS(NUM2INT(fps));
    }
    
    return RUBY_Qnil;
}

RB_METHOD(bitmapGetFPS){
    RB_UNUSED_PARAM;
    
    rb_check_argc(argc, 0);
    
    Bitmap *b = getPrivateData<Bitmap>(self);
    return rb_float_new(b->getAnimationFPS());
}

RB_METHOD(bitmapSetLooping){
    RB_UNUSED_PARAM;
    
    bool loop;
    rb_get_args(argc, argv, "b", &loop RB_ARG_END);
    
    Bitmap *b = getPrivateData<Bitmap>(self);
    b->setLooping(loop);
    
    return RUBY_Qnil;
}

RB_METHOD(bitmapGetLooping){
    RB_UNUSED_PARAM;
    
    rb_check_argc(argc, 0);
    
    Bitmap *b = getPrivateData<Bitmap>(self);
    return rb_bool_new(b->getLooping());
}

RB_METHOD(bitmapGetMaxSize){
  RB_UNUSED_PARAM;

  rb_check_argc(argc, 0);

  return INT2NUM(Bitmap::maxSize());
}

RB_METHOD(bitmapInitializeCopy) {
  rb_check_argc(argc, 1);
  VALUE origObj = argv[0];

  if (!OBJ_INIT_COPY(self, origObj))
    return self;

  Bitmap *orig = getPrivateData<Bitmap>(origObj);
  Bitmap *b = 0;
  GUARD_EXC(b = new Bitmap(*orig););

  bitmapInitProps(b, self);
  b->setFont(orig->getFont());

  setPrivateData(self, b);

  return self;
}

RB_METHOD(bitmapMode7) {
  Bitmap *b = getPrivateData<Bitmap>(self);
  
  VALUE srcObj;
  double rotation, scale;
  int x, y;

  rb_get_args(argc, argv, "offii", &srcObj, &rotation, &scale, &x, &y RB_ARG_END);

  Bitmap *srcBitmap = getPrivateDataCheck<Bitmap>(srcObj, BitmapType);

  GUARD_EXC(b->setMode7(*srcBitmap, rotation, scale, x, y););

  return self;
}

RB_METHOD(bitmapTransform) {
  Bitmap *b = getPrivateData<Bitmap>(self);
  
  VALUE srcObj;
  int transformType, time, amplitude;
  double frequency, speed;

  rb_get_args(argc, argv, "oiiiff", &srcObj, &transformType, &time, &amplitude, &frequency, &speed RB_ARG_END);

  Bitmap *srcBitmap = getPrivateDataCheck<Bitmap>(srcObj, BitmapType);

  GUARD_EXC(b->setTransform(*srcBitmap, transformType, time, amplitude, frequency, speed););

  return self;
}

RB_METHOD(bitmapGetRawPNGData) {
  RB_UNUSED_PARAM;

  Bitmap *b = getPrivateData<Bitmap>(self);
	// Three bytes per pixel, plus one byte per row for the filter type
  int size = (b->width()+1) * b->height() * 3;
  VALUE ret = rb_str_new(0, size);

  GUARD_EXC(b->getRawForPNG(RSTRING_PTR(ret), size););

  return ret;
}

void bitmapBindingInit() {
  VALUE klass = rb_define_class("Bitmap", rb_cObject);
#if RAPI_FULL > 187
  rb_define_alloc_func(klass, classAllocate<&BitmapType>);
#else
  rb_define_alloc_func(klass, BitmapAllocate);
#endif

  disposableBindingInit<Bitmap>(klass);

  _rb_define_method(klass, "initialize", bitmapInitialize);
  _rb_define_method(klass, "initialize_copy", bitmapInitializeCopy);

  _rb_define_method(klass, "width", bitmapWidth);
  _rb_define_method(klass, "height", bitmapHeight);
  _rb_define_method(klass, "rect", bitmapRect);
  _rb_define_method(klass, "blt", bitmapBlt);
  _rb_define_method(klass, "stretch_blt", bitmapStretchBlt);
  _rb_define_method(klass, "fill_rect", bitmapFillRect);
  _rb_define_method(klass, "clear", bitmapClear);
  _rb_define_method(klass, "get_pixel", bitmapGetPixel);
  _rb_define_method(klass, "set_pixel", bitmapSetPixel);
  _rb_define_method(klass, "hue_change", bitmapHueChange);
  _rb_define_method(klass, "draw_text", bitmapDrawText);
  _rb_define_method(klass, "text_size", bitmapTextSize);

  _rb_define_method(klass, "raw_data", bitmapGetRawData);
  _rb_define_method(klass, "raw_data=", bitmapSetRawData);
  _rb_define_method(klass, "to_file", bitmapSaveToFile);

  _rb_define_method(klass, "gradient_fill_rect", bitmapGradientFillRect);
  _rb_define_method(klass, "clear_rect", bitmapClearRect);
  _rb_define_method(klass, "blur", bitmapBlur);
  _rb_define_method(klass, "radial_blur", bitmapRadialBlur);

  _rb_define_method(klass, "mega?", bitmapGetMega);

  // Eulogy stuff
  _rb_define_method(klass, "mode7", bitmapMode7);
  _rb_define_method(klass, "transform", bitmapTransform);
  _rb_define_method(klass, "raw_png_data", bitmapGetRawPNGData);

  rb_define_singleton_method(klass, "max_size", RUBY_METHOD_FUNC(bitmapGetMaxSize), -1);
    
    _rb_define_method(klass, "animated?", bitmapGetAnimated);
    _rb_define_method(klass, "playing", bitmapGetPlaying);
    _rb_define_method(klass, "playing=", bitmapSetPlaying);
    _rb_define_method(klass, "goto_and_stop", bitmapGotoStop);
    _rb_define_method(klass, "goto_and_play", bitmapGotoPlay);
    _rb_define_method(klass, "frame_count", bitmapFrames);
    _rb_define_method(klass, "current_frame", bitmapCurrentFrame);
    _rb_define_method(klass, "frame_rate", bitmapGetFPS);
    
    // For some reason Ruby says "screw this function in particular"
    _rb_define_method(klass, "frame_rate=", bitmapSetFPS);
    _rb_define_method(klass, "looping", bitmapGetLooping);
    _rb_define_method(klass, "looping=", bitmapSetLooping); 

  INIT_PROP_BIND(Bitmap, Font, "font");
}
