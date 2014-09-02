--[[ interiordesign <file.csv>
-- The letters match the b menu abbreviations, - is used a placeholder
example bedrooms.csv:
-, , ,b,c, , , , , , , , , , , , , , , , , , , , , , 
 , , ,f,-, ,b,c, ,c,b, ,b,c, ,c,b, ,b,c, ,c,b, ,c,b, ,
 , , , ,-, ,f,-, ,-,f, ,f,-, ,-,f, ,f,-, ,-,f, ,-,f, , , ,
 ,b,f, ,d, ,d, , , ,d, ,d, , , ,d, ,d, , , ,d, ,d, , ,f,b,
 ,c,-,d,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,d,-,-,c,
 , , , ,-, , , , , , , , , ,-, , , , , , , , , ,-, , , , ,
 ,b,f,d,-, , , ,b,-, ,-,b, ,-, ,b,-, ,-,b,f, , ,-,d,f,c, ,
 ,c,-, ,-, ,f, ,f,-, ,-,f, ,-, ,f,-, ,-, , , , ,-, ,-,b, ,
 , , , ,-, ,b, , ,d, ,d, , ,-, , ,d, ,d, ,f,b, ,-, , , , ,
 ,c,-, ,-, ,-,-,d,-,-,-,-,-,-,-,-,-,-,-,d,-,-, ,-, ,-,c, ,
 ,b,f,d,-, , , , ,-, , , , ,-, , , , ,-, , , , ,-,d,f,b, ,
 , , , ,-, ,-,-,d,-, ,-,f, ,-, ,-,-, ,-,d,-,-, ,-, , , , ,
 ,b,f,d,-, ,b,f, ,-, ,-,b, ,-, ,b,f, ,-, ,f,b, ,-,d,f,b, ,
 ,c,-, ,-, , , , ,-, ,d, , ,-, , ,d, ,-, , , , ,-, ,-,c, ,
 , , , ,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-, , 
 ,c,-, ,-, , , , ,-, ,d, , ,-, , ,d, ,-, , , , ,-, ,-,c, , 
 ,b,f,d,-, ,b,f, ,-, ,f,b, ,-, ,b,-, ,-, ,f,b, ,-,d,f,b, , 
 , , , ,-, ,-,-,d,-, ,-,-, ,-, ,f,-, ,-,d,-,-, ,-, , , , , 
 ,b,f,d,-, , , , ,-, , , , ,-, , , , ,-, , , , ,-,d,f,b, , 
 ,c,-, ,-, ,-,-,d,-,-,-,-,-,-,-,-,-,-,-,d,-,-, ,-, ,-,c, , 
 , , , ,-, ,b,f, ,d, ,d, , ,-, , ,d, ,d, , ,b, ,-, , , , , 
 ,c,-, ,-, , , , ,-, ,-,f, ,-, ,f,-, ,-,f, ,f, ,-, ,-,c, , 
 ,b,f,d,-, , ,f,b,-, ,-,b, ,-, ,b,-, ,-,b, , , ,-,d,f,b, , 
 , , , ,-, , , , , , , , , ,-, , , , , , , , , ,-, , , , , 
c,-,-,d,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,d,-,c, ,
b,f, , ,d, ,d, , , ,d, ,d, , , ,d, ,d, , , ,d, ,d, ,f,b, ,
 , , ,f,-, ,f,-, ,-,f, ,f,-, ,-,f, ,f,-, ,-,f, ,-, , , , ,
 , , ,b,c, ,b,c, ,c,b, ,b,c, ,c,b, ,b,c, ,c,b, ,-,f
 , , , , , , , , , , , , , , , , , , , , , , , ,c,b, 
--]]
local plugin = require('plugins.buildingplan')
local utils = require('utils')
args = {...}
file_name = args[1]
if file_name == nil then
  print("First parameter must be a csv file")
  return
end
local file, err = io.open(file_name, 'rb')
if not file then
  print('error: ',err)
  return
end 
old_type = df.global.ui_build_selector.building_type
old_x = df.global.cursor.x
old_y = df.global.cursor.y
str = file:read()
while str do
  df.global.cursor.x = old_x
  if str ~= '#' then
  end
  if not str then
    break
  end
  for i, char in ipairs(utils.split_string(str, ',')) do
    local building_type = nil
    if char == 'f' then
      building_type = 14
    elseif char == 'b' then
      building_type = 1
    elseif char == 'd' then
      building_type = 8
    elseif char == 'c' then
      building_type = 0
    elseif char == 't' then
      building_type = 2
    elseif char == 'h' then
      building_type = 10
    elseif char == 'R' then
      building_type = 45
    elseif char == 'n' then
      building_type = 3
    elseif char == 'v' then
      building_type = 27
    end
    if building_type then
      df.global.ui_build_selector.building_type = building_type
      plugin.makePlannedBuilding()
    end
    df.global.cursor.x = df.global.cursor.x + 1
  end
  df.global.cursor.y = df.global.cursor.y + 1
  str = file:read()
end
df.global.cursor.x = old_x
df.global.cursor.y = old_y
