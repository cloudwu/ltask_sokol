#define SOKOL_IMPL
#define SOKOL_D3D11

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
#include <stdio.h>

#include "sokol_app.h"
#include "sokol_log.h"
#include "sokol_args.h"
#include "message.h"

struct app_context {
	lua_State *L;
	void (*send_message)(void *ud, void *p);
	void *send_message_ud;
};

static struct app_context *CTX = NULL;

static void
send_app_message(void *p) {
	if (CTX && CTX->send_message) {
		CTX->send_message(CTX->send_message_ud, p);
	}
}

static int
set_callback(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	CTX->send_message = lua_touserdata(L, 1);
	CTX->send_message_ud = lua_touserdata(L, 2);
	return 0;
}

static int
pmain(lua_State *L) {
	luaL_openlibs(L);
	lua_pushcfunction(L, set_callback);
	lua_setglobal(L, "external_messsage");
	int n = sargs_num_args();
	luaL_checkstack(L, n+1, NULL);
	int i;
	int filename_index = -1;
	for (i=0;i<n;i++) {
		if (sargs_value_at(i)[0] == 0) {
			filename_index = i;
			break;
		}
	}
	if (filename_index < 0) {
		return luaL_error(L, "No filename");		
	}
	lua_newtable(L);
	int arg_table = lua_gettop(L);
	for (i=0;i<n;i++) {
		const char *k = sargs_key_at(i);
		const char *v = sargs_value_at(i);
		if (v[0] == 0) {
			lua_pushstring(L, sargs_key_at(i));
		} else {
			lua_pushstring(L, v);
			lua_setfield(L, arg_table, k);
		}
	}
	int arg_n = lua_gettop(L) - arg_table + 1;
	if (luaL_loadfile(L, sargs_key_at(filename_index)) != LUA_OK) {
		return lua_error(L);
	}
	lua_insert(L, -arg_n-1);
	if (lua_pcall(L, arg_n, 0, 0) != LUA_OK) {
		return lua_error(L);
	}
	
	return 0;
}

static void
start_app(lua_State *L) {
	lua_pushcfunction(L, pmain);
	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		fprintf(stderr, "Error : %s", lua_tostring(L, -1));
		lua_close(L);
		sapp_quit();
	}
}

static void
app_init() {
	int n = sargs_num_args();
	if (n < 1) {
		fprintf(stderr, "Need startup filename");
		sapp_quit();
		return;
	}
	static struct app_context app;
	lua_State *L = luaL_newstate();
	if (L == NULL)
		return;

	app.L = L;
	app.send_message = NULL;
	app.send_message_ud = NULL;
	
	CTX = &app;
	
	start_app(L);
	sargs_shutdown();
}

static void
app_frame() {
	send_app_message(message_create("frame", 0, 0));
}

static int
pcleanup(lua_State *L) {
	if (lua_getglobal(L, "cleanup") != LUA_TFUNCTION)
		return 0;
	lua_call(L, 0, 0);
	return 0;
}

static void
app_cleanup() {
	if (CTX == NULL)
		return;
	lua_State *L = CTX->L;
	send_app_message(message_create("cleanup", 0, 0));
	lua_pushcfunction(L, pcleanup);
	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		fprintf(stderr, "Error: %s", lua_tostring(L, -1));
	}
	lua_close(L);
	memset(CTX, 0, sizeof(*CTX));
}

static void
mouse_message(const sapp_event* ev) {
	const char *typestr = NULL;
	int p1 = 0;
	int p2 = 0;
	switch (ev->type) {
	case SAPP_EVENTTYPE_MOUSE_MOVE:
		typestr = "mouse_move";
		p1 = ev->mouse_x;
		p2 = ev->mouse_y;
		break;
	case SAPP_EVENTTYPE_MOUSE_DOWN:
	case SAPP_EVENTTYPE_MOUSE_UP:
		typestr = "mouse_button";
		p1 = ev->mouse_button;
		p2 = ev->type == SAPP_EVENTTYPE_MOUSE_DOWN;
		break;
	case SAPP_EVENTTYPE_MOUSE_SCROLL:
		typestr = "mouse_scroll";
		p1 = ev->scroll_y;
		p2 = ev->scroll_x;
		break;
	default:
		typestr = "mouse";
		p1 = ev->type;
		break;
	}
	send_app_message(message_create(typestr, p1, p2));
}


static void
app_event(const sapp_event* ev) {
	switch (ev->type) {
	case SAPP_EVENTTYPE_MOUSE_MOVE:
	case SAPP_EVENTTYPE_MOUSE_DOWN:
	case SAPP_EVENTTYPE_MOUSE_UP:
	case SAPP_EVENTTYPE_MOUSE_SCROLL:
	case SAPP_EVENTTYPE_MOUSE_ENTER:
	case SAPP_EVENTTYPE_MOUSE_LEAVE:
		mouse_message(ev);
		break;
	default:
		send_app_message(message_create("message", ev->type, 0));
		break;
	}
}

sapp_desc
sokol_main(int argc, char* argv[]) {
	sargs_desc arg_desc;
	memset(&arg_desc, 0, sizeof(arg_desc));
	arg_desc.argc = argc;
	arg_desc.argv = argv;
	sargs_setup(&arg_desc);
	
	sapp_desc d;
	memset(&d, 0, sizeof(d));
		
	d.width = 640;
	d.height = 480;
	d.init_cb = app_init;
	d.frame_cb = app_frame;
	d.cleanup_cb = app_cleanup;
	d.event_cb = app_event;
	d.logger.func = slog_func;
	d.win32_console_utf8 = 1;
	d.win32_console_attach = 1;

	return d;
}
