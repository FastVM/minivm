
local gui = vm.import("test/app/gui.lua")

local s = 3

local xs = 16 * s
local ys = 9 * s

local grid = gui.Grid.new(xs, ys)

local empty = gui.Rectangle.rgb(0, 0, 0)
local alive = gui.Rectangle.rgb(0, 128, 0)
local dead = gui.Rectangle.rgb(128, 128, 128)
local apple = gui.Rectangle.rgb(200, 0, 0)

local vars = {
    snake = {{2, ys // 2}},
    dir = {2, 1},
    time = 1,
    sleep = 4,
    seed = 0,
    apples = {},
    dead = {}
}

local n = 5
for i=1, n do
    vars.apples[#vars.apples + 1] = {xs / n * i // 1 - 1, ys // 2}
end

local function random(n)
    vars.seed = (101 * vars.seed + 103) % 919
    return vars.seed % n
end

local function on_draw()
    vars.time = vars.time + 1
    if vars.time % (vars.sleep + 1) == 0 then
        for i=0, xs * ys - 1 do
            local y = i % xs + 1
            local x = i // xs + 1
            grid[y][x] = empty
        end

        local piece = {
            (vars.snake[1][1] + (vars.dir[1] - 1) - 1 + xs) % xs + 1,
            (vars.snake[1][2] + (vars.dir[2] - 1) - 1 + ys) % ys + 1,
        }
        
        for i=1, #vars.snake do
            local cur = vars.snake[i]
            if cur[1] == piece[1] then
                if cur[2] == piece[2] then
                    local s = {}
                    for j=1, i do
                        local copy = vars.snake[j]
                        s[j] = {copy[1], copy[2]}
                    end
                    vars.snake = s
                    break
                end
            end
        end
        
        local last = vars.snake[#vars.snake]

        vars.snake[0] = piece

        for i=1, #vars.snake do
            local n = #vars.snake + 1 - i
            vars.snake[n] = vars.snake[n-1]
        end

        for i=1, #vars.apples do
            if vars.snake[1][1] == vars.apples[i][1] then
                if vars.snake[1][2] == vars.apples[i][2] then
                    vars.apples[i][1] = random(xs) + 1
                    vars.apples[i][2] = random(ys) + 1
                    vars.snake[#vars.snake + 1] = last
                end
            end
        end
        
        for i=1, #vars.snake do
            local p = vars.snake[i]
            if vars.dead[i] then
                grid[p[1]][p[2]] = dead
            else
                grid[p[1]][p[2]] = alive
            end
        end

        for i=1, #vars.apples do
            local p = vars.apples[i]
            grid[p[1]][p[2]] = apple
        end
    end
end

local function set_dir(d)
    return function()
        vars.dir = d
    end
end

local frame = gui.Code.new(on_draw)

local onkey = {
    up = gui.Key.down("W", set_dir{1, 0}),
    down = gui.Key.down("S", set_dir{1, 2}),
    left = gui.Key.down("A", set_dir{0, 1}),
    right = gui.Key.down("D", set_dir{2, 1}),
}

draw = {}
draw.draw = grid
draw.frame = frame
draw.onkey = onkey
draw.vars = vars

app()
