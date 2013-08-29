class WebMonitor
  class Application
    extend Logging::Loggable

    log_as_facility "app"

    class TemplateContext
      def initialize(server)
        @server = server
      end

      def production?; false; end
      def development?; true; end

      def asset_path(asset)
        "/assets/#{asset}"
      end
    end

    def initialize(server)
      @server = server
    end

    def call(env)
      request = Rack::Request.new env
      response = Rack::Response.new

      if request.request_method == "GET" && request.path_info == "/"
        begin
          response.status = 200
          response['Content-Type'] = 'text/html; charset=utf-8'
          response.write compile_view('index.haml')
        rescue => e
          response.status = 500
          response['Content-Type'] = 'text/plain'
          response.write "Internal Server Error\n"
          response.write e.message
          response.write "\n"

          e.backtrace.each do |line|
            response.write line
            response.write "\n"
          end

          logger.error e
        end
      else
        response.status = 404
        response['Content-Type'] = 'text/plain'
        response.write "[#{request.request_method}] #{request.path_info}: Not Found"
      end

      response.finish
    end

    private

    def compile_view(name)
      tpl = File.read(File.expand_path("../../../views/#{name}", __FILE__))
      engine = Haml::Engine.new tpl,
        :mime_type => 'text/html',
        :format => :html5

      engine.render TemplateContext.new(@server)
    end
  end
end
