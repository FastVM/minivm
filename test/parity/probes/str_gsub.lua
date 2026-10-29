print(string.gsub("hello world", "o", "0"))
print(string.gsub("abc", "%a", function(c)
    return c:upper()
end))
