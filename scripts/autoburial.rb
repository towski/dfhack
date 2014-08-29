df.world.buildings.all.select{|b| b.type.to_s =~ /coffin/i }.each{|c| c.burial_mode.allow_burial = true }
