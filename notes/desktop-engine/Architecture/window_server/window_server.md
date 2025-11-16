**Description**: 
Stores information about windows, send it to connected modules, get changes from this modules and apply to windows.

Contains:
Wayland server, XWayland compatibility layer, IPC communication bridge to receive and send data.
# Folder Architecture
## ` src `
**Description** main logic, args parser, wayland server implementation
1. server.c (Wayland server methods implementation)
2. xwayland.c (XWayland layer implementation)
3. main.c
## ` logger `
**Description** advanced async logging. Mark proccesses: IPC or wayland server logs, formatting, time, colors, log to file, config file for logger configuration. See examples/logger.conf. for help run window_server module with args `--log-help`
1. logger.h (Logger header file)
2. logger.c (Logger implementation)
3. logger_config.h (Logger configuration header)
4. logger_config.c (Logger configuration methods: find file, edit, parse)
## ` ipc `


# Structures
## `struct server` 
```
{
	1. wl_display display - wayland display instance
	2. wl_list clients - list of running wayland clients (windows)
	3. wl_list outputs - list of virtual outputs
}
```

### server->outputs
Outputs defining on server startup. We should run renderer before wayland server, get physical monitors and pass it's values to the wayland server, or we should use config file for wayland server that will store physical output's inside. And we can edit this file via ipc from the renderer too, or pass it as we defined with hands.

**Problems**
Hot-plug. Physical monitors can be plugged in and off in work.
Window resizing. When remote renderer is not on khr_swapchain, but on glfw window or wayland window, we should watch for window resizing.

