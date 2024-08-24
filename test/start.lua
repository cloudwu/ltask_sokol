local boot = require "ltask.bootstrap"

local function searchpath(name)
	return assert(package.searchpath(name, "ltask/lualib/?.lua"))
end

local function readall(path)
	local f <close> = assert(io.open(path))
	return f:read "a"
end

local M = {}

local wait_func

function M.start(config)
	-- set callback message handler
	local servicepath = searchpath "service"
	local root_config = {
		bootstrap = config.bootstrap,
		service_source = readall(servicepath),
		service_chunkname = "@" .. servicepath,
		initfunc = ([=[
local name = ...
package.path = [[${lua_path}]]
package.cpath = [[${lua_cpath}]]
local filename, err = package.searchpath(name, "${service_path}")
if not filename then
	return nil, err
end
return loadfile(filename)
]=]):gsub("%$%{([^}]*)%}", {
			lua_path = package.path,
			lua_cpath = package.cpath,
			service_path = config.service_path,
		}),
	}

	boot.init_socket()
	local bootstrap = dofile(searchpath "bootstrap")
	local core = config.core or {}
	core.external_queue = core.external_queue or 4096
	local ctx = bootstrap.start {
		core = core,
		root = root_config,
		root_initfunc = root_config.initfunc,
		mainthread = config.mainthread,
	}
	_G.external_messsage(bootstrap.external_sender(ctx))
	function wait_func()
		bootstrap.wait(ctx)
	end
	print "ltask Start"
end

function M.wait()
	wait_func()
end

return M