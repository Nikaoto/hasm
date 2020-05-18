-- print_table module

local indentation_unit = " "
local indentation_mult = 2

local function indent(str, indentation_lvl)
   return string.rep(indentation_unit, indentation_lvl * indentation_mult) .. str
end

local function print_table(tbl, indentation_lvl)
   if tbl == nil then print("nil") return end
   indentation_lvl = indentation_lvl or 1
   if indentation_lvl == 1 then print("{") end
   for k, v in pairs(tbl) do
      if type(v) == "table" then
         print(indent(string.format("%s = {", k), indentation_lvl))
         print_table(v, indentation_lvl + 1)
         print(indent("}", indentation_lvl))
      else
         print(indent(string.format("%s = %s", k, v), indentation_lvl))
      end
   end
   if indentation_lvl == 1 then print("}") end
end

return print_table
