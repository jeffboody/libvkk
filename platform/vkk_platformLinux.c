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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "../../libcc/cc_timestamp.h"
#include "../../libbfs/bfs_util.h"
#include "../core/vkk_engine.h"
#include "../vkk_platform.h"
#include "../vkk.h"
#include "vkk_platformLinux.h"

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
		*keycode = VKK_PLATFORM_KEYCODE_ENTER;
	}
	else if(keysym->sym == SDLK_ESCAPE)
	{
		*keycode = VKK_PLATFORM_KEYCODE_ESCAPE;
	}
	else if(keysym->sym == SDLK_BACKSPACE)
	{
		*keycode = VKK_PLATFORM_KEYCODE_BACKSPACE;
	}
	else if(keysym->sym == SDLK_DELETE)
	{
		*keycode = VKK_PLATFORM_KEYCODE_DELETE;
	}
	else if(keysym->sym == SDLK_UP)
	{
		*keycode = VKK_PLATFORM_KEYCODE_UP;
	}
	else if(keysym->sym == SDLK_DOWN)
	{
		*keycode = VKK_PLATFORM_KEYCODE_DOWN;
	}
	else if(keysym->sym == SDLK_LEFT)
	{
		*keycode = VKK_PLATFORM_KEYCODE_LEFT;
	}
	else if(keysym->sym == SDLK_RIGHT)
	{
		*keycode = VKK_PLATFORM_KEYCODE_RIGHT;
	}
	else if(keysym->sym == SDLK_HOME)
	{
		*keycode = VKK_PLATFORM_KEYCODE_HOME;
	}
	else if(keysym->sym == SDLK_END)
	{
		*keycode = VKK_PLATFORM_KEYCODE_END;
	}
	else if(keysym->sym == SDLK_PAGEUP)
	{
		*keycode = VKK_PLATFORM_KEYCODE_PGUP;
	}
	else if(keysym->sym == SDLK_PAGEDOWN)
	{
		*keycode = VKK_PLATFORM_KEYCODE_PGDOWN;
	}
	else if(keysym->sym == SDLK_INSERT)
	{
		*keycode = VKK_PLATFORM_KEYCODE_INSERT;
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
		*meta |= VKK_PLATFORM_META_ALT;
	}
	if(keysym->mod & KMOD_LALT)
	{
		*meta |= VKK_PLATFORM_META_ALT_L;
	}
	if(keysym->mod & KMOD_RALT)
	{
		*meta |= VKK_PLATFORM_META_ALT_R;
	}
	if(keysym->mod & KMOD_CTRL)
	{
		*meta |= VKK_PLATFORM_META_CTRL;
	}
	if(keysym->mod & KMOD_LCTRL)
	{
		*meta |= VKK_PLATFORM_META_CTRL_L;
	}
	if(keysym->mod & KMOD_RCTRL)
	{
		*meta |= VKK_PLATFORM_META_CTRL_R;
	}
	if(keysym->mod & KMOD_SHIFT)
	{
		*meta |= VKK_PLATFORM_META_SHIFT;
	}
	if(keysym->mod & KMOD_LSHIFT)
	{
		*meta |= VKK_PLATFORM_META_SHIFT_L;
	}
	if(keysym->mod & KMOD_RSHIFT)
	{
		*meta |= VKK_PLATFORM_META_SHIFT_R;
	}
	if(keysym->mod & KMOD_CAPS)
	{
		*meta |= VKK_PLATFORM_META_CAPS;
	}

	return 1;
}

typedef struct
{
	vkk_platform_t* platform;

	vkk_platformEventType_e type;
	double                  ts;
	int                     count;
	int                     id[4];
	float                   x[4];
	float                   y[4];
} vkk_platformTouch_t;

static void
vkk_platformTouch_init(vkk_platformTouch_t* self,
                       vkk_platform_t* platform)
{
	ASSERT(self);
	ASSERT(platform);

	self->platform = platform;
	self->type     = VKK_PLATFORM_EVENTTYPE_UNDEFINED;
	self->count    = 0;
}

