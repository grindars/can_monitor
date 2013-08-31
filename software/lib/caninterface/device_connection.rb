module CANInterface
  class DeviceConnection < EventMachine::Connection
    ESCAPE_MASK    = 0x20
    ESCAPE         = 0x10
    XON            = 0x11
    XOFF           = 0x13
    START_OF_FRAME = 0x7F
    ACK            = 0x00
    ACK_TIMEOUT    = 0.2
    MAX_RETRIES    = 3

    class Message
      attr_reader :type, :data, :serial
      attr_accessor :sent
      attr_accessor :retries, :retry_timer

      def initialize(type, data, serial = nil)
        @type = type
        @data = data
        if serial.nil?
          @@serial ||= 0
          @@serial += 1
          @@serial &= 65535
          
          @serial = @@serial
        else
          @serial = serial
        end
      end
    end

    def initialize(watcher)
      @watcher = watcher
      @escape_received = false
      @state = :sof
      @message_type = nil
      @message_data_size = nil
      @message_serial = nil
      @message_data = "".encode("BINARY")
      @sent_messages = Set[]
    end

    def post_init
      EM.next_tick do
        @watcher.connection_ready
      end
    end

    def unbind
      @watcher.connection_closed
    end

    def receive_data(string)
      log_frame false, string

      string.each_byte do |byte|
        byte = unstuff_byte byte
        next if byte.nil?

        case @state
        when :sof
          if byte == START_OF_FRAME
            @state = :type
          end

        when :type
          @message_type = byte
          @state = :data_size

        when :data_size
          @message_data_size = byte
          @state = :serial_low
        
        when :serial_low
          @message_serial = byte
          @state = :serial_high

        when :serial_high
          @message_serial |= byte << 8
          @message_data.clear

          if @message_data_size == 0
            @state = :sof
            dispatch_message
          else
            @state = :message_data
          end

        when :message_data
          @message_data << byte.chr
          @message_data_size -= 1

          if @message_data_size == 0
            @state = :sof
            dispatch_message
          end
        end
      end
    end

    def self.open(watcher, *args)
      io = SerialPort.new *args
      io.flow_control = SerialPort::SOFT
      EventMachine.attach io, self, watcher
    end

    def send_message(type, data, &block)
      message = Message.new type, data
      message.sent = block

      @sent_messages.add message
      message.retries = 0
      try_to_send message
    end

    private

    def try_to_send(message)
      message.retry_timer = EventMachine.add_timer ACK_TIMEOUT do
        message.retry_timer = nil

        if message.retries == MAX_RETRIES
          @sent_messages.delete message
          unless message.sent.nil?
            message.sent.call false
          end
        else
          message.retries += 1
          try_to_send message
        end
      end

      send_frame message.type, message.serial, message.data
    end

    def dispatch_message
      if @message_type == ACK
        message = @sent_messages.detect { |message| message.serial == @message_serial }
        unless message.nil?
          unless message.retry_timer.nil?
            EventMachine.cancel_timer message.retry_timer
            message.retry_timer = nil
          end

          @sent_messages.delete message

          unless message.sent.nil?
            message.sent.call true
          end
        end
      else
        @watcher.handle_message Message.new(@message_type, @message_data, @message_serial)
        send_frame ACK, @message_serial, ""
      end
    end

    def send_frame(type, serial, data)
      frame = stuff_string([ START_OF_FRAME, type, data.size, serial ].pack("CCCv") + data.force_encoding("BINARY"))

      log_frame true, frame
      send_data frame
    end

    def stuff_string(string)
      stuffed = "".encode("BINARY")
      string.each_byte do |byte|
        if byte == ESCAPE || byte == XON || byte == XOFF
          stuffed << ESCAPE.chr
          stuffed << (byte ^ ESCAPE_MASK).chr
        else
          stuffed << byte.chr
        end
      end
      stuffed
    end

    def unstuff_byte(byte)
      if byte == ESCAPE
        @escape_received = true

        nil

      elsif @escape_received
        @escape_received = false

        byte ^ ESCAPE_MASK
      else
        byte
      end
    end

    def log_frame(sent, data)
      io = @watcher.trace_log

      unless io.nil?
        time = Time.now
        length = data.length
        length |= 0x80000000 if sent

        header = [
          0xDEADBEEF,
          time.tv_sec,
          time.tv_nsec,
          length
        ].pack("V*")

        io.write header
        io.write data
        io.flush
      end 

    rescue => e
      warn "logging failed: #{e}"
      e.backtrace.each { |line| warn line }
    end
  end
end
