require "sprockets"
require "thin"
require "em-websocket"
require "logger"
require "haml"
require "json"

class WebMonitor

  attr_reader :trace_log

  def initialize(opts)
    @opts = opts

    route_websocket = method :route_websocket

    root = File.expand_path("../..", __FILE__)

    sprockets = Sprockets::Environment.new
    sprockets.append_path "#{root}/assets/javascripts"
    sprockets.append_path "#{root}/assets/stylesheets"
    sprockets.append_path "#{root}/vendor/assets/javascripts"
    sprockets.append_path "#{root}/vendor/assets/stylesheets"
    sprockets.append_path "#{root}/vendor/assets/fonts"

    @clients = Set[]

    @thin = Thin::Server.new(
      "127.0.0.1",
      @opts[:port],
      :signals => false
    ) do
      use LoggingMiddleware

      map '/ws' do
        run WebSocketBridge.new(route_websocket)
      end

      map '/assets' do
        run sprockets
      end

      run Application.new(self)
    end

    @thin.extend ThinLoggingIntegration
    @thin.logger = logger
  end

  def run
    unless @opts[:trace].nil?
      @trace_log = File.open @opts[:trace], "ab"
    end

    EventMachine.run do
      @thin.start

      try_to_connect
    end

    unless @trace_log.nil?
      @trace_log.close
      @trace_log = nil
    end
  end

  def register_can_client(client)
    @clients.add client
  end

  def unregister_can_client(client)
    @clients.delete client
  end

  def connection_ready
    @logger.info "device connected"
    broadcast "device connected\n"
  end

  def connection_closed
    @connection = nil

    logger.error "connection lost"
    broadcast "device lost\n"

    EventMachine.add_timer 5, &method(:try_to_connect)
  end

  def handle_message(message)
    @clients.each do |client|
      client.handle_can message
    end
  end

  def send_message(*args, &block)
    if @connection.nil?
      block.call false if block_given?
    else
      @connection.send_message *args, &block
    end
  end

  private

  def try_to_connect
    logger.info "trying to open device"
    broadcast "opening device\n"

    begin
      @connection = CANInterface::DeviceConnection.open self, @opts[:device], 460800
    rescue => e
      @logger.error e.to_s
      connection_closed
    end
  end

  def broadcast(message)
    @clients.each do |client|
      client.print message
    end
  end

  def route_websocket(req)
    logger.info "WS #{req["PATH_INFO"]}"

    if req["PATH_INFO"] == "/ws/can"
      logger.info "accepted, CAN"

      [ WebSocketCANConnection, self ]
    else
      logger.info "rejected"

      response = Rack::Response.new("Endpoint #{req["PATH_INFO"]} is not defined", 404)
      response["Content-Type"] = "text/plain"

      response.finish
    end
  end
end

require_relative "web_monitor/logging"
require_relative "web_monitor/logging_middleware"
require_relative "web_monitor/application"
require_relative "web_monitor/web_socket_bridge"
require_relative "web_monitor/thin_logging_integration"
require_relative "web_monitor/web_socket_can_connection"
require_relative "command_parser"
require_relative "caninterface"
require_relative "line_splitter"

class WebMonitor
  extend Logging::Loggable

  log_as_facility "server"
end
