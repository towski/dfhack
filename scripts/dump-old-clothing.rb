age = $script_args[0] || 1
df.world.items.all.select{|i| i.wear > age.to_i }.each {|i| i.flags.dump = true }
