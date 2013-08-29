class WebMonitor
  class WebSocketBridge
    def initialize(server)
      @server = server
    end

    def call(req)
      connection = req['async.callback'].receiver
      
      EventMachine.next_tick do
        routed = @server.call(req)

        unless routed.first.kind_of? Class
          req['async.callback'].call routed
        else
          fd = connection.detach
          ws_conn = EventMachine.attach IO.for_fd(fd), *routed, {}

          greet = ''.force_encoding("BINARY")

          url = req["PATH_INFO"]
          unless req["QUERY_STRING"].empty?
            url << "?#{req["QUERY_STRING"]}"
          end

          greet << "#{req["REQUEST_METHOD"]} #{url} #{req["HTTP_VERSION"]}\r\n"

          req.each do |key, value|
            case key
            when "HTTP_VERSION"

            when "CONTENT_LENGTH"
              greet << "Content-Length: #{value}\r\n"

            when "CONTENT_TYPE"
              greet << "Content-Type: #{value}\r\n"            

            else
              name = key.dup
              if name.gsub!(/\Ahttp_([a-z_]+)\z/i) { $1.split('_').map! { |r| r.downcase!.capitalize! }.join('-') }
                greet << "#{name}: #{value}\r\n"
              end
            end
          end

          greet << "\r\n"
          greet << connection.request.body.read
          ws_conn.receive_data greet
        end
      end

      [ -1, {}, [] ]
    end
  end
end
