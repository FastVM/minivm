
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

local Button = {}

function Button.new(click, func)
    return {
        type = "button",
        click = click,
        run = func,
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

local List = {}

function List.new(table)
    local r = {type="list"}
    for i=1, #table do
        r[i] = table[i]
    end
    return r
end

_ENV.List = List
_ENV.Button = Button
_ENV.Color = Color
_ENV.Rectangle = Rectangle