static void
vkk_platformTouch_event(vkk_platformTouch_t* self)
{
	ASSERT(self);

	vkk_platformOnEvent_fn onEvent;
	onEvent = VKK_PLATFORM_INFO.onEvent;

	if(self->type == VKK_PLATFORM_EVENTTYPE_UNDEFINED)
	{
		return;
	}

	vkk_platformEvent_t ve =
	{
		.type    = self->type,
		.ts      = self->ts,
		.action  =
		{
			.count = self->count,
			.coord =
			{
				{
					.x = self->x[0],
					.y = self->y[0]
				},
				{
					.x = self->x[1],
					.y = self->y[1]
				},
				{
					.x = self->x[2],
					.y = self->y[2]
				},
				{
					.x = self->x[3],
					.y = self->y[3]
				}
			}
		}
	};
	(*onEvent)(self->platform->priv, &ve);

	// reset touch state
	if(self->type == VKK_PLATFORM_EVENTTYPE_ACTION_UP)
	{
		self->count = 0;
	}
	self->type = VKK_PLATFORM_EVENTTYPE_UNDEFINED;
}

static void
vkk_platformTouch_action(vkk_platformTouch_t* self,
                         vkk_platformEventType_e type,
                         double ts,
                         int id,
                         float x, float y)
{
	ASSERT(self);

	// init action
	if((self->type == VKK_PLATFORM_EVENTTYPE_UNDEFINED) ||
	   (type == VKK_PLATFORM_EVENTTYPE_ACTION_UP))
	{
		// ignore invalid actions
		if((self->count == 0) &&
		   (type != VKK_PLATFORM_EVENTTYPE_ACTION_DOWN))
		{
			return;
		}

		self->type = type;
	}

	// update event
	int i;
	for(i = 0; i < self->count; ++i)
	{
		if(id == self->id[i])
		{
			self->x[i] = x;
			self->y[i] = y;
			vkk_platformTouch_event(self);
			return;
		}
	}

	// too many fingers
	if(self->count >= 4)
	{
		return;
	}

	// add the event
	int count = self->count;
	self->ts        = ts;
	self->id[count] = id;
	self->x[count]  = x;
	self->y[count]  = y;
	++self->count;
	vkk_platformTouch_event(self);
}

/***********************************************************
* platform                                                 *
***********************************************************/

