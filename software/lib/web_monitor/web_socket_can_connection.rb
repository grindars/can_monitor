class WebMonitor
  class WebSocketCANConnection < EventMachine::WebSocket::Connection
    extend Logging::Loggable

    DEBUG = 1
    PING  = 2
    PONG  = 3
    CAN_CONTROL = 4
    CAN_MESSAGE = 5

    CAN_ADDRESS_REMOTE = 1 << 1
    CAN_ADDRESS_EXTENDED = 1 << 2

    log_as_eventmachine_facility "em-rpc-%s:%d"

    def initialize(server, options)
      super(options)
      @server = server
      @debug_splitter = LineSplitter.new
    end

    def trigger_on_open(handshake)
      logger.debug "connected"
    
      @server.register_can_client self
    end

    def trigger_on_close(status)
      logger.debug "disconnected (#{status[:code]})"

      @server.unregister_can_client self
    end

    def trigger_on_ping(data); end
    def trigger_on_pong(data); end

    def trigger_on_message(msg)
      msg = JSON.load(msg)

      case msg["action"]
      when "transmit"
        parser = CommandParser.new
        parser.add msg["message"]
        transmit *parser.end

      when "ping"
        @server.send_message PING, "yes we CAN" do |sent|
          if sent
            print "PING sent\n"
          else
            print "PING NOT sent - check connection\n"
          end
        end

      when "mode"
        mode = 0

        mode |= 1 if msg["enabled"]
        mode |= 2 if msg["silent"]
        mode |= 4 if msg["loopback"]

        @server.send_message CAN_CONTROL, [ mode ].pack("V") do |acked|
          if acked
            print "mode changed\n"
          else
            print "mode not changed\n"
          end
        end

      else
        raise ArgumentError, "unsupported action #{msg["action"]}"
      end

    rescue => e
      print "command processing failed:\n"
      print "#{e}\n"
      e.backtrace.each do |line|
        print "#{line}\n"
      end
    end

    def print(message)
      send_msg action: "debug", message: message
    end

    def handle_can(message)
      case message.type
      when DEBUG
        @debug_splitter.add message.data do |line|
          print "device: #{line}\n"
        end

      when PING
        print "PING received\n"
        @server.send_message PONG, message.data

      when PONG
        print "PONG received\n"

      when CAN_MESSAGE
        address, timestamp_length = message.data.unpack("VV")
        data = message.data.slice 8, (timestamp_length & 15)

        address_string = ""
        data_string = ""

        if (address & CAN_ADDRESS_REMOTE) != 0
          address_string << "RTR "
        end

        if (address & CAN_ADDRESS_EXTENDED) != 0
          address_string << "EXT:#{(address >> 3).to_s(16)}"
        else
          address_string << "STD:#{(address >> 21).to_s(16)}"
        end

        data.each_byte do |byte|
          data_string << "#{byte.to_s(16)} "
        end

        send_msg(
          action: "can_message",
          address: address_string,
          data: data_string
        )

      else
        logger.warn "incoming message of type #{message.type}: #{message.data}"
      end
    end

    private

    def transmit(*args)
      flags = 0
      data = ""
      address = nil

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
          print "unknown argument: #{arg.inspect}\n"
          return
        end
      end

      if address.nil?
        print "syntax: send [remote] <ext|std>:<address> [data bytes in hex...]\n"
        return
      end

      address |= flags

      if data.length > 8
        print "data is too long\n"
        return
      end

      @server.send_message CAN_MESSAGE, ([ address, data.length ].pack("V*") + data.ljust(8, "\0")) do |success|
        if success
          print "message sent\n"
        else
          print "message not sent\n"
        end
      end
    end

    def send_msg(obj)
      send JSON.dump(obj)
    end
  end
end
