/**
 * @file
 *
 * @brief Implementation of notification functions as defined in kdbnotification.h
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 *
 */

#include <kdbassert.h>
#include <kdbease.h>
#include <kdbinternal.h>
#include <kdbioprivate.h>
#include <kdblogger.h>
#include <kdbnotification.h>
#include <kdbnotificationplugin.h>

#include <stdio.h>

/**
 * @internal
 * Retrieves a function exported by a plugin.
 *
 * @param  plugin Plugin handle
 * @param  name   Function name
 * @return        Pointer to function
 */
static size_t getPluginFunction (Plugin * plugin, const char * name)
{
	KeySet * exports = ksNew (0, KS_END);
	Key * pk = keyNew ("system/elektra/modules", KEY_END);
	keyAddBaseName (pk, plugin->name);
	plugin->kdbGet (plugin, exports, pk);
	ksRewind (exports);
	keyAddBaseName (pk, "exports");
	keyAddBaseName (pk, name);
	Key * keyFunction = ksLookup (exports, pk, 0);
	if (!keyFunction)
	{
		ELEKTRA_LOG_DEBUG ("function \"%s\" from plugin \"%s\" not found", name, plugin->name);
		ksDel (exports);
		keyDel (pk);
		return 0;
	}

	size_t * buffer;
	size_t bufferSize = keyGetValueSize (keyFunction);
	buffer = elektraMalloc (bufferSize);
	if (buffer)
	{
		int result = keyGetBinary (keyFunction, buffer, bufferSize);
		if (result == -1 || buffer == NULL)
		{
			ELEKTRA_LOG_WARNING ("could not get function \"%s\" from plugin \"%s\"", name, plugin->name);
			return 0;
		}
	}

	size_t func = *buffer;

	elektraFree (buffer);
	ksDel (exports);
	keyDel (pk);

	return func;
}

/**
 * @internal
 * Converts a placement name to an index in the globalPlugins array of
 * the internal KDB structure.
 *
 * @param  placement Placement name
 * @return           Placement index or -1 on unknown placement name
 */
int placementToPosition (char * placement)
{
	if (strcmp (placement, "prerollback") == 0)
	{
		return PREROLLBACK;
	}
	else if (strcmp (placement, "rollback") == 0)
	{
		return ROLLBACK;
	}
	else if (strcmp (placement, "postrollback") == 0)
	{
		return POSTROLLBACK;
	}
	else if (strcmp (placement, "getresolver") == 0)
	{
		return GETRESOLVER;
	}
	else if (strcmp (placement, "pregetstorage") == 0)
	{
		return PREGETSTORAGE;
	}
	else if (strcmp (placement, "getstorage") == 0)
	{
		return GETSTORAGE;
	}
	else if (strcmp (placement, "postgetstorage") == 0)
	{
		return POSTGETSTORAGE;
	}
	else if (strcmp (placement, "setresolver") == 0)
	{
		return SETRESOLVER;
	}
	else if (strcmp (placement, "postgetcleanup") == 0)
	{
		return POSTGETCLEANUP;
	}
	else if (strcmp (placement, "presetstorage") == 0)
	{
		return PRESETSTORAGE;
	}
	else if (strcmp (placement, "setstorage") == 0)
	{
		return SETSTORAGE;
	}
	else if (strcmp (placement, "presetstorage") == 0)
	{
		return PRESETSTORAGE;
	}
	else if (strcmp (placement, "setstorage") == 0)
	{
		return SETSTORAGE;
	}
	else if (strcmp (placement, "presetcleanup") == 0)
	{
		return PRESETCLEANUP;
	}
	else if (strcmp (placement, "precommit") == 0)
	{
		return PRECOMMIT;
	}
	else if (strcmp (placement, "commit") == 0)
	{
		return COMMIT;
	}
	else if (strcmp (placement, "postcommit") == 0)
	{
		return POSTCOMMIT;
	}
	else
	{
		ELEKTRA_LOG_WARNING ("unknown placement name \"%s\"", placement);
		return -1;
	}
}

/**
 * @internal
 * Convert plament name to list plugin's position type.
 *
 * The list plugin distinguishes between three position types: get, set & error.
 * Plament names are converted to one of these position types.
 *
 * @param  placement Placement name
 * @return           Placement type for list plugin or NULL on unknown placement name
 */