static vkk_platform_t* vkk_platform_new(void)
{
	vkk_platformOnCreate_fn  onCreate;
	vkk_platformOnDestroy_fn onDestroy;
	vkk_platformOnPause_fn   onPause;
	vkk_platformOnDraw_fn    onDraw;
	vkk_platformOnEvent_fn   onEvent;
	vkk_platformOnMain_fn    onMain;
	onCreate  = VKK_PLATFORM_INFO.onCreate;
	onDestroy = VKK_PLATFORM_INFO.onDestroy;
	onPause   = VKK_PLATFORM_INFO.onPause;
	onDraw    = VKK_PLATFORM_INFO.onDraw;
	onEvent   = VKK_PLATFORM_INFO.onEvent;
	onMain    = VKK_PLATFORM_INFO.onMain;

	if(onMain)
	{
		// disallow all other callbacks when onMain is defined
		if(onCreate || onDestroy || onPause ||
		   onDraw   || onEvent)
		{
			LOGE("invalid");
			return NULL;
		}
	}

	// initialize home
	char home[256];
	const char* tmp = getenv("HOME");
	if(tmp)
	{
		snprintf(home, 256, "%s", tmp);
	}
	else
	{
		struct passwd  pwd;
		struct passwd* result = NULL;
		char           buf[4096];
		int ret = getpwuid_r(getuid(), &pwd,
		                     buf, 4096,
		                     &result);
		if((ret != 0) || (result == NULL))
		{
			LOGE("invalid home");
			return NULL;
		}

		snprintf(home, 256, "%s", result->pw_dir);
	}

	vkk_platform_t* self;
	self = (vkk_platform_t*) CALLOC(1, sizeof(vkk_platform_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->running     = 1;
	self->paused      = 1;
	self->document_fd = -1;

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
		else
		{
			self->width  = (float) w;
			self->height = (float) h;
		}
		fclose(f);
	}

	self->engine = vkk_engine_new(self,
	                              VKK_PLATFORM_INFO.app_name,
	                              VKK_PLATFORM_INFO.app_dir,
	                              &VKK_PLATFORM_INFO.app_version,
	                              home, home);
	if(self->engine == NULL)
	{
		goto fail_engine;
	}

	if(onCreate)
	{
		self->priv = (*onCreate)(self->engine);
		if(self->priv == NULL)
		{
			goto fail_priv;
		}
	}

	if(onEvent)
	{
		vkk_platformEvent_t ve =
		{
			.type    = VKK_PLATFORM_EVENTTYPE_DENSITY,
			.ts      = cc_timestamp(),
			.density = density
		};
		(*onEvent)(self->priv, &ve);
	}

	// success
	return self;

	// failure
	fail_priv:
		vkk_engine_delete(&self->engine);
	fail_engine:
		FREE(self);
	return NULL;
}

static void vkk_platform_delete(vkk_platform_t** _self)
{
	vkk_platformOnDestroy_fn onDestroy;
	vkk_platformOnPause_fn   onPause;
	onDestroy = VKK_PLATFORM_INFO.onDestroy;
	onPause   = VKK_PLATFORM_INFO.onPause;

	vkk_platform_t* self = *_self;
	if(self)
	{
		if(onPause && (self->paused == 0))
		{
			(*onPause)(self->priv);
			self->paused = 1;
		}

		if(self->joy)
		{
			SDL_JoystickClose(self->joy);
		}

		vkk_engine_shutdown(self->engine);
		if(onDestroy)
		{
			(*onDestroy)(&self->priv);
		}
		vkk_engine_delete(&self->engine);
		FREE(self);
		*_self = NULL;
	}
}

void vkk_platform_cmd(vkk_platform_t* self,
                      vkk_platformCmdInfo_t* info)
{
	ASSERT(self);
	ASSERT(info);

	if(info->cmd == VKK_PLATFORM_CMD_EXIT)
	{
		self->running = 0;
	}
	else if((info->cmd == VKK_PLATFORM_CMD_DOCUMENT_CREATE) ||
	        (info->cmd == VKK_PLATFORM_CMD_DOCUMENT_OPEN))
	{
		int fd = self->document_fd;
		if(fd >= 0)
		{
			LOGE("invalid fd=%i", fd);
			return;
		}
		else if(info->cmd == VKK_PLATFORM_CMD_DOCUMENT_CREATE)
		{
			fd = open(info->msg,
			          O_RDWR | O_CREAT | O_TRUNC,
			          S_IRUSR | S_IWUSR);
		}
		else if(info->cmd == VKK_PLATFORM_CMD_DOCUMENT_OPEN)
		{
			fd = open(info->msg, O_RDWR);
		}

		if(fd < 0)
		{
			LOGE("open failed");
			return;
		}

		self->document_ready = 1;
		self->document_fd    = fd;
		self->document_priv  = info->priv;
		self->document_fn    = info->document_fn;
		snprintf(self->document_uri, 256, "%s", info->msg);
	}
	else if(info->cmd == VKK_PLATFORM_CMD_DOCUMENT_NAME)
	{
		self->document_ready = 1;
		self->document_fd    = -1;
		self->document_priv  = info->priv;
		self->document_fn    = info->document_fn;
		snprintf(self->document_uri, 256, "%s", info->msg);
	}
	else if(info->cmd == VKK_PLATFORM_CMD_LOADURL)
	{
		LOGI("url=%s", info->msg);
	}
}

/***********************************************************
* main                                                     *
***********************************************************/

int main(int argc, char** argv)
{
	vkk_platformOnDraw_fn  onDraw;
	vkk_platformOnEvent_fn onEvent;
	vkk_platformOnMain_fn  onMain;
	onDraw  = VKK_PLATFORM_INFO.onDraw;
	onEvent = VKK_PLATFORM_INFO.onEvent;
	onMain  = VKK_PLATFORM_INFO.onMain;

	if(bfs_util_initialize() == 0)
	{
		return EXIT_FAILURE;
	}

	vkk_platform_t* platform = vkk_platform_new();
	if(platform == NULL)
	{
		goto fail_platform;
	}

	vkk_platformButton_e button_map[] =
	{
		VKK_PLATFORM_BUTTON_A,
		VKK_PLATFORM_BUTTON_B,
		VKK_PLATFORM_BUTTON_X,
		VKK_PLATFORM_BUTTON_Y,
		VKK_PLATFORM_BUTTON_L1,
		VKK_PLATFORM_BUTTON_R1,
	};

	int button_count = (int) (sizeof(button_map)/
	                          sizeof(vkk_platformButton_e));

	int status = EXIT_SUCCESS;
	vkk_platformTouch_t t;
	vkk_platformTouch_init(&t, platform);
	if(onMain)
	{
		status = (*onMain)(platform->engine, argc, argv);
	}
	else
	{
		while(platform->running)
		{
			// process document event
			if(platform->document_ready)
			{
				(platform->document_fn)(platform->document_priv,
				                        platform->document_uri,
				                        &platform->document_fd);

				// reset document event
				platform->document_ready = 0;
				if(platform->document_fd >= 0)
				{
					close(platform->document_fd);
					platform->document_fd = -1;
				}
				platform->document_priv = NULL;
				platform->document_fn   = NULL;
			}

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

					vkk_platformEventType_e type = VKK_PLATFORM_EVENTTYPE_KEY_UP;
					if(se.type == SDL_KEYDOWN)
					{
						type = VKK_PLATFORM_EVENTTYPE_KEY_DOWN;
					}

					double ms = ((double) se.key.timestamp);
					double ts = ms/1000.0;
					vkk_platformEvent_t ve =
					{
						.type = type,
						.ts   = ts,
						.key  =
						{
							.keycode = keycode,
							.meta    = meta,
							.repeat  = se.key.repeat
						}
					};

					int pressed = (*onEvent)(platform->priv, &ve);
					if((type    == VKK_PLATFORM_EVENTTYPE_KEY_UP) &&
					   (keycode == VKK_PLATFORM_KEYCODE_ESCAPE)   &&
					   (pressed == 0))
					{
						// double tap back to exit
						if((ts - platform->escape_t0) < 0.5)
						{
							vkk_platformCmdInfo_t info =
							{
								.cmd = VKK_PLATFORM_CMD_EXIT,
							};
							vkk_platform_cmd(platform, &info);
						}
						else
						{
							platform->escape_t0 = ts;
						}
					}
				}
				else if((se.type == SDL_MOUSEBUTTONUP)   ||
				        (se.type == SDL_MOUSEBUTTONDOWN) ||
				        (se.type == SDL_MOUSEMOTION))
				{
					vkk_platformEventType_e type = VKK_PLATFORM_EVENTTYPE_ACTION_UP;
					if(se.type == SDL_MOUSEBUTTONDOWN)
					{
						type = VKK_PLATFORM_EVENTTYPE_ACTION_DOWN;
					}
					else if(se.type == SDL_MOUSEMOTION)
					{
						type = VKK_PLATFORM_EVENTTYPE_ACTION_MOVE;
					}

					double ms = (double) se.button.timestamp;
					double ts = ms/1000.0;
					vkk_platformEvent_t ve =
					{
						.type    = type,
						.ts      = ts,
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
				else if((se.type == SDL_FINGERUP)   ||
				        (se.type == SDL_FINGERDOWN) ||
				        (se.type == SDL_FINGERMOTION))
				{
					vkk_platformEventType_e type = VKK_PLATFORM_EVENTTYPE_ACTION_UP;
					if(se.type == SDL_FINGERDOWN)
					{
						type = VKK_PLATFORM_EVENTTYPE_ACTION_DOWN;
					}
					else if(se.type == SDL_FINGERMOTION)
					{
						type = VKK_PLATFORM_EVENTTYPE_ACTION_MOVE;
					}

					double ms = (double) se.tfinger.timestamp;
					double ts = ms/1000.0;
					vkk_platformTouch_action(&t, type, ts,
					                         se.tfinger.fingerId,
					                         platform->width*se.tfinger.x,
					                         platform->height*se.tfinger.y);
				}
				else if(se.type == SDL_JOYAXISMOTION)
				{
					LOGD("SDL_JOYAXISMOTION which=%u, axis=%u, value=%i",
					     (uint32_t) se.jaxis.which,
					     (uint32_t) se.jaxis.axis,
					     (int) se.jaxis.value);

					int ignore = 0;

					int    id = se.jaxis.which;
					double ms = (double) se.jaxis.timestamp;
					double ts = ms/1000.0;
					vkk_platformEvent_t ve =
					{
						.type = VKK_PLATFORM_EVENTTYPE_AXIS_MOVE,
						.ts   = ts,
						.axis =
						{
							.id = id
						}
					};

					if(id != platform->joy_id)
					{
						// unknown id
						ignore = 1;
					}

					// scale axis value from -1.0f to 1.0f
					ve.axis.value = (float) se.jaxis.value;
					if(ve.axis.value < 0.0f)
					{
						ve.axis.value /= 32768.0f;
					}
					else
					{
						ve.axis.value /= 32767.0f;
					}

					// assign axis using ipega joystick layout
					// note that the trigger resting position is -32768
					uint32_t axis = se.jaxis.axis;
					if(axis == 0)
					{
						ve.axis.axis = VKK_PLATFORM_AXIS_X1;
					}
					else if(axis == 1)
					{
						ve.axis.axis = VKK_PLATFORM_AXIS_Y1;
					}
					else if(axis == 3)
					{
						ve.axis.axis = VKK_PLATFORM_AXIS_X2;
					}
					else if(axis == 4)
					{
						ve.axis.axis = VKK_PLATFORM_AXIS_Y2;
					}
					else if(axis == 5)
					{
						ve.axis.axis  = VKK_PLATFORM_AXIS_RT;
						ve.axis.value = (ve.axis.value + 1.0f)/2.0f;
					}
					else if(axis == 2)
					{
						ve.axis.axis  = VKK_PLATFORM_AXIS_LT;
						ve.axis.value = (ve.axis.value + 1.0f)/2.0f;
					}
					else
					{
						// unknown axis
						ignore = 1;
					}

					if(ignore == 0)
					{
						(*onEvent)(platform->priv, &ve);
					}
				}
				else if(se.type == SDL_JOYBALLMOTION)
				{
					LOGD("SDL_JOYBALLMOTION which=%u, ball=%u, xrel=%i, yrel=%i",
					     (uint32_t) se.jball.which,
					     (uint32_t) se.jball.ball,
					     (int) se.jball.xrel,
					     (int) se.jball.yrel);
				}
				else if(se.type == SDL_JOYHATMOTION)
				{
					LOGD("SDL_JOYHATMOTION which=%u, hat=%u, value=%u",
					     (uint32_t) se.jhat.which,
					     (uint32_t) se.jhat.hat,
					     (uint32_t) se.jhat.value);
				}
				else if(se.type == SDL_JOYBUTTONDOWN)
				{
					LOGD("SDL_JOYBUTTONDOWN which=%u, button=%u, state=%u",
					     (uint32_t) se.jbutton.which,
					     (uint32_t) se.jbutton.button,
					     (uint32_t) se.jbutton.state);

					double ms = (double) se.jbutton.timestamp;
					double ts = ms/1000.0;
					vkk_platformEvent_t ve =
					{
						.type   = VKK_PLATFORM_EVENTTYPE_BUTTON_DOWN,
						.ts     = ts,
						.button =
						{
							.id = se.jbutton.which
						}
					};

					if(se.jbutton.button < button_count)
					{
						ve.button.button = button_map[se.jbutton.button];
						(*onEvent)(platform->priv, &ve);
					}
				}
				else if(se.type == SDL_JOYBUTTONUP)
				{
					LOGD("SDL_JOYBUTTONUP which=%u, button=%u, state=%u",
					     (uint32_t) se.jbutton.which,
					     (uint32_t) se.jbutton.button,
					     (uint32_t) se.jbutton.state);

					double ms = (double) se.jbutton.timestamp;
					double ts = ms/1000.0;
					vkk_platformEvent_t ve =
					{
						.type   = VKK_PLATFORM_EVENTTYPE_BUTTON_UP,
						.ts     = ts,
						.button =
						{
							.id = se.jbutton.which
						}
					};

					if(se.jbutton.button < button_count)
					{
						ve.button.button = button_map[se.jbutton.button];
						(*onEvent)(platform->priv, &ve);
					}
				}
				else if(se.type == SDL_JOYDEVICEADDED)
				{
					LOGD("SDL_JOYDEVICEADDED which=%u",
					     (uint32_t) se.jdevice.which);

					if(platform->joy == NULL)
					{
						platform->joy = SDL_JoystickOpen(se.jdevice.which);
						if(platform->joy)
						{
							platform->joy_id = se.jdevice.which;
						}
					}
				}
				else if(se.type == SDL_JOYDEVICEREMOVED)
				{
					LOGD("SDL_JOYDEVICEREMOVED which=%u",
					     (uint32_t) se.jdevice.which);

					if(platform->joy &&
					   (se.jdevice.which == platform->joy_id))
					{
						SDL_JoystickClose(platform->joy);
						platform->joy = NULL;
					}
				}
				else if((se.type == SDL_WINDOWEVENT) &&
				        (se.window.event == SDL_WINDOWEVENT_RESIZED))
				{
					platform->width  = se.window.data1;
					platform->height = se.window.data2;
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
				platform->paused = 0;
			}
		}
	}

	vkk_platform_delete(&platform);

	size_t count = MEMCOUNT();
	size_t size  = MEMSIZE();
	if(count || size)
	{
		LOGW("memory leak detected");
		MEMINFO();
	}

	bfs_util_shutdown();

	// success
	return status;

	// failure
	fail_platform:
		bfs_util_shutdown();
	return EXIT_FAILURE;
}
