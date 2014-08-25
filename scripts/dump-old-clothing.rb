df.world.items.all.select{|i| i.wear > 0 }.each {|i| i.flags.dump = true }
