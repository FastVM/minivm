
local gui = vm.import("test/app/gui.lua")

local scale = 20

local xsize = 1920 // scale
local ysize = 1080 // scale

local grid = gui.Grid.new(xsize, ysize)
local window = gui.Window.new(1920 // 2, 1080 // 2)

local select = {x = 0, y = 0}

local function cell(x, y)
    local env = {}

    local function on_click()
        env.draw = gui.Rectangle.WHITE
    end

    env.draw = gui.Rectangle.BLACK

    env.click = gui.Drag.left(on_click)

    return env
end

local function on_init()
    for y=1, ysize do
        for x=1, xsize do
            grid[x][y] = cell(x, y)
        end
    end
end

draw = {
    grid,
    window,
}

on_init()

app()
