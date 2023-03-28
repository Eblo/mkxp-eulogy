/*
** plane.cpp
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

#include "plane.h"

#include "sharedstate.h"
#include "bitmap.h"
#include "etc.h"
#include "util.h"

#include "gl-util.h"
#include "quad.h"
#include "quadarray.h"
#include "transform.h"
#include "etc-internal.h"
#include "shader.h"
#include "glstate.h"

#include "sigslot/signal.hpp"
#include "binding-util.h"
#include "rb_shader.h"
#include "binding-types.h"

static float fwrap(float value, float range)
{
	float res = fmod(value, range);
	return res < 0 ? res + range : res;
}

struct PlanePrivate
{
	Bitmap *bitmap;

	NormValue opacity;
	BlendType blendType;
	Color *color;
	Tone *tone;

	int ox, oy;
	float zoomX, zoomY;

	Scene::Geometry sceneGeo;

	bool quadSourceDirty;

	SimpleQuadArray qArray;

	EtcTemps tmp;

	sigslot::connection prepareCon;

	VALUE shaderArr;

	PlanePrivate()
	    : bitmap(0),
	      opacity(255),
	      blendType(BlendNormal),
	      color(&tmp.color),
	      tone(&tmp.tone),
	      ox(0), oy(0),
	      zoomX(1), zoomY(1),
	      quadSourceDirty(false),
		  shaderArr(0)
	{
		prepareCon = shState->prepareDraw.connect
		        (&PlanePrivate::prepare, this);

		qArray.resize(1);
	}

	~PlanePrivate()
	{
		prepareCon.disconnect();
	}

	void updateQuadSource()
	{
		if (gl.npot_repeat)
		{
			FloatRect srcRect;
			srcRect.x = (sceneGeo.orig.x + ox) / zoomX;
			srcRect.y = (sceneGeo.orig.y + oy) / zoomY;
			srcRect.w = sceneGeo.rect.w / zoomX;
			srcRect.h = sceneGeo.rect.h / zoomY;

			Quad::setTexRect(&qArray.vertices[0], srcRect);
			qArray.commit();

			return;
		}

		if (nullOrDisposed(bitmap))
			return;

		/* Scaled (zoomed) bitmap dimensions */
		float sw = bitmap->width()  * zoomX;
		float sh = bitmap->height() * zoomY;

		/* Plane offset wrapped by scaled bitmap dims */
		float wox = fwrap(ox, sw);
		float woy = fwrap(oy, sh);

		/* Viewport dimensions */
		int vpw = sceneGeo.rect.w;
		int vph = sceneGeo.rect.h;

		/* Amount the scaled bitmap is tiled (repeated) */
		size_t tilesX = ceil((vpw - sw + wox) / sw) + 1;
		size_t tilesY = ceil((vph - sh + woy) / sh) + 1;

		FloatRect tex = bitmap->rect();

		qArray.resize(tilesX * tilesY);

		for (size_t y = 0; y < tilesY; ++y)
			for (size_t x = 0; x < tilesX; ++x)
			{
				SVertex *vert = &qArray.vertices[(y*tilesX + x) * 4];
				FloatRect pos(x*sw - wox, y*sh - woy, sw, sh);

				Quad::setTexPosRect(vert, tex, pos);
			}

		qArray.commit();
	}

	void prepare()
	{
		if (quadSourceDirty)
		{
			updateQuadSource();
			quadSourceDirty = false;
		}
	}

    CompiledShader* bindCustomShader(long i, int width, int height)
    {
        VALUE value = rb_ary_entry(shaderArr, i);
        CustomShader* shader = getPrivateDataCheck<CustomShader>(value, CustomShaderType);
        CompiledShader* compiled = shader->getShader();

        compiled->bind();
        compiled->applyViewportProj();

        shader->applyArgs();
        shader->setFloat("opacity", opacity.norm);
		shader->setVec4("color", color->norm);
		shader->setVec4("tone", tone->norm);
        shader->incrementPhase();
        shader->setTime();

        compiled->setTexSize(Vec2i(width, height));
        return compiled;
    }
};

Plane::Plane(Viewport *viewport)
    : ViewportElement(viewport)
{
	p = new PlanePrivate();

	onGeometryChange(scene->getGeometry());
}

DEF_ATTR_RD_SIMPLE(Plane, Bitmap,    Bitmap*, p->bitmap)
DEF_ATTR_RD_SIMPLE(Plane, OX,        int,     p->ox)
DEF_ATTR_RD_SIMPLE(Plane, OY,        int,     p->oy)
DEF_ATTR_RD_SIMPLE(Plane, ZoomX,     float,   p->zoomX)
DEF_ATTR_RD_SIMPLE(Plane, ZoomY,     float,   p->zoomY)
DEF_ATTR_RD_SIMPLE(Plane, BlendType, int,     p->blendType)