static char * placementToListPositionType (char * placement)
{
	if (strcmp (placement, "prerollback") == 0)
	{
		return "error";
	}
	else if (strcmp (placement, "rollback") == 0)
	{
		return "error";
	}
	else if (strcmp (placement, "postrollback") == 0)
	{
		return "error";
	}
	else if (strcmp (placement, "getresolver") == 0)
	{
		return "get";
	}
	else if (strcmp (placement, "pregetstorage") == 0)
	{
		return "get";
	}
	else if (strcmp (placement, "getstorage") == 0)
	{
		return "get";
	}
	else if (strcmp (placement, "postgetstorage") == 0)
	{
		return "get";
	}
	else if (strcmp (placement, "setresolver") == 0)
	{
		return "set";
	}
	else if (strcmp (placement, "postgetcleanup") == 0)
	{
		return "get";
	}
	else if (strcmp (placement, "presetstorage") == 0)
	{
		return "set";
	}
	else if (strcmp (placement, "setstorage") == 0)
	{
		return "set";
	}
	else if (strcmp (placement, "presetstorage") == 0)
	{
		return "set";
	}
	else if (strcmp (placement, "setstorage") == 0)
	{
		return "set";
	}
	else if (strcmp (placement, "presetcleanup") == 0)
	{
		return "set";
	}
	else if (strcmp (placement, "precommit") == 0)
	{
		return "set";
	}
	else if (strcmp (placement, "commit") == 0)
	{
		return "set";
	}
	else if (strcmp (placement, "postcommit") == 0)
	{
		return "set";
	}
	else
	{
		ELEKTRA_LOG_WARNING ("unknown placement name \"%s\"", placement);
		return NULL;
	}
}

/**
 * @internal
 * Load plugin by name.
 *
 * Uses module cache from KDB handle.
 * The plugin only needs to be closed after use.
 *
 * @param  kdb    KDB handle
 * @param  name   Plugin name
 * @param  config Plugin configuration
 * @return     Plugin handle or NULL on error
 */
static Plugin * loadPlugin (KDB * kdb, char * name, KeySet * config)
{
	// Load required plugin
	Key * errorKey = keyNew (0);
	KeySet * moduleCache = kdb->modules; // use kdb module cache
	Plugin * plugin = elektraPluginOpen (name, moduleCache, config, errorKey);

	int hasError = keyGetMeta (errorKey, "error") != NULL;
	keyDel (errorKey);

	if (!plugin || hasError)
	{
		ELEKTRA_LOG_WARNING ("elektraPluginOpen failed!\n");
		return NULL;
	}

	return plugin;
}

/**
 * @internal
 * Unload plugin given by plugin handle.
 *
 * @return     Plugin handle or NULL on error
 */

/**
 * @internal
 * Unload plugin by plugin handle.
 *
 * @param  plugin Plugin handle
 * @retval 0 on error
 * @retval 1 on success
 */
static int unloadPlugin (Plugin * plugin)
{
	Key * errorKey = keyNew (0);
	int result = elektraPluginClose (plugin, errorKey);

	int hasError = keyGetMeta (errorKey, "error") != NULL;
	keyDel (errorKey);

	if (!result || hasError)
	{
		ELEKTRA_LOG_WARNING ("elektraPluginClose failed result=%d", result);
		return 0;
	}
	else
	{
		return 1;
	}
}

/**
 * @internal
 * Read placement list from plugin.
 *
 * The returned string needs to be freed.
 *
 * @param  plugin Plugin
 * @return        Space separated list of placement names
 */
static char * getPluginPlacementList (Plugin * plugin)
{
	// Get placements from plugin
	Key * pluginInfo = keyNew ("system/elektra/modules/", KEY_END);
	keyAddBaseName (pluginInfo, plugin->name);
	KeySet * ksResult = ksNew (0, KS_END);
	plugin->kdbGet (plugin, ksResult, pluginInfo);

	Key * placementsKey = keyDup (pluginInfo);
	keyAddBaseName (placementsKey, "infos");
	keyAddBaseName (placementsKey, "placements");
	Key * placements = ksLookup (ksResult, placementsKey, 0);
	if (placements == NULL)
	{
		ELEKTRA_LOG_WARNING ("could not read placements from plugin");
		return 0;
	}
	char * placementList = strdup (keyString (placements));

	keyDel (pluginInfo);
	keyDel (placementsKey);
	ksDel (ksResult);

	return placementList;
}

