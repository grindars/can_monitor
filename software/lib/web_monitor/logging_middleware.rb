class WebMonitor
  class LoggingMiddleware
    class ErrorReceiver
      def initialize(logger)
        @logger = logger
        @buffer = ""
      end

      def puts(string)
        write string
        write "\n"
      end

      def write(string)
        @buffer << string

        while true
          end_of_line = @buffer.index "\n"
          break if end_of_line.nil?

          @logger.error @buffer.slice(0, end_of_line)
          @buffer.slice! 0, end_of_line + 1
        end
      end

      def flush
        unless @buffer.empty?
          @logger.error @buffer
          @buffer.clear
        end
      end
    end

    extend Logging::Loggable

    log_as_facility "application"

    def initialize(app)
      @app = app
      @receiver = ErrorReceiver.new(logger)
    end

    def call(request)
      request['rack.logger'] = logger
      request['rack.errors'] = @receiver

      log_request request

      response = @app.call request
      
      log_response response
      
      response
    end

    private

    def log_request(env)
      @started = Time.now

      logger.info "#{env["REQUEST_METHOD"]} #{env["PATH_INFO"]} for #{env["HTTP_X_FORWARDED_FOR"] || env["REMOTE_ADDR"]}"
    end

    def log_response((status, header, body))
      @finished = Time.now

      logger.info "rendered #{status} in #{((@finished - @started) * 1000).round} ms"
    end
  end
end