DEF_ATTR_SIMPLE(Plane, Opacity,   int,     p->opacity)
DEF_ATTR_SIMPLE(Plane, Color,     Color&, *p->color)
DEF_ATTR_SIMPLE(Plane, Tone,      Tone&,  *p->tone)
DEF_ATTR_SIMPLE(Plane, ShaderArr, VALUE, p->shaderArr)

Plane::~Plane()
{
	dispose();
}

void Plane::setBitmap(Bitmap *value)
{
	guardDisposed();

	p->bitmap = value;

	if (!value)
		return;

	value->ensureNonMega();
}

void Plane::setOX(int value)
{
	guardDisposed();

	if (p->ox == value)
	        return;

	p->ox = value;
	p->quadSourceDirty = true;
}

void Plane::setOY(int value)
{
	guardDisposed();

	if (p->oy == value)
	        return;

	p->oy = value;
	p->quadSourceDirty = true;
}

void Plane::setZoomX(float value)
{
	guardDisposed();

	if (p->zoomX == value)
	        return;

	p->zoomX = value;
	p->quadSourceDirty = true;
}

void Plane::setZoomY(float value)
{
	guardDisposed();

	if (p->zoomY == value)
	        return;

	p->zoomY = value;
	p->quadSourceDirty = true;
}

void Plane::setBlendType(int value)
{
	guardDisposed();

	switch (value)
	{
	default :
	case BlendNormal :
		p->blendType = BlendNormal;
		return;
	case BlendAddition :
		p->blendType = BlendAddition;
		return;
	case BlendSubstraction :
		p->blendType = BlendSubstraction;
		return;
	}
}

void Plane::initDynAttribs()
{
	p->color = new Color;
	p->tone = new Tone;
}

void Plane::draw()
{
	if (nullOrDisposed(p->bitmap))
		return;

	if (!p->opacity)
		return;

	ShaderBase *base;

	if (p->color->hasEffect() || p->tone->hasEffect() || p->opacity != 255)
	{
		PlaneShader &shader = shState->shaders().plane;

		shader.bind();
		shader.applyViewportProj();
		shader.setTone(p->tone->norm);
		shader.setColor(p->color->norm);
		shader.setFlash(Vec4());
		shader.setOpacity(p->opacity.norm);

		base = &shader;
	}
	else
	{
		SimpleShader &shader = shState->shaders().simple;

		shader.bind();
		shader.applyViewportProj();
		shader.setTranslation(Vec2i());

		base = &shader;
	}

	glState.blendMode.pushSet(p->blendType);
	p->bitmap->bindTex(*base);

	long size = rb_array_len(p->shaderArr);
	if (size > 0) {
        // Store the current FBO used, as FBO::unbind() will set it to 0 which is not correct
        GLint originalFbo = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &originalFbo);

        // Get the general purpose quad and set it to the bitmap's dimensions for shader stacking
        // This is needed to ensure that the shaders apply to the bitmap's size and position in isolation
        Quad &quad = shState->gpQuad();
        int width = p->bitmap->width(), height = p->bitmap->height();
        FloatRect rect(0, 0, width, height);
        quad.setTexPosRect(rect, rect);        
        glState.blend.pushSet(false);
        glState.viewport.pushSet(IntRect(0, 0, width, height));
        
        // Bind the bitmap's frontBuffer FBO and use the temporary viewport to render the sprite in isolation. This
		// will apply the plane's main shader first as the base shader.
        FBO::bind(p->bitmap->frontBuffer().fbo);
        base->applyViewportProj();
        
        CompiledShader *customShader;
		for (long i = 0; i < size; i++) {
            // Using the currently bound shader, draw using the general purpose quad. This writes to the frontBuffer
            quad.draw();
            // Now, the frontBuffer TBO contains the output of the above draw call. Swap frontBuffer and backBuffer,
            // binding the resulting output TBO as the next input TBO.
            p->bitmap->pingpongBind();
            // Bind the custom shader and assign it, as the final one must be drawn differently
            customShader = p->bindCustomShader(i, width, height);
		}

        // Restore the original viewport and blend
        glState.viewport.pop();
        glState.blend.pop();
        customShader->applyViewportProj();

        // Restore the original FBO for the final draw
        gl.BindFramebuffer(GL_FRAMEBUFFER, originalFbo);
	}

	if (gl.npot_repeat)
		TEX::setRepeat(true);

	p->qArray.draw();

	if (gl.npot_repeat)
		TEX::setRepeat(false);

	glState.blendMode.pop();
}

void Plane::onGeometryChange(const Scene::Geometry &geo)
{
	if (gl.npot_repeat)
		Quad::setPosRect(&p->qArray.vertices[0], FloatRect(geo.rect));

	p->sceneGeo = geo;
	p->quadSourceDirty = true;
}

void Plane::releaseResources()
{
	unlink();

	delete p;
}
