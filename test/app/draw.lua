
local gui = vm.import("test/app/gui.lua")

local function square(colors)
    local ret = {}
    ret.n = 0
    local colors = {
        gui.Rectangle.rgb(100, 100, 100),
        gui.Rectangle.rgb(120, 120, 120),
    }
    local function update()
        ret.n = ret.n + 1
        ret.color = colors[ret.n % 2 + 1]
    end 
    ret.button = gui.Button.left(update)
    update()
    return ret
end

local s = 4

local xs = 16 * s
local ys = 9 * s
draw = gui.Grid.new(xs, ys)

for i=0, xs * ys - 1 do
    local x = i % xs + 1
    local y = i // xs + 1
    draw[x][y] = square() 
end

app()
