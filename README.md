# Libqril

Libqril is a high-level library which encompasses event handling, async, service
discovery, and state management for QMI messages. It is built on top of Bjorn
Anderssons [QRTR](https://github.com/andersson/qrtr) and
[QMIC](https://github.com/andersson/qmic) projects (with some additions). It's
designed to offer a simple high level interface for communication with Qualcomm
DSPs like the modem found on devices like the OnePlus 6 which use QRTR/QMI.

## Status

The primary user and motivation for this project is
[qrild](https://github.com/aospm/qrild). Please refer to the README there for
more information.

Refer to [libqril.h](/lib/libqril.h) for the API definition, and see
[examples](/examples/) for example usage.

## Dependencies

Libqril needs to be built against [this fork of
QRTR](https://github.com/aospm/qrtr) and [this fork of
QMIC](https://github.com/aospm/qmic).
