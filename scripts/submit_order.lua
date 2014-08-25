local gui = require "gui"
local utils = require "utils"
local table = require "table"
local stockflow = require 'plugins.stockflow'

materials = {
    rock = {
        adjective = "rock",
        management = {mat_type = 0},
    },
    metal = {
        adjective = "metal",
        management = {mat_type = 0, mat_index = 3}, -- copper for now
    }
}

for _, name in ipairs{"wood", "cloth", "leather", "silk", "yarn", "bone", "shell", "tooth", "horn", "pearl"} do
    materials[name] = {
        adjective = name,
        management = {material_category = {[name] = true}},
    }
end

materials.wood.adjective = "wooden"
materials.tooth.adjective = "ivory/tooth"
materials.leather.clothing_flag = "LEATHER"

print(materials['rock'])

MAX_JOB_ID = 231

function getJobType(tbl, value)
  local i = 0
  while (i>=0 and i<=MAX_JOB_ID) do
    v = tbl[i]
    if v.caption then
      if string.find(v.caption:lower(), value) then
        return i
      end
    end
    i = i + 1
  end

  return nil
end

args = {...}
opt = args[1]
mandate_id = args[2]
material = args[3] or 'stone'
number = args[4] or 1
print(opt)
print(mandate_id)
if opt == nil then
  print("First parameter must be a string for job creation")
  return
end
if material == nil then
  print("Needs material")
end

local job = getJobType(df.job_type.attrs, opt:lower())
if job == nil or job == 0 then
  print("No job found for '" .. opt .."'")
  return
end

local new_order = df.manager_order:new()
    new_order:assign{
        job_type = job,
        unk_2 = -1,
        item_subtype = -1,
        mat_type = -1,
        mat_index = -1,
    }

new_order:assign(materials[material].management)
new_order.amount_left = number
new_order.amount_total = number
new_order.is_validated = 0
df.global.world.manager_orders:insert('#', new_order)
dfhack.run_command("automandate update " .. mandate_id)
