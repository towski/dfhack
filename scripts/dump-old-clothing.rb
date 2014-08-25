df.world.items.all.select{|i| i.wear > 1 }.each {|i| i.flags.dump = true }
