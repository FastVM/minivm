print(3 & 7)
print(3.0 & 7)
print(7.0 >> 1)
print(3.0 | 4)
-- Strip the leading "source:line:" location so the probe is path-independent
-- while still verifying the error message text.
local function err(f)
  local ok, msg = pcall(f)
  if type(msg) == "string" then
    msg = msg:gsub("^.-:%d+: ", "")
  end
  return ok, msg
end
print(err(function() return 3.5 & 7 end))
print(err(function() return 3 & 7.5 end))
print(err(function() return 4.5 | 1 end))
print(err(function() return 7.5 ~ 1 end))
print(err(function() return ~3.14 end))
