<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [SysAdm Qt Client](#sysadm-qt-client)
    - [Required Qt Modules](#required-qt-modules)
    - [Building SysAdm Qt Client](#building-sysadm-qt-client)
    - [Starting SysAdm Client](#starting-sysadm-client)
    - [Websockets API Documentation](#websockets-api-documentation)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# SysAdm Qt Client

Official repo for TrueOS' sysadm WebSocket Client

This multi-platform client is designed to work with [TrueOS' sysadm server](https://github.com/trueos/sysadm) <br />
for administration of Free/TrueOS systems

### Required Qt Modules

```
Qt5 Core (pkg install qt5-core)
Qt5 Gui (pkg install qt5-gui)
Qt5 Concurrent (pkg install qt5-concurrent)
Qt5 Websockets (pkg install qt5-websockets)
```

### Building SysAdm Qt Client

```
% git clone https://github.com/trueos/sysadm-ui-qt.git
% cd sysadm/src-qt5
% /usr/local/lib/qt5/bin/qmake -recursive
% make && sudo make install
```

### Starting SysAdm Client

```
(For GUI interface)
% gui_client/sysadm-client

(For CLI interface)
% cli_client/sysadm-client
```

### Websockets API Documentation

https://api.pcbsd.org

