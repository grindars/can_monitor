class WebMonitor
  module Logging
    module Loggable
      def log_as_facility(facility)
        define_method(:logger) do
          @logger ||= Logging.logger(facility)
        end
      end

      def log_as_eventmachine_facility(format)
        define_method(:logger) do
          if @logger.nil?
            port, ip = Socket.unpack_sockaddr_in(get_peername)
            @logger = Logging.logger(sprintf(format, ip, port))
          else
            @logger
          end
        end
      end
    end

    class Receiver < Logger::LogDevice
      
    end

    class Formatter < Logger::Formatter
      def initialize
        super

        @datetime_format = "%Y-%m-%d %H:%M:%S"
      end

      def call(severity, time, progname, msg)
        sprintf "[%s] %5s %s: %s\n", format_datetime(time), severity, progname, msg
      end
    end

    class LoggerProxy < BasicObject
      def initialize(facility, logger)
        @facility = facility
        @logger = logger
      end

      def self.define_log_handler(type, level)
        define_method(type) do |progname = @facility, &block|
          if block.nil?
            @logger.add level, progname, @facility, &block
          else
            @logger.add level, nil, progname, &block
          end
        end
      end

      define_log_handler :debug, ::Logger::DEBUG
      define_log_handler :info, ::Logger::INFO
      define_log_handler :warn, ::Logger::WARN
      define_log_handler :error, ::Logger::ERROR
      define_log_handler :fatal, ::Logger::FATAL
      define_log_handler :unknown, ::Logger::UNKNOWN

      def respond_to_missing?(name, include_private = false)
        super || @logger.respond_to?(name)
      end

      def method_missing(name, *args, &block)
        if @logger.respond_to? name
          @logger.send name, *args, &block
        else
          super
        end
      end
    end

    class << self
      def init(device)
        @receiver = Receiver.new(device)
        @logger   = Logger.new(device)
        @logger.formatter = Formatter.new
      end

      def reopen(device)
        @receiver.reopen device
      end

      def logger(facility = nil)
        if facility.nil?
          @logger
        else
          LoggerProxy.new(facility, @logger)
        end
      end
    end
  end
end
