
local Color = {}

function Color.new(red, green, blue)
    return {
        red = red,
        green = green,
        blue = blue,
    }
end

Color.RED = Color.new(255, 0, 0)
Color.ORANGE = Color.new(255, 127, 0)
Color.YELLOW = Color.new(255, 255, 0)
Color.GREEN = Color.new(0, 255, 0)
Color.CYAN = Color.new(0, 255, 255)
Color.BLUE = Color.new(0, 0, 255)
Color.PURPLE = Color.new(255, 0, 255)
Color.PINK = Color.new(255, 192, 192)
Color.WHITE = Color.new(255, 255, 255)
Color.GRAY = Color.new(127, 127, 127)
Color.BLACK = Color.new(0, 0, 0)

local Rectangle = {}

function Rectangle.new(color)
    return {
        type = "rectangle",
        color = color,
    }
end

function Rectangle.rgb(red, green, blue)
    return {
        type = "rectangle",
        color = Color.new(red, green, blue),
    }
end

Rectangle.RED = Rectangle.new(Color.RED)
Rectangle.ORANGE = Rectangle.new(Color.ORANGE)
Rectangle.YELLOW = Rectangle.new(Color.YELLOW)
Rectangle.GREEN = Rectangle.new(Color.GREEN)
Rectangle.CYAN = Rectangle.new(Color.CYAN)
Rectangle.BLUE = Rectangle.new(Color.BLUE)
Rectangle.PURPLE = Rectangle.new(Color.PURPLE)
Rectangle.PINK = Rectangle.new(Color.PINK)
Rectangle.WHITE = Rectangle.new(Color.WHITE)
Rectangle.GRAY = Rectangle.new(Color.GRAY)
Rectangle.BLACK = Rectangle.new(Color.BLACK)

local Code = {}

function Code.new(run)
    return {
        type = "code",
        run = run,
    }
end

local Button = {}

function Button.new(button, run)
    return {
        type = "click",
        button = button,
        run = Code.new(run),
    }
end

function Button.left(func)
    return Button.new("left", func)
end

function Button.middle(func)
    return Button.new("middle", func)
end

function Button.right(func)
    return Button.new("right", func)
end


local Drag = {}

function Drag.new(button, run)
    return {
        type = "drag",
        button = button,
        run = Code.new(run),
    }
end

function Drag.left(func)
    return Drag.new("left", func)
end

function Drag.middle(func)
    return Drag.new("middle", func)
end

function Drag.right(func)
    return Drag.new("right", func)
end

local Hover = {}

function Hover.new(button)
    return {
        type = "hover",
        button = button,
        run = Code.new(run),
    }
end

local List = {}

function List.new(table)
    local r = {type="list"}
    for i=1, #table do
        r[i] = table[i]
    end
    return r
end

local Split = {}

function Split.new(table)
    local r = {type="split"}
    for i=1, #table do
        r[i] = table[i]
    end
    return r
end

local Grid = {}

function Grid.new(x, y)
    local ls = Split.new{}
    for i=1, x do
        local sp = List.new{}
        for j=1, y do
            sp[j] = {}
        end
        ls[i] = sp
    end
    return ls
end

local Key = {}

function Key.down(key, run)
    return {
        type = "keydown",
        key = key,
        run = Code.new(run),
    }
end

function Key.up(key, run)
    return {
        type = "keyup",
        key = key,
        run = Code.new(run),
    }
end

function Key.pressed(key, run)
    return {
        type = "keypressed",
        key = key,
        run = Code.new(run),
    }
end

function Key.released(key, run)
    return {
        type = "keyreleased",
        key = key,
        run = Code.new(run),
    }
end

local Window = {}

function Window.new(width, height)
    return {
        type = "window",
        width = width,
        height = height,
    }
end

return {
    Rectangle = Rectangle,
    Color = Color,
    Button = Button,
    Drag = Drag,
    List = List,
    Split = Split,
    Grid = Grid,
    Code = Code,
    Key = Key,
    Window = Window,
}
