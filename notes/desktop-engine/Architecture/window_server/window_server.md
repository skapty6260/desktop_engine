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

