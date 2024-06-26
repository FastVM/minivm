
local gui = vm.import("test/app/gui.lua")

local scale = 10

local xsize = 3 * scale
local ysize = 4 * scale

local grid = gui.Grid.new(xsize, ysize)
local window = gui.Window.new(600, 800)

local function on_draw()
    print(1)
end

local colors = {
    sky = gui.Rectangle.rgb(60, 60, 220),
    l1 = gui.Rectangle.rgb(220, 170, 60),
    l2 = gui.Rectangle.rgb(220, 100, 60),
    l3 = gui.Rectangle.rgb(220, 30, 60),
    l4 = gui.Rectangle.rgb(170, 30, 60),
    empty = gui.Rectangle.rgb(60, 60, 60),
    player = gui.Rectangle.rgb(230, 220, 240),
    enemy = gui.Rectangle.rgb(60, 220, 60),
}

local player = {
    x = ysize // 5 - 1, 
    y = xsize // 2,
}

local enemies = {
    {
        x = ysize * 3 // 5,
        y = xsize // 2,
        timer = 0,
        max = 2,
    },
    {
        x = ysize * 4 // 5,
        y = xsize * 2 // 3,
        timer = 0,
        max = 3,
    },
    {
        x = ysize * 4 // 5,
        y = xsize * 1 // 3,
        timer = 0,
        max = 5,
    },
}

local function reset(x, y)
    local p = x * 5 // ysize
    if p == 0 then
        grid[y][x] = colors.sky
    elseif p == 1 then
        grid[y][x] = colors.l1
    elseif p == 2 then
        grid[y][x] = colors.l2
    elseif p == 3 then
        grid[y][x] = colors.l3
    elseif p == 4 then
        grid[y][x] = colors.l4
    else
        grid[y][x] = colors.empty
    end
end

local function on_move()
    grid[player.y][player.x] = colors.player

    for i=1, #enemies do
        local enemy = enemies[i]
        local ex = enemy.x
        local ey = enemy.y
        if enemy.timer >= enemy.max then
            enemy.timer = 0
            local dx = enemy.x - player.x
            local dy = enemy.y - player.y
            local ax = dx
            if ax < 0 then
                ax = -ax
            end
            local ay = dy
            if ay < 0 then
                ay = -ay
            end
            if ax > ay then
                if dx < 0 then
                    enemy.x = enemy.x + 1
                else
                    enemy.x = enemy.x - 1
                end
            else
                if dy < 0 then
                    enemy.y = enemy.y + 1
                else
                    enemy.y = enemy.y - 1
                end
            end
        end
        if grid[enemy.y][enemy.x] == colors.empty then
            enemy.timer = enemy.timer + 2
        else
            enemy.timer = enemy.timer + 1
        end
        reset(ex, ey)
        grid[enemy.y][enemy.x] = colors.enemy
    end
end

local function on_init()
    for x=1, ysize do
        for y=1, xsize do
            reset(x, y)
        end
    end
    on_move()
end

local function set_dir(x, y)
    return function()
        grid[player.y][player.x] = colors.empty
        player.y = player.y + (x - 1)
        player.x = player.x + (y - 1)
        on_move()
    end
end

local onkey = {
    up = gui.Key.pressed("W", set_dir(1, 0)),
    down = gui.Key.pressed("S", set_dir(1, 2)),
    left = gui.Key.pressed("A", set_dir(0, 1)),
    right = gui.Key.pressed("D", set_dir(2, 1)),
}

draw = {
    grid,
    onkey,
    window,
}

on_init()

app()