/**
 * @internal
 * Add plugin at placement to list plugin configuration and apply it.
 *
 * @param  list      List plugin
 * @param  plugin    Plugin to add
 * @param  placement Placement name
 * @retval 0 on error
 * @retval 0 on success
 */
static int listAddPlugin (Plugin * list, Plugin * plugin, char * placement)
{
	KeySet * newConfig = ksDup (list->config);

	// Find name for next item in plugins array
	Key * configBase = keyNew ("user/plugins", KEY_END);
	KeySet * array = elektraArrayGet (configBase, newConfig);
	Key * pluginItem = elektraArrayGetNextKey (array);
	ELEKTRA_NOT_NULL (pluginItem);
	keySetString (pluginItem, plugin->name);
	keyDel (configBase);

	// Create key with plugin handle
	Key * pluginHandle = keyDup (pluginItem);
	keyAddName (pluginHandle, "handle");
	keySetBinary (pluginHandle, &plugin, sizeof (plugin));

	// Create key with plugin placement
	char * placementType = placementToListPositionType (placement);
	if (placementType == NULL)
	{
		keyDel (configBase);
		keyDel (pluginItem);
		keyDel (pluginHandle);
		return 0;
	}
	Key * pluginPlacements = keyDup (pluginItem);
	keyAddName (pluginPlacements, "placements/");
	keyAddName (pluginPlacements, placementType);
	keySetString (pluginPlacements, placement);

	// Append keys to list plugin configuration
	ksAppendKey (newConfig, pluginItem);
	ksAppendKey (newConfig, pluginHandle);
	ksAppendKey (newConfig, pluginPlacements);

	ksDel (array);
	ksDel (list->config);

	// Apply new configuration
	list->config = newConfig;
	list->kdbOpen (list, NULL);

	return 1;
}

/**
 * @internal
 * Create a new key with a different root or common name.
 *
 * Does not modify `key`. The new key needs to be freed after usage.
 *
 * Preconditions: The key name starts with `source`.
 *
 * Example:
 * ```
 * Key * source = keyNew("user/plugins/foo/placements/get", KEY_END);
 * Key * dest = renameKey ("user/plugins/foo", "user/plugins/bar", source);
 * succeed_if_same_string (keyName(dest), "user/plugins/bar/placements/get");
 * ```
 *
 *
 * @param  source Part of the key name to replace
 * @param  dest   Replaces `source`
 * @param  key    key
 * @return        key with new name
 */
static Key * renameKey (const char * source, const char * dest, Key * key)
{
	const char * name = keyName (key);
	char * baseKeyNames = strndup (name + strlen (source), strlen (name));

	Key * moved = keyDup (key);
	keySetName (moved, dest);
	keyAddName (moved, baseKeyNames);

	elektraFree (baseKeyNames);

	return moved;
}

/**
 * @internal
 * Recursively move all keys in keyset below source to dest.
 *
 * Modifies the keyset.
 *
 * Example:
 * ```
 * moveKeysRecursive("user/plugins/#0", "user/plugins/#1", config);
 * ```
 *
 * @param source Root part to replace
 * @param dest   Destination for keys
 * @param keyset keyset
 */
static void moveKeysRecursive (const char * source, const char * dest, KeySet * keyset)
{
	Key * sourceBaseKey = keyNew (source, KEY_END);
	KeySet * newKeys = ksNew (0, KS_END);

	// Rename keys in keyset
	Key * sourceKey;
	ksRewind (keyset);
	while ((sourceKey = ksNext (keyset)) != NULL)
	{
		// Rename all keys below sourceKey
		if (!keyIsBelowOrSame (sourceBaseKey, sourceKey)) continue;
		Key * destKey = renameKey (source, dest, sourceKey);
		ksAppendKey (newKeys, destKey);
	}

	// Remove source keys from keyset
	KeySet * cut = ksCut (keyset, sourceBaseKey);
	ksDel (cut);

	ksAppend (keyset, newKeys);
	ksDel (newKeys);

	keyDel (sourceBaseKey);
}

/**
 * @internal
 * Remove plugin at all placements from list plugin configuration and apply it.
 *
 * @param  list   List plugin
 * @param  plugin Plugin to remove
 * @retval 0 on error
 * @retval 1 on success
 */
