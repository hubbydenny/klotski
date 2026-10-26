# Lua API

Place `.lua` files in `Documents/klotski/scripts`. The LUA tab lists scripts
and supports refresh, load, reload and unload. Scripts are not auto-loaded.
Copy `example.lua` into that directory to try the API.

Each script owns a Lua state. `on_paint()` runs once per rendered frame.
Errors unload only the failing script and appear in the LUA tab and console.
Rendering functions are available only inside `on_paint`.

```lua
client.log("message")
local width, height = client.screen_size()
local seconds = client.time()
local frame_seconds = client.frame_time()
local connected = client.is_in_game()

function on_paint()
    render.text(20, 100, "Hello", 144, 145, 185, 255)
    render.line(20, 120, 200, 120, 144, 145, 185, 255, 1)
    render.rect(20, 140, 180, 30, 144, 145, 185, 200, true)
    render.circle(240, 150, 15, 144, 145, 185, 255, false)
    local text_width, text_height = render.text_size("Hello")
end
```

Colors use integers from 0 to 255; omitted color channels default to 255.
Line thickness defaults to 1 and must be greater than 0 and at most 16.
The runtime opens base, math, string and table libraries. It does not open
io, os, debug, package or coroutine. `dofile`, `loadfile`, `pcall` and `xpcall`
are unavailable. Each callback has an instruction limit and at most 2048 draw
calls. These limits are guardrails, not isolation for untrusted scripts.
