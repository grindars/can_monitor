require_relative "caninterface"
require_relative "async_readline"
require_relative "command_parser"

class CANMonitor
  DEBUG = 1
  PING  = 2
  PONG  = 3
  CAN_CONTROL = 4
  CAN_MESSAGE = 5

  CAN_ADDRESS_REMOTE = 1 << 1
  CAN_ADDRESS_EXTENDED = 1 << 2

  FLAGS = {
    "enabled"   => 1,
    "silent"    => 2,
    "loopback"  => 4
  }

  attr_reader :trace_log

  def initialize(opts)
    @opts = opts
    @debug_splitter = LineSplitter.new
    @readline = AsyncReadline.new
    @logger = Logger.new @readline.create_log_redirector
  end

  def connection_ready
    @logger.info "device connected"
    send_ping
    read_command
  end

  def connection_closed
    @logger.info "connection closed"
    @connection = nil
    EventMachine.stop_event_loop
  end

  def handle_message(message)
    case message.type
    when DEBUG
      @debug_splitter.add message.data do |line|
        @logger.debug "device: #{line}"
      end

    when PING
      @logger.info "PING received"
      @connection.send_message PONG, message.data

    when PONG
      @logger.info "PONG received"

    when CAN_MESSAGE
      info = "CAN: "
      address, timestamp_length = message.data.unpack("VV")
      data = message.data.slice 8, (timestamp_length & 15)

      info << (timestamp_length >> 16).to_s
      info << " FMI:#{((timestamp_length & 0xFF00) >> 8).to_s} "

      if (address & CAN_ADDRESS_REMOTE) != 0
        info << "RTR "
      end

      if (address & CAN_ADDRESS_EXTENDED) != 0
        info << "EXT:#{(address >> 3).to_s(16)} "
      else
        info << "STD:#{(address >> 21).to_s(16)} "
      end

      data.each_byte do |byte|
        info << "#{byte.to_s(16)} "
      end

      @logger.info info
    else
      @logger.warn "incoming message of type #{message.type}: #{message.data}"
    end
  end

  def run
    unless @opts[:trace].nil?
      @trace_log = File.open @opts[:trace], "ab"
    end

    EventMachine.run do
      @connection = CANInterface::DeviceConnection.open self, @opts[:device], 460800
    end

    unless @trace_log.nil?
      @trace_log.close
      @trace_log = nil
    end
  end

  private

  def read_command
    @readline.readline("> ") do |line|
      begin
        if line.nil?
          EventMachine.stop_event_loop
        else
          command_parser = CommandParser.new
          command_parser.add line
          execute_command *command_parser.end
        end
      rescue => e
        @logger.error "Unable to execute #{line.inspect}: #{e}"
        e.backtrace.each do |line|
          @logger.error line
        end
      end
    end
  end

  def flag_by_name(name)
    flag = FLAGS[name]
    raise "invalid flag: #{name}" if flag.nil?
    flag
  end

  def execute_command(command, *args)
    case command
    when "mode"
      flags = args.map(&method(:flag_by_name)).reduce(0, &:|)
      @connection.send_message CAN_CONTROL, [ flags ].pack("V") do |acked|
        if acked
          @logger.info "mode changed"
        else
          @logger.info "mode not changed"
        end
      end

    when "ping"
      send_ping

    when "send"
      address = nil
      flags = 0
      data = ""

      for arg in args
        if arg == "remote"
          flags |= CAN_ADDRESS_REMOTE

        elsif arg =~ /^std:([0-9a-fA-F]+)$/
          address = $1.to_i(16) << 21

        elsif arg =~ /^ext:([0-9a-fA-F]+)$/
          address = $1.to_i(16) << 3
          flags |= CAN_ADDRESS_EXTENDED
        
        elsif arg =~ /^([0-9a-fA-F]{2})$/
          data << $1.to_i(16).chr

        else
          logger.error "unknown argument: #{arg.inspect}"
          return
        end
      end

      if address.nil?
        @logger.error "syntax: send [remote] <ext|std>:<address> [data bytes in hex...]"
        return
      end

      address |= flags

      if data.length > 8
        @logger.error "data is too long"
        return
      end

      @connection.send_message CAN_MESSAGE, ([ address, data.length ].pack("V*") + data.ljust(8, "\0")) do |success|
        if success
          @logger.info "message sent"
        else
          @logger.info "message not sent"
        end
      end

    else
      @logger.error "unknown command: #{command}. try ping, mode or send"
    end
  end

  def send_ping
    @connection.send_message PING, "yes we CAN" do |sent|
      if sent
        @logger.info "PING sent"
      else
        @logger.warn "PING NOT sent - check connection"
      end
    end
  end
end
