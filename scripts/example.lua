client.log("example loaded")

function on_paint()
    local width, height = client.screen_size()
    render.text(20, height / 2, "Lua is running", 144, 145, 185, 255)
    render.line(20, height / 2 + 18, 180, height / 2 + 18, 144, 145, 185, 255, 1)
end
