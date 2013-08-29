class AsyncReadline
  class LogRedirector
    def initialize(readline)
      @readline = readline
      @splitter = LineSplitter.new
    end

    def write(string)
      @splitter.add string do |line|
        @readline.message line
      end
    end

    def flush; end
    def close; end
  end
end
