local subjects = {"", "a", "aaa", "hello world", "x = [[a]] (b) {c}", "THE (quick) fox", "a.b-c_d", "  trim  ", "]", "a,b,,c", "\0z\0"}
local patterns = {
  "a", "a*", "a+", "a-", "a?", "^a", "a$", "^$", ".", ".-", "(a*)", "()", "(a)(a)",
  "%a+", "%A+", "%d", "%s*(.-)%s*$", "%w+", "%p", "%x+", "%z", "[%a_]+", "[^%s]+",
  "[a-]", "[]]", "[^]]+", "[a%-c]+", "[%a-z]", "%bxy", "%b()", "%b[]", "%f[%w]%w+",
  "%f[%W]", "(%w)%1", "((a)%2)", "(.)%1", "[a-c]+", "[c-a]", "a.-b", "^(%w+)",
  "(h)(e)(l)(l)(o)", "%.", "%%", "x*$", "[%]]", "^[^,]*", ",(,?)", "(%s*)$",
  "%", "[a", "%b", "%bx", "%fa", "(a", "a)", "%1", "(a)%2", "%0",
}
for _, s in ipairs(subjects) do
  for _, p in ipairs(patterns) do
    print(string.format("%q %q", s, p), pcall(string.find, s, p))
    print("  m", pcall(string.match, s, p))
  end
end
for _, p in ipairs({"%w+", "a*", "", "(%w)(%w)", "%f[%w]", ".-"}) do
  local out = {}
  local ok, err = pcall(function()
    for a, b in string.gmatch("one two  three", p) do
      out[#out + 1] = tostring(a) .. "/" .. tostring(b)
    end
  end)
  print("gmatch", p, ok, err, table.concat(out, " "))
end
for _, case in ipairs({{"hello world", "o", "0"}, {"abc", "%w", "%0%0"}, {"abc", "", "-"}, {"abc", "^a", "x"}, {"a.b", "%.", "%%"}, {"abc", "(b)", "[%1]"}, {"hello", "l+", {ll = "LL"}}}) do
  print("gsub", pcall(string.gsub, case[1], case[2], case[3]))
end
print(pcall(string.find, string.rep("a", 40), string.rep("(a)", 33)))
print(pcall(string.find, string.rep("a", 300), string.rep("a?", 300) .. string.rep("a", 300)))
print(pcall(string.match, string.rep("a", 50) .. "b", string.rep("a*", 50) .. "b"))
