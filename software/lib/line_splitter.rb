class LineSplitter
  def initialize
    @buffer = ""
  end

  def add(chunk, &block)
    @buffer << chunk

    while true
      line_end = @buffer.index "\n"
      break if line_end.nil?

      line = @buffer.slice! 0, line_end
      @buffer.slice! 0, 1

      yield line
    end
  end
end
