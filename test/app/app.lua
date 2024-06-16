
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

_ENV.Button = Button
_ENV.Color = Color
_ENV.Rectangle = Rectangle
