local lpeg = require("lulpeg")
local P, R, S, C, Ct, V = lpeg.P, lpeg.R, lpeg.S, lpeg.C, lpeg.Ct, lpeg.V

-- csv splitter
local field = C((1 - S',\n"')^0) + P'"' * C(((1 - P'"') + P'""')^0) * P'"'
local record = Ct(field * (P',' * field)^0) * (P'\n' + -1)

-- arithmetic expression grammar evaluating to a number
local expr = P{
  "Exp",
  Exp = V"Term" * (C(S"+-") * V"Term")^0 / function(a, ...)
    local args = {...}
    local acc = a
    for i = 1, #args, 2 do
      if args[i] == "+" then acc = acc + args[i + 1] else acc = acc - args[i + 1] end
    end
    return acc
  end,
  Term = V"Factor" * (C(S"*/") * V"Factor")^0 / function(a, ...)
    local args = {...}
    local acc = a
    for i = 1, #args, 2 do
      if args[i] == "*" then acc = acc * args[i + 1] else acc = acc // args[i + 1] end
    end
    return acc
  end,
  Factor = P"(" * V"Exp" * P")" + (R"09"^1 / tonumber),
}

local rows = {}
for i = 1, 40 do
  rows[#rows + 1] = string.format('a%d,"quoted %d, with comma",%d,%d', i, i, i * 3, i * i)
end
local csv = table.concat(rows, "\n")

local exprs = {}
for i = 1, 40 do
  exprs[#exprs + 1] = string.format("(%d+%d)*%d-(%d*%d)/(%d+1)", i, i + 1, i + 2, i, i + 3, i)
end

local iters = 500
local sum = 0
for it = 1, iters do
  local pos = 1
  local nfields = 0
  while pos <= #csv do
    local t, np = record:match(csv, pos)
    if t == nil then break end
    nfields = nfields + #t
    for j = 1, #t do
      sum = (sum + #t[j]) % 1000000007
    end
    pos = np or (#csv + 1)
    if np == nil then break end
  end
  for j = 1, #exprs do
    sum = (sum + expr:match(exprs[j])) % 1000000007
  end
  sum = (sum + nfields) % 1000000007
end
print("lulpeg", iters, sum)
