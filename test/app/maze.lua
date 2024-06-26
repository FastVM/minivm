
local gui = vm.import("test/app/gui.lua")

local debug = false

local s = 200

local xs = 2 * s + 3
local ys = 2 * s + 3

local grid = gui.Grid.new(xs, ys)

local solved = {}

for i=1, s do
    solved[i] = {}
    for j=1, s do
        solved[i][j] = false
    end
end

local colors = {
    solved = gui.Rectangle.GREEN,
    unsolved = gui.Rectangle.WHITE,
}

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

local vars = {
    cur = 1,
    last = 0,
    seed = 0
}

local function random4()
    return math.randint() % 4
end

local function dir_x(d)
    if d == dir.px then
        return 1
    elseif d == dir.nx then
        return -1
    else
        return 0
    end
end

local function dir_y(d)
    if d == dir.py then
        return 1
    elseif d == dir.ny then
        return -1
    else
        return 0
    end
end

local function frame()
    if vars.cur == vars.last then
        return nil
    end
    vars.last = vars.cur
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
            local c = if not solved[x] or not solved[x][y] then
                colors.unsolved
            else 
                colors.solved
            end
            grid[px][py] = c
            if ent == dir.root then
                grid[px][py] = c
            end
            if ent == dir.nx then
                grid[px-1][py] = c
            end
            if ent == dir.px then
                grid[px+1][py] = c
            end
            if ent == dir.ny then
                grid[px][py-1] = c
            end
            if ent == dir.py then
                grid[px][py+1] = c
            end
        end
    end
end

local function walk(x, y)
    local v = true
    solved[x][y] = v
    local run = true
    while run do
        local val = solved[x][y]
        local ent = maze[x][y]
        if ent == dir.root or ent == v then
            run = false
        end
        x = x + dir_x(ent)
        y = y + dir_y(ent)
        local n = solved[x][y]
        solved[x][y] = v
        if n then
            v = false
        end
    end
end

local points = {
    start = {1, 1},
    stop = {s, s},
}

local function rep(n)
    return function()
        vars.cur = vars.cur + 1
        local x = 0
        local y = 0
        for xp=1, #maze do
            for yp=1, #maze[xp] do
                if maze[xp][yp] == dir.root then
                    x = xp
                    y = yp
                end
            end
        end
        for i=1, n do
            local ent = maze[x][y]
            if ent == dir.root then
                local more = true
                while more do
                    local v = random4() + 1
                    maze[x][y] = v
                    if v == dir.nx and x ~= 1 then
                        maze[x-1][y] = dir.root
                        x = x - 1
                        more = false
                    end
                    if v == dir.px and x ~= #maze then
                        maze[x+1][y] = dir.root
                        x = x + 1
                        more = false
                    end
                    if v == dir.ny and y ~= 1 then
                        maze[x][y-1] = dir.root
                        y = y - 1
                        more = false
                    end
                    if v == dir.py and y ~= #maze[x] then
                        maze[x][y+1] = dir.root
                        y = y + 1
                        more = false
                    end
                end
            end
        end
        
        for i=1, s do
            for j=1, s do
                solved[i][j] = nil
            end
        end

        walk(points.start[1], points.start[2])
        walk(points.stop[1], points.stop[2])
    end
end

local code = gui.Code.new(frame)

local redo = rep(s * s * 50)

redo()

draw = {}
draw.code = code
draw.grid = grid
draw.keys = {}
draw.keys.n = gui.Key.pressed("N", redo)
draw.window = gui.Window.new(800, 800)

app()