static int listRemovePlugin (Plugin * list, Plugin * plugin)
{
	KeySet * newConfig = ksDup (list->config);

	Key * configBase = keyNew ("user/plugins", KEY_END);
	KeySet * array = elektraArrayGet (configBase, newConfig);

	// Find the plugin with our handle
	Key * current;
	ksRewind (array);
	while ((current = ksNext (array)) != NULL)
	{
		Key * handleLookup = keyDup (current);
		keyAddBaseName (handleLookup, "handle");
		Key * handle = ksLookup (newConfig, handleLookup, 0);
		keyDel (handleLookup);

		if (handle)
		{
			Plugin * handleValue = (*(Plugin **)keyValue (handle));
			if (handleValue == plugin)
			{
				// Remove plugin configuration
				KeySet * cut = ksCut (newConfig, current);
				ksDel (cut);
			}
		}
	}
	ksDel (array);

	// Renumber array items
	KeySet * sourceArray = elektraArrayGet (configBase, newConfig);
	Key * renumberBase = keyNew ("user/plugins/#", KEY_END);
	ksRewind (sourceArray);
	while ((current = ksNext (sourceArray)) != NULL)
	{
		// Create new array item base name e.g. "user/plugins/#0"
		elektraArrayIncName (renumberBase);
		moveKeysRecursive (keyName (current), keyName (renumberBase), newConfig);
	}

	keyDel (configBase);
	keyDel (renumberBase);
	ksDel (sourceArray);
	ksDel (list->config);

	// Apply new configuration
	list->config = newConfig;
	list->kdbOpen (list, NULL);

	return 1;
}

/**
 * @internal
 * Global mount given plugin at run-time.
 *
 * Reads placements from the plugin directly inserts the plugin.
 * Also supports adding itself to the list plugin at run-time if present
 * at requested global placement.
 *
 * @param  kdb    KDB handle
 * @param  plugin Plugin handle
 * @retval 0 on errors
 * @retval 1 on success
 */
static int mountGlobalPlugin (KDB * kdb, Plugin * plugin)
{
	char * placementList = getPluginPlacementList (plugin);

	// Parse plament list (contains placements from README.md seperated by whitespace)
	char * placement = strtok (placementList, " ");
	while (placement != NULL)
	{
		// Convert placement name to internal index
		int placementIndex = placementToPosition (placement);
		if (placementIndex == -1)
		{
			elektraFree (placementList);
			return 0;
		}

		if (kdb->globalPlugins[placementIndex][MAXONCE] == NULL)
		{
			// Insert directly as global plugin
			kdb->globalPlugins[placementIndex][MAXONCE] = plugin;
		}
		else
		{
			Plugin * pluginAtPlacement = kdb->globalPlugins[placementIndex][MAXONCE];
			// Add plugin to list plugin
			if (strcmp (pluginAtPlacement->name, "list") == 0)
			{
				ELEKTRA_LOG_DEBUG ("required position %s/maxonce taken by list plugin, adding plugin", placement);
				int result = listAddPlugin (pluginAtPlacement, plugin, placement);
				if (!result)
				{
					ELEKTRA_LOG_WARNING ("could not add plugin to list plugin at position %s/maxonce", placement);
					elektraFree (placementList);
					return 0;
				}
			}
			else
			{
				printf ("mountGlobalPlugin: required position %s/maxonce taken by plugin %s, skipping!\n", placement,
					pluginAtPlacement->name);
				// cannot manually add list module here as configuration is broken.
				// the list module needs to be mounted in every position to keep track
				// of the current position
				ELEKTRA_LOG_WARNING ("required position %s/maxonce taken by plugin %s, aborting!", placement,
						     pluginAtPlacement->name);
				elektraFree (placementList);
				return 0;
			}
		}

		// Process next placement in list
		placement = strtok (NULL, " ");
	}

	elektraFree (placementList);

	return 1;
}

/**
 * @internal
 * Unmount global plugin at run-time.
 *
 * Removes a plugin at all placements.
 * Undos `mountGlobalPlugin()`.
 *
 * @param  kdb    KDB handle
 * @param  plugin Plugin handle
 * @retval 0 on errors
 * @retval 1 on success
 */
