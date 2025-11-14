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
## ` ipc `

