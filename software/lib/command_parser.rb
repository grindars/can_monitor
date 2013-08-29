class CommandParser
  def initialize
    @arguments = []
    @state = :normal
    @generate_argument = false
    @buffer = ""
  end

  def add(string)
    string.each_char do |char|
      case @state
      when :normal
        if char == " "
          if @generate_argument
            @generate_argument = false
            @arguments << @buffer
            @buffer = ""
          end
        elsif char == "\""
          @state = :quote
          @generate_argument = true
        else
          @buffer << char
          @generate_argument = true
        end

      when :quote
        if char == "\\"
          @state = :escape
        elsif char == "\""
          @state = :normal
        else
          @buffer << char
        end

      when :escape
        @buffer << char
        @state = :quote
      end
    end
  end

  def end
    case @state
    when :normal
      @arguments << @buffer if @generate_argument

    else
      raise ArgumentError, "unexpected end of string"
    end

    @arguments
  end
end
