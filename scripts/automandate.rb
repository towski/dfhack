class AutoMandate
  def initialize
    @mandates = []
    set_mandates
  end

  def setdefault(v)
    @thresholds.default = v.to_i
  end

  def find_mandates
    mandates = df.world.mandates.select do |mandate|
      mandate.mode == 1
    end
    # we store our data in the hist_figure_id field
    before_orders = df.world.manager_orders
    @mandates.each do |mandate_id|
      puts "before orders"
      puts before_orders.inspect
      order = before_orders.find{|o| o.hist_figure_id }
      if order.nil?
        puts "Removing mandate #{mandate_id}"
        @mandates.delete mandate_id
      end
    end
    @before_orders = before_orders.map{|o| o._memaddr }
    mandates.each do |mandate|
      if !@mandates.include?(mandate.timeout_limit)
        puts "submitting mandate for #{mandate.timeout_limit}"
        material = "rock"
        type = mandate.item_type.to_s
        number = mandate.amount_remaining
        case mandate.item_type
        when :AMULET, :RING
          type = "crafts"
        when :BACKPACK
          material = "leather"
        when :CHAIR
          type = "throne"
        when :COIN
          material = "metal"
        end
        command =  "submit_order #{type} #{mandate.timeout_limit} #{material} #{number}"
        puts command
        df.dfhack_run command
      end
    end
  end

  def set_mandates
    @mandates = df.world.manager_orders.select do |order|
      order.hist_figure_id != -1
    end.map{|o| o.hist_figure_id }
  end

  def update(mandate_id)
    after_orders = df.world.manager_orders.map{|o| o._memaddr }
    orders = after_orders - @before_orders
    @before_orders = after_orders
    if orders.length == 1
      order_id = orders.first
      puts "setting job #{mandate_id} #{orders.first}"
      @mandates << mandate_id.to_i
      order = df.world.manager_orders.find{|o| o._memaddr == order_id }
      order.hist_figure_id = mandate_id.to_i
    end
  end

  def process
    puts "Finding mandates"
    find_mandates
    return unless @running
  end

  def start
    return if @running
    @onupdate = df.onupdate_register('automandate', 1200) { process }
    @running = true
  end

  def stop
    df.onupdate_unregister(@onupdate)
    @running = false
  end

  def status
    stat = @running ? "Running." : "Stopped."
    @mandates.each { |k,v|
      stat << "mandate #{k} : #{v}"
    }
    stat
  end
end

case $script_args[0]
when 'enable' 
  df.onstatechange_register_once { |st|
    puts st
    if st == :MAP_LOADED
      $AutoMandate = AutoMandate.new
      $AutoMandate.start
    elsif st == :MAP_UNLOADED
      $AutoMandate.stop
    end
  }
when 'start' 
  if $AutoMandate
    $AutoMandate.start
    puts $AutoMandate.status
  end

when 'update'
  $AutoMandate.update $script_args[1]

when 'end', 'stop', 'disable'
  $AutoMandate.stop
  puts 'Stopped.'

when 'status'
  puts $AutoMandate.status

when 'set_mandates'
  $AutoMandate.set_mandates($script_args[1])

when 'delete'
  $AutoMandate.stop
  $AutoMandate = nil

when 'help', '?'
  puts <<EOS
Automatically fulfill nobles mandates

Usage:
 automandate start
 automandate default 30
EOS

when 'process'
  $AutoMandate.process
  puts $AutoMandate.status
end
