class WebMonitor
  module ThinLoggingIntegration
    attr_accessor :logger

    def log(msg)
      @logger.info msg
    end

    def trace(msg = nil, &block)
      @logger.debug msg, &block
    end

    def debug(msg = nil, &block)
      @logger.debug msg, &block
    end

    def log_error(e = $!)
      @logger.error e
    end        
  end
end
