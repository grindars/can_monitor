require 'ffi'
require_relative "line_splitter"

class AsyncReadline
  def initialize
    @watcher = nil
  end

  def readline(prompt, &block)
    @watcher = EventMachine.watch STDIN, WatcherConnection, self, prompt, block
  end

  def message(msg)
    if @watcher.nil?
      STDOUT.puts msg
    else
      print "\r\033[K"
      puts msg
      Library.rl_forced_update_display
    end
  end

  def create_log_redirector
    LogRedirector.new self
  end

  def detach_watcher
    unless @watcher.nil?
      @watcher.detach
      @watcher = nil
    end
  end
end

require_relative "async_readline/library"
require_relative "async_readline/log_redirector"
require_relative "async_readline/watcher_connection"
