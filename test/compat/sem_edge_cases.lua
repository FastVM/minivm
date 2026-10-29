local function try(label, f, ...)
    print(label, pcall(f, ...))
end

local z = 0
try("idiv0", function(a) return a // z end, 7)
try("idiv0k", function(a) return a // 0 end, 7)
try("mod0", function(a) return a % z end, 7)

local main = debug.getinfo(1, "S")
print("main lastlinedefined", main.lastlinedefined, main.what)

local keys = {}
for i = 1, 4 do
    keys[i] = string.pack("i4i4", 0, i)
end
local t = {}
for i = 1, 4 do
    t[keys[i]] = i
end
for i = 1, 4 do
    io.write(t[keys[i]], " ")
end
print()

print(pcall(error, 2.0))
print(pcall(next, io.stdout))

io.open("/nonexistent/review_leads")
local pipe = io.popen("exit 3")
print("pclose", pipe:close())

print("bytes lt", "a\0b" < "a\0c", "a" < "a\0", "a\0" < "a")
