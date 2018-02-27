- infos = Plugin for internal notification
- infos/author = Thomas Wahringer <waht@libelektra.org>
- infos/licence = BSD
- infos/needs =
- infos/provides =
- infos/recommends =
- infos/placements = postgetstorage postcommit
- infos/status = global libc unittest concept unfinished experimental
- infos/metadata =
- infos/description = Plugin for internal notification

## Usage

Allows applications to automatically update registered variables when the value
of a specified key has changed.

It is recommended to use the notification wrapper (see
[notification tutorial](https://www.libelektra.org/tutorials/notifications)) instead of this plugin
directly.
The wrapper has a simple API and decouples applications from the actual plugin.

## Exported Methods

This plugin exports the following functions. The functions addresses are
exported below `system/elektra/modules/internalnotification/exports/`.

All functions have a similar signature:

```C
int registerX (Plugin * handle, Key * key, ...);
```

If the given `key` is contained in a KeySet on a kdbGet or kdbSet operation a
action according to the function's description is executed.
Cascading keys as `key` names are also supported.

*Parameters*

- *handle* The internal plugin `handle` is exported as  		 	
    `system/elektra/modules/internalnotification/exports/handle`.
- *key* Key to watch for changes.

*Returns*

1 on success, 0 otherwise

Please note that the plugin API may change as this plugin is experimental.

### int registerInt (Plugin * handle, Key * key, int * variable)

The key's value is converted to integer and the registered variable is updated
with the new value.

*Additional Parameters*

- *variable* Pointer to the variable

### int registerCallback (Plugin * handle, Key * key, ElektraNotificationChangeCallback callback)

When the key changes the callback is called with the new key.

*Additional Parameters*

- *callback* Callback function with the signature
    `void (*ElektraNotificationChangeCallback) (Key * key)`.
