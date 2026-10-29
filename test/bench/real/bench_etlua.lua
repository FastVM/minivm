local etlua = require("etlua")

local page = etlua.compile([[
<html>
<head><title><%= title %></title></head>
<body>
<h1><%= title %></h1>
<ul>
<% for i, item in ipairs(items) do %>
  <li class="<%= i % 2 == 0 and "even" or "odd" %>">
    <span><%- item.name %></span> = <%= item.value * 2 %>
  </li>
<% end %>
</ul>
<p><%= footer %></p>
</body>
</html>
]])

local items = {}
for i = 1, 30 do
  items[i] = {name = "item <" .. i .. ">", value = i * 3}
end

local iters = 15000
local sum, len = 0, 0
for it = 1, iters do
  local out = page({
    title = "Page " .. (it % 7),
    items = items,
    footer = "generated " .. it,
  })
  len = #out
  if it == iters then
    for i = 1, #out do sum = (sum * 131 + out:byte(i)) % 1000000007 end
  end
end

-- per-iteration compile of a small template
local csum = 0
for it = 1, 1000 do
  local small = etlua.render("v=<%= v %>;", {v = it})
  csum = (csum + #small + small:byte(3)) % 1000000007
end
print("etlua", iters, len, sum, csum)
