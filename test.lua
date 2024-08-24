package.cpath = "ltask/?.dll;./?.dll"
package.path = "?.lua;ltask/lualib/?.lua"

local start = require "test.start"

function cleanup()
	start.wait()
end

start.start {
    core = {
        debuglog = "=", -- stdout
    },
    service_path = "ltask/service/?.lua;test/?.lua",
    bootstrap = {
        {
            name = "timer",
            unique = true,
        },
        {
            name = "logger",
            unique = true,
        },
        {
            name = "bootstrap",
        },
    },
}
