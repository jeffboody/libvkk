/*
 * Copyright (c) 2020 Jeff Boody
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_TAG "vkk"
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "../libcc/cc_timestamp.h"
#include "vkk_linux.h"
#include "vkk.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int keyPress(SDL_Keysym* keysym,
                    int* keycode, int* meta)
{
	ASSERT(keysym);
	ASSERT(keycode);
	ASSERT(meta);

	// convert the keycode
	*keycode = 0;
	*meta    = 0;
	if((keysym->sym >= SDLK_a) &&
	   (keysym->sym <= SDLK_z))
	{
		int a = (int) 'a';
		*keycode = a + (keysym->sym - SDLK_a);
	}
	else if((keysym->sym >= SDLK_0) &&
	        (keysym->sym <= SDLK_9))
	{
		int z = (int) '0';
		*keycode = z + (keysym->sym - SDLK_0);
	}
	else if((keysym->sym >= SDLK_KP_0) &&
	        (keysym->sym <= SDLK_KP_9))
	{
		int z = (int) '0';
		*keycode = z + (keysym->sym - SDLK_KP_0);
	}
	else if((keysym->sym == SDLK_RETURN) ||
	        (keysym->sym == SDLK_KP_ENTER))
	{
		*keycode = VKK_KEYCODE_ENTER;
	}
	else if(keysym->sym == SDLK_ESCAPE)
	{
		*keycode = VKK_KEYCODE_ESCAPE;
	}
	else if(keysym->sym == SDLK_BACKSPACE)
	{
		*keycode = VKK_KEYCODE_BACKSPACE;
	}
	else if(keysym->sym == SDLK_DELETE)
	{
		*keycode = VKK_KEYCODE_DELETE;
	}
	else if(keysym->sym == SDLK_UP)
	{
		*keycode = VKK_KEYCODE_UP;
	}
	else if(keysym->sym == SDLK_DOWN)
	{
		*keycode = VKK_KEYCODE_DOWN;
	}
	else if(keysym->sym == SDLK_LEFT)
	{
		*keycode = VKK_KEYCODE_LEFT;
	}
	else if(keysym->sym == SDLK_RIGHT)
	{
		*keycode = VKK_KEYCODE_RIGHT;
	}
	else if(keysym->sym == SDLK_HOME)
	{
		*keycode = VKK_KEYCODE_HOME;
	}
	else if(keysym->sym == SDLK_END)
	{
		*keycode = VKK_KEYCODE_END;
	}
	else if(keysym->sym == SDLK_PAGEUP)
	{
		*keycode = VKK_KEYCODE_PGUP;
	}
	else if(keysym->sym == SDLK_PAGEDOWN)
	{
		*keycode = VKK_KEYCODE_PGDOWN;
	}
	else if(keysym->sym == SDLK_INSERT)
	{
		*keycode = VKK_KEYCODE_INSERT;
	}
	else if(keysym->sym == SDLK_TAB)
	{
		*keycode = (int) '\t';
	}
	else if(keysym->sym == SDLK_SPACE)
	{
		*keycode = (int) ' ';
	}
	else if(keysym->sym == SDLK_EXCLAIM)
	{
		*keycode = (int) '!';
	}
	else if(keysym->sym == SDLK_QUOTEDBL)
	{
		*keycode = (int) '"';
	}
	else if(keysym->sym == SDLK_HASH)
	{
		*keycode = (int) '#';
	}
	else if(keysym->sym == SDLK_DOLLAR)
	{
		*keycode = (int) '$';
	}
	else if(keysym->sym == SDLK_AMPERSAND)
	{
		*keycode = (int) '&';
	}
	else if(keysym->sym == SDLK_QUOTE)
	{
		*keycode = (int) '\'';
	}
	else if(keysym->sym == SDLK_LEFTPAREN)
	{
		*keycode = (int) '(';
	}
	else if(keysym->sym == SDLK_RIGHTPAREN)
	{
		*keycode = (int) ')';
	}
	else if((keysym->sym == SDLK_ASTERISK) ||
	        (keysym->sym == SDLK_KP_MULTIPLY))
	{
		*keycode = (int) '*';
	}
	else if((keysym->sym == SDLK_PLUS) ||
	        (keysym->sym == SDLK_KP_PLUS))
	{
		*keycode = (int) '+';
	}
	else if(keysym->sym == SDLK_COMMA)
	{
		*keycode = (int) ',';
	}
	else if((keysym->sym == SDLK_MINUS) ||
	        (keysym->sym == SDLK_KP_MINUS))
	{
		*keycode = (int) '-';
	}
	else if((keysym->sym == SDLK_PERIOD) ||
	        (keysym->sym == SDLK_KP_PERIOD))
	{
		*keycode = (int) '.';
	}
	else if((keysym->sym == SDLK_SLASH) ||
	        (keysym->sym == SDLK_KP_DIVIDE))
	{
		*keycode = (int) '/';
	}
	else if(keysym->sym == SDLK_COLON)
	{
		*keycode = (int) ':';
	}
	else if(keysym->sym == SDLK_SEMICOLON)
	{
		*keycode = (int) ';';
	}
	else if(keysym->sym == SDLK_LESS)
	{
		*keycode = (int) '<';
	}
	else if((keysym->sym == SDLK_EQUALS) ||
	        (keysym->sym == SDLK_KP_EQUALS))
	{
		*keycode = (int) '=';
	}
	else if(keysym->sym == SDLK_GREATER)
	{
		*keycode = (int) '>';
	}
	else if(keysym->sym == SDLK_QUESTION)
	{
		*keycode = (int) '?';
	}
	else if(keysym->sym == SDLK_AT)
	{
		*keycode = (int) '@';
	}
	else if(keysym->sym == SDLK_LEFTBRACKET)
	{
		*keycode = (int) '[';
	}
	else if(keysym->sym == SDLK_BACKSLASH)
	{
		*keycode = (int) '\\';
	}
	else if(keysym->sym == SDLK_RIGHTBRACKET)
	{
		*keycode = (int) ']';
	}
	else if(keysym->sym == SDLK_CARET)
	{
		*keycode = (int) '^';
	}
	else if(keysym->sym == SDLK_UNDERSCORE)
	{
		*keycode = (int) '_';
	}
	else if(keysym->sym == SDLK_BACKQUOTE)
	{
		*keycode = (int) '`';
	}
	else
	{
		// ignore
		LOGD("sym=0x%X", keysym->sym);
		return 0;
	}

	// convert the meta
	if(keysym->mod & KMOD_ALT)
	{
		*meta |= VKK_META_ALT;
	}
	if(keysym->mod & KMOD_LALT)
	{
		*meta |= VKK_META_ALT_L;
	}
	if(keysym->mod & KMOD_RALT)
	{
		*meta |= VKK_META_ALT_R;
	}
	if(keysym->mod & KMOD_CTRL)
	{
		*meta |= VKK_META_CTRL;
	}
	if(keysym->mod & KMOD_LCTRL)
	{
		*meta |= VKK_META_CTRL_L;
	}
	if(keysym->mod & KMOD_RCTRL)
	{
		*meta |= VKK_META_CTRL_R;
	}
	if(keysym->mod & KMOD_SHIFT)
	{
		*meta |= VKK_META_SHIFT;
	}
	if(keysym->mod & KMOD_LSHIFT)
	{
		*meta |= VKK_META_SHIFT_L;
	}
	if(keysym->mod & KMOD_RSHIFT)
	{
		*meta |= VKK_META_SHIFT_R;
	}
	if(keysym->mod & KMOD_CAPS)
	{
		*meta |= VKK_META_CAPS;
	}

	return 1;
}

/***********************************************************
* platform                                                 *
***********************************************************/

