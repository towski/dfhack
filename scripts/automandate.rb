class AutoMandate
  def initialize
    @thresholds = Hash.new(50)
    @lastcounts = Hash.new(0)
    @mandates = Hash.new
  end

  def setdefault(v)
    @thresholds.default = v.to_i
  end

  def find_mandates
    mandates = df.world.mandates.select do |mandate|
      mandate.mode == 1
    end
    @before_jobs = df.world.manager_orders.map{|o| o._memaddr }
    @mandates.each do |key, value|
      if !@before_jobs.include?(value)
        puts "Removing mandate #{key} with job #{value}"
        @mandates.delete key
      end
    end
    mandates.each do |mandate|
      if @mandates[mandate.timeout_limit.to_s].nil?
        puts "submitting mandate for #{mandate.timeout_limit}"
        material = "rock"
        type = mandate.item_type.to_s
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
        command =  "submit_order #{type} #{mandate.timeout_limit} #{material}"
        puts command
        df.dfhack_run command
      end
    end
  end

  def update(mandate_id)
    after_jobs = df.world.manager_orders.map{|o| o._memaddr }
    jobs = after_jobs - @before_jobs
    @before_jobs = after_jobs
    if jobs.length == 1
      puts "setting job #{mandate_id} #{jobs.first}"
      @mandates[mandate_id] = jobs.first
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

$AutoMandate ||= AutoMandate.new

case $script_args[0]
when 'start', 'enable'
  $AutoMandate.start
  puts $AutoMandate.status

when 'update'
  $AutoMandate.update $script_args[1]

when 'end', 'stop', 'disable'
  $AutoMandate.stop
  puts 'Stopped.'

when 'default'
  $AutoMandate.setdefault($script_args[1])

when 'status'
  puts $AutoMandate.status()

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

else
  $AutoMandate.process
  puts $AutoMandate.status
end
