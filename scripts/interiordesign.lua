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
    elseif char == 'D' then
      building_type = 8
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