static vkk_platform_t* vkk_platform_new(void)
{
	vkk_platformOnCreate_fn onCreate;
	vkk_platformOnEvent_fn  onEvent;
	onCreate = VKK_PLATFORM_CALLBACKS.onCreate;
	onEvent  = VKK_PLATFORM_CALLBACKS.onEvent;

	vkk_platform_t* self;
	self = (vkk_platform_t*) CALLOC(1, sizeof(vkk_platform_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->running = 1;

	// override the default screen density
	float density = 1.0f;
	FILE* f = fopen("sdl.cfg", "r");
	if(f)
	{
		int   w;
		int   h;
		int   fullscreen;
		if(fscanf(f, "%i %i %f %i",
		          &w, &h, &density, &fullscreen) != 4)
		{
			LOGW("fscanf failed");
		}
		fclose(f);
	}

	self->priv = (*onCreate)(self);
	if(self->priv == NULL)
	{
		goto fail_priv;
	}

	vkk_event_t ve =
	{
		.type    = VKK_EVENT_TYPE_DENSITY,
		.ts      = cc_timestamp(),
		.density = density
	};
	(*onEvent)(self->priv, &ve);

	// success
	return self;

	// failure
	fail_priv:
		FREE(self);
	return NULL;
}

static void vkk_platform_delete(vkk_platform_t** _self)
{
	vkk_platformOnDestroy_fn onDestroy;
	onDestroy = VKK_PLATFORM_CALLBACKS.onDestroy;

	vkk_platform_t* self = *_self;
	if(self)
	{
		(*onDestroy)(&self->priv);
		FREE(self);
		*_self = NULL;
	}
}

void vkk_platform_cmd(vkk_platform_t* self, int cmd,
                 const char* msg)
{
	// msg may be NULL
	ASSERT(self);

	if(cmd == VKK_PLATFORM_CMD_EXIT)
	{
		self->running = 0;
	}
}

/***********************************************************
* main                                                     *
***********************************************************/

int main(int argc, char** argv)
{
	vkk_platformOnDraw_fn  onDraw;
	vkk_platformOnEvent_fn onEvent;
	onDraw  = VKK_PLATFORM_CALLBACKS.onDraw;
	onEvent = VKK_PLATFORM_CALLBACKS.onEvent;

	vkk_platform_t* platform = vkk_platform_new();
	if(platform == NULL)
	{
		return EXIT_FAILURE;
	}

	while(platform->running)
	{
		// translate SDL events to VKK events
		SDL_Event se;
		while(SDL_PollEvent(&se))
		{
			if((se.type == SDL_KEYUP) || (se.type == SDL_KEYDOWN))
			{
				int keycode = 0;
				int meta    = 0;
				if(keyPress(&se.key.keysym, &keycode, &meta) == 0)
				{
					continue;
				}

				int type = VKK_EVENT_TYPE_KEY_UP;
				if(se.type == SDL_KEYDOWN)
				{
					type = VKK_EVENT_TYPE_KEY_DOWN;
				}
				vkk_event_t ve =
				{
					.type = type,
					.ts   = cc_timestamp(),
					.key  =
					{
						.keycode = keycode,
						.meta    = meta,
						.repeat  = se.key.repeat
					}
				};
				(*onEvent)(platform->priv, &ve);
			}
			else if((se.type == SDL_MOUSEBUTTONUP)   ||
			        (se.type == SDL_MOUSEBUTTONDOWN) ||
			        (se.type == SDL_MOUSEMOTION))
			{
				int type = VKK_EVENT_TYPE_ACTION_UP;
				if(se.type == SDL_MOUSEBUTTONDOWN)
				{
					type = VKK_EVENT_TYPE_ACTION_DOWN;
				}
				else if(se.type == SDL_MOUSEMOTION)
				{
					type = VKK_EVENT_TYPE_ACTION_MOVE;
				}
				vkk_event_t ve =
				{
					.type    = type,
					.ts      = cc_timestamp(),
					.action  =
					{
						.count = 1,
						.coord =
						{
							{
								.x = se.button.x,
								.y = se.button.y
							}
						}
					}
				};
				(*onEvent)(platform->priv, &ve);
			}
			else if((se.type == SDL_WINDOWEVENT) &&
			        (se.window.event == SDL_WINDOWEVENT_RESIZED))
			{
				vkk_event_t ve =
				{
					.type = VKK_EVENT_TYPE_RESIZE,
					.ts   = cc_timestamp()
				};
				if((*onEvent)(platform->priv, &ve) == 0)
				{
					platform->running = 0;
					break;
				}
			}
			else if(se.type == SDL_QUIT)
			{
				platform->running = 0;
				break;
			}
		}

		// draw event
		if(platform->running)
		{
			(*onDraw)(platform->priv);
		}
	}

	vkk_platform_delete(&platform);

	// success
	return EXIT_SUCCESS;
}
