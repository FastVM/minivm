
local gui = vm.import("test/app/gui.lua")

local debug = false

local s = 20

local xs = 2 * s + 3
local ys = 2 * s + 3

local grid = gui.Grid.new(xs, ys)

local dir = {
    root = 0
    nx = 1,
    px = 2,
    ny = 3,
    py = 4,
}

local maze = {}

for x=1, s do
    maze[x] = {}
    for y=1, s do
        if y ~= 1 then
            maze[x][y] = dir.ny
        elseif x ~= 1 then
            maze[x][y] = dir.nx
        else
            maze[x][y] = dir.root
        end
    end
end

local seed = {
    seed1 = 0
}

local function random(n)
    seed.seed1 = (3301 * seed.seed1 + 4993) % 6121
    return (seed.seed1) % n
end

local function frame()
    for x=1, xs do
        for y=1, ys do
            grid[x][y] = gui.Rectangle.BLACK
        end
    end

    for x=1, #maze do
        for y=1, #maze[x] do
            local px = x*2 + 1
            local py = y*2 + 1
            local ent = maze[x][y]
            grid[px][py] = gui.Rectangle.ORANGE
            if ent == dir.root then
                grid[px][py] = gui.Rectangle.RED
            end
            if ent == dir.nx then
                grid[px-1][py] = gui.Rectangle.YELLOW
            end
            if ent == dir.px then
                grid[px+1][py] = gui.Rectangle.YELLOW
            end
            if ent == dir.ny then
                grid[px][py-1] = gui.Rectangle.YELLOW
            end
            if ent == dir.py then
                grid[px][py+1] = gui.Rectangle.YELLOW
            end
        end
    end
end

local function maze_iter()
    for x=1, #maze do
        for y=1, #maze[x] do
            local ent = maze[x][y]
            if ent == dir.root then
                local more = true
                while more do
                    local v = random(4) + 1
                    maze[x][y] = v
                    if v == dir.nx and x ~= 1 then
                        maze[x-1][y] = dir.root
                        more = false
                    end
                    if v == dir.px and x ~= #maze then
                        maze[x+1][y] = dir.root
                        more = false
                    end
                    if v == dir.ny and y ~= 1 then
                        maze[x][y-1] = dir.root
                        more = false
                    end
                    if v == dir.py and y ~= #maze[x] then
                        maze[x][y+1] = dir.root
                        more = false
                    end
                end
            end
        end
    end
end

local function rep(n)
    return function()
        for i=1, n do
            maze_iter()
        end
    end
end

local code = gui.Code.new(frame)

draw = {}
draw.code = code
draw.grid = grid
draw.keys = {}
draw.keys.one = gui.Key.pressed("ONE", rep(1))
draw.keys.two = gui.Key.pressed("TWO", rep(8))
draw.keys.three = gui.Key.pressed("THREE", rep(64))
draw.keys.four = gui.Key.pressed("FOUR", rep(512))
draw.keys.five = gui.Key.pressed("FIVE", rep(2048))
draw.keys.six = gui.Key.pressed("SIX", rep(16386))
draw.window = gui.Window.new(600, 600)

app()
