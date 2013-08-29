class Engine
  constructor: ->
    console.log 'Initializing application'

    @$connStatus = document.getElementById 'connection-status-label'
    @$primaryContainer = document.getElementById 'primary-container'
    @_systemLog = @_createLogWindow "Debug messages"

    document.getElementById('transmit-form').addEventListener 'submit', (e) =>
      e.preventDefault()
      @_handleTransmit e.target

    document.getElementById('mode-form').addEventListener 'submit', (e) =>
      e.preventDefault()
      @_handleMode e.target

    document.getElementById('ping-form').addEventListener 'submit', (e) =>
      e.preventDefault()
      @_sendPing()

    @_connected = false
    @_panels = {}
    @_attemptConnection()

  _attemptConnection: ->
    @_printPanel @_systemLog, "trying to connect\n"
    @_socket = new WebSocket("ws://127.0.0.1:7000/ws/can")
    @_socket.onopen = @_wsOpen.bind(this)
    @_socket.onclose = @_wsClose.bind(this)
    @_socket.onmessage = @_wsMessage.bind(this)

  _wsOpen: ->
    @_printPanel @_systemLog, "connected\n"
    @_setConnectionStatus true

  _wsClose: ->
    if @_connected
      @_printPanel @_systemLog, "disconnected\n"
      @_setConnectionStatus false

    delete @_socket
    setTimeout @_attemptConnection.bind(this), 5000

  _wsMessage: (message) ->
    message = JSON.parse message.data
    console.log message

    switch message.action
      when "debug"
        @_printPanel @_systemLog, message.message

      when "can_message"
        panel = @_panels[message.address]
        unless panel?
          panel = @_createLogWindow message.address
          @_panels[message.address] = panel

        @_printPanel panel, "[ " + message.data + "]\n"

  _setConnectionStatus: (connected) ->
    @_connected = connected

    if connected
      @$connStatus.className = "label label-success"
      @$connStatus.innerText = "connected"
    else
      @$connStatus.className = "label label-danger"
      @$connStatus.innerText = "not connected"  

  _handleTransmit: (form) ->
    if @_connected
      @_socket.send JSON.stringify
        action: "transmit"
        message: form.message.value

      form.message.value = ""

  _handleMode: (form) ->
    if @_connected
      @_socket.send JSON.stringify
        action: "mode"
        enabled: form.enabled.checked
        loopback: form.loopback.checked
        silent: form.silent.checked

  _sendPing: ->
    if @_connected
      @_socket.send JSON.stringify
        action: "ping"

  _printPanel: (panel, message) ->
    date = new Date

    pad = (number, digits) ->
      str = number.toString()

      while str.length < digits
        str = "0" + str

      str

    stamp = pad(date.getHours(), 2) + ":" +
            pad(date.getMinutes(), 2) + ":" +
            pad(date.getSeconds(), 2) + "." +
            pad(date.getMilliseconds(), 3) + " "

    panel.body.appendChild @_text(stamp + message)

    panel.body.scrollTop = panel.body.scrollHeight

    @_flashPanel panel

  _flashPanel: (panel) ->
    if panel.flashTimeout?
      clearTimeout panel.flashTimeout
    else
      panel.panel.className = "panel panel-danger"

    callback = ->
      delete panel.flashTimeout
      panel.panel.className = "panel panel-default"

    panel.flashTimeout = setTimeout callback, 1000

  _createLogWindow: (title) ->
    title_node = @_create "h3", className: "panel-title"
    title_node.appendChild @_text(title)

    heading = @_create "div", className: "panel-heading"
    heading.appendChild title_node

    body_node = @_create "div", className: "panel-body debug-list"

    panel = @_create "div", className: "panel panel-default"
    panel.appendChild heading
    panel.appendChild body_node

    column = @_create "div", className: "col-md-6"
    column.appendChild panel

    if @$fillingRow?
      @$fillingRow.appendChild column
      delete @$fillingRow
    else
      row = @_create "div", className: "row"
      row.appendChild column
      @$fillingRow = row
      @$primaryContainer.appendChild row

    {
      panel: panel,
      body: body_node
    }

  _create: (element, attributes) ->
    node = document.createElement element

    for name, value of attributes
      node[name] = value

    node

  _text: (text) ->
    document.createTextNode text

$ ->
  new Engine
