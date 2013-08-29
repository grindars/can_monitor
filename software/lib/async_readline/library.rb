class AsyncReadline
  module Library
    extend FFI::Library
    ffi_lib 'readline'

    callback :vcpfunc, [ :string ], :void
    attach_function :rl_callback_handler_install, [ :string, :vcpfunc ], :void
    attach_function :rl_callback_read_char, [ ], :void
    attach_function :rl_callback_handler_remove, [ ], :void
    attach_function :rl_forced_update_display, [ ], :void
  end
end