int unmountGlobalPlugin (KDB * kdb, Plugin * plugin)
{
	char * placementList = getPluginPlacementList (plugin);

	// Parse plament list (contains placements from README.md seperated by whitespace)
	char * placement = strtok (placementList, " ");
	while (placement != NULL)
	{
		// Convert placement name to internal index
		int placementIndex = placementToPosition (placement);
		if (placementIndex == -1)
		{
			elektraFree (placementList);
			return 0;
		}

		if (kdb->globalPlugins[placementIndex][MAXONCE] == plugin)
		{
			// Remove from direct placement as global plugin
			kdb->globalPlugins[placementIndex][MAXONCE] = NULL;
		}
		else
		{
			Plugin * pluginAtPlacement = kdb->globalPlugins[placementIndex][MAXONCE];
			// Add plugin to list plugin
			if (strcmp (pluginAtPlacement->name, "list") == 0)
			{
				ELEKTRA_LOG_DEBUG ("required position %s/maxonce taken by list plugin, removing plugin", placement);
				int result = listRemovePlugin (pluginAtPlacement, plugin);
				if (!result)
				{
					ELEKTRA_LOG_WARNING ("could not remove plugin from list plugin at position %s/maxonce", placement);
					elektraFree (placementList);
					return 0;
				}
			}
			else
			{
				ELEKTRA_LOG_WARNING ("required position %s/maxonce taken by plugin %s, should be either list or plugin!",
						     placement, pluginAtPlacement->name);
			}
		}

		// Process next placement in list
		placement = strtok (NULL, " ");
	}

	elektraFree (placementList);

	return 1;
}

int elektraNotificationOpen (KDB * kdb)
{
	// Allow open only once
	if (kdb->notificationPlugin)
	{
		return 0;
	}

	Plugin * notificationPlugin = loadPlugin (kdb, "internalnotification", NULL);
	if (!notificationPlugin)
	{
		return 0;
	}

	int mountResult = mountGlobalPlugin (kdb, notificationPlugin);
	if (!mountResult)
	{
		Key * errorKey = keyNew (0);
		elektraPluginClose (notificationPlugin, errorKey);
		keyDel (errorKey);
		return 0;
	}

	kdb->notificationPlugin = notificationPlugin;

	return 1;
}

int elektraNotificationClose (KDB * kdb)
{
	if (!kdb->notificationPlugin)
	{
		return 0;
	}

	Plugin * notificationPlugin = kdb->notificationPlugin;

	// Unmount the plugin
	int result = unmountGlobalPlugin (kdb, notificationPlugin);
	if (!result)
	{
		ELEKTRA_LOG_WARNING ("unmountGlobalPlugin failed");
		return 0;
	}

	// Unload the notification plugin
	result = unloadPlugin (notificationPlugin);
	if (!result)
	{
		ELEKTRA_LOG_WARNING ("elektraPluginClose failed result=%d", result);
		return 0;
	}

	kdb->notificationPlugin = NULL;
	return 1;
}

/**
 * @internal
 * Get notification plugin from kdb.
 *
 * @param  kdb KDB handle
 * @return     Notification plugin handle or NULL if not present
 */
static Plugin * getNotificationPlugin (KDB * kdb)
{
	if (kdb->notificationPlugin)
	{
		return kdb->notificationPlugin;
	}
	else
	{
		ELEKTRA_LOG_WARNING (
			"notificationPlugin not set. use elektraNotificationOpen before calling other elektraNotification-functions");
		return NULL;
	}
}

int elektraNotificationRegisterInt (KDB * kdb, Key * key, int * variable)
{
	// Find notification plugin
	Plugin * notificationPlugin = getNotificationPlugin (kdb);
	if (!notificationPlugin)
	{
		return 0;
	}

	// Get register function from plugin
	size_t func = getPluginFunction (notificationPlugin, "registerInt");
	if (!func)
	{
		return 0;
	}

	// Call register function
	ElektraNotificationPluginRegisterInt registerFunc = (ElektraNotificationPluginRegisterInt)func;
	return registerFunc (notificationPlugin, key, variable);
}

int elektraNotificationRegisterCallback (KDB * kdb, Key * key, ElektraNotificationChangeCallback callback)
{
	// Find notification plugin
	Plugin * notificationPlugin = getNotificationPlugin (kdb);
	if (!notificationPlugin)
	{
		return 0;
	}

	// Get register function from plugin
	size_t func = getPluginFunction (notificationPlugin, "registerCallback");
	if (!func)
	{
		return 0;
	}

	// Call register function
	ElektraNotificationPluginRegisterCallback registerFunc = (ElektraNotificationPluginRegisterCallback)func;
	return registerFunc (notificationPlugin, key, callback);
}
