#ifndef __ELEKTAR_IO_DBUS_ADAPTER_H__
#define __ELEKTAR_IO_DBUS_ADAPTER_H__

#include <dbus/dbus.h>
#include <stdlib.h>
#include <string.h>

#include <kdbio.h>

int elektraIoDbusAdapterAttach (DBusConnection * connection, ElektraIoInterface * ioBinding);

#endif
