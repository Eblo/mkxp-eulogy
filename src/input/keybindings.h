/*
** keybindings.h
**
** This file is part of mkxp.
**
** Copyright (C) 2014 Jonas Kulla <Nyocurio@gmail.com>
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

#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include "input.h"

#include <SDL_scancode.h>
#include <SDL_keyboard.h>
#include <SDL_joystick.h>
#include <stdint.h>
#include <assert.h>
#include <vector>

enum AxisDir
{
	Negative,
	Positive
};

enum SourceType
{
	Invalid,
	Key,
	JButton,
	JAxis,
	JHat
};

struct SourceDesc
{
	SourceType type;

	union Data
	{
		/* Keyboard scancode */
		SDL_Scancode scan;
		/* Joystick button index */
		uint8_t jb;
		struct
		{
			/* Joystick axis index */
			uint8_t axis;
			/* Joystick axis direction */
			AxisDir dir;
		} ja;
		struct
		{
			/* Joystick axis index */
			uint8_t hat;
			/* Joystick axis direction */
			uint8_t pos;
		} jh;
	} d;

	bool operator==(const SourceDesc &o) const
	{
		if (type != o.type)
			return false;

		switch (type)
		{
		case Invalid:
			return true;
		case Key:
			return d.scan == o.d.scan;
		case JButton:
			return d.jb == o.d.jb;
		case JAxis:
			return (d.ja.axis == o.d.ja.axis) && (d.ja.dir == o.d.ja.dir);
		case JHat:
			return (d.jh.hat == o.d.jh.hat) && (d.jh.pos == o.d.jh.pos);
		default:
			assert(!"unreachable");
			return false;
		}
	}

	bool operator!=(const SourceDesc &o) const
	{
		return !(*this == o);
	}

	/* Human readable string representation */
	std::string sourceDescString() const
	{
		char buf[128];
		char pos;

		switch (type)
		{
		case Invalid:
			return std::string();

		case Key:
		{
			if (d.scan == SDL_SCANCODE_LSHIFT)
				return "Shift";

			SDL_Keycode key = SDL_GetKeyFromScancode(d.scan);
			const char *str = SDL_GetKeyName(key);

			if (*str == '\0')
				return "Unknown key";
			else
				return str;
		}
		case JButton:
			snprintf(buf, sizeof(buf), "JS %d", d.jb);
			return buf;

		case JHat:
			switch(d.jh.pos)
			{
			case SDL_HAT_UP:
				pos = 'U';
				break;

			case SDL_HAT_DOWN:
				pos = 'D';
				break;

			case SDL_HAT_LEFT:
				pos = 'L';
				break;

			case SDL_HAT_RIGHT:
				pos = 'R';
				break;

			default:
				pos = '-';
			}
			snprintf(buf, sizeof(buf), "Hat %d:%c",
					d.jh.hat, pos);
			return buf;

		case JAxis:
			snprintf(buf, sizeof(buf), "Axis %d%c",
					d.ja.axis, d.ja.dir == Negative ? '-' : '+');
			return buf;
		}

		assert(!"unreachable");
		return "";
	}

};

#define JAXIS_THRESHOLD 0x4000

struct BindingDesc
{
	SourceDesc src;
	Input::ButtonCode target;
};

typedef std::vector<BindingDesc> BDescVec;
struct Config;

BDescVec genDefaultBindings(const Config &conf);

void storeBindings(const BDescVec &d, const Config &conf);
BDescVec loadBindings(const Config &conf);

#endif // KEYBINDINGS_H
