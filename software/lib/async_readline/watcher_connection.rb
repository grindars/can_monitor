class AsyncReadline
  class WatcherConnection < EventMachine::Connection
    def initialize(parent, prompt, handler)
      @parent = parent
      @prompt = prompt
      @handler = handler
    end

    def post_init
      Library.rl_callback_handler_install @prompt, method(:callback)
      self.notify_readable = true
    end

    def notify_readable
      Library.rl_callback_read_char
    end

    def unbind
      Library.rl_callback_handler_remove
    end

    private

    def callback(line)
      EM.next_tick do
        @handler.call line
      end
    end
  end
end
