local function show(f, err)
  if f then
    print(type(f))
  else
    print(err)
  end
end
x = 5
print(pcall(load("global x = 1")))
print(pcall(load("global y = 1; return y")))
show(load("global a; b = 1"))
show(load("global<const> c; c = 1"))
show(load("global *; d = 1; return d"))
print(pcall(load("global function f() return 7 end; return f()")))
show(load("local e; do global e; e = 1 end; return e"))
do
  global g
  g = 10
end
print(g)
show(load("global z <close>"))
show(load("local function q() end; global q; q = 3"))
show(load("global<const> *; w = 1"))
show(load("global k; local function h() return m end"))
