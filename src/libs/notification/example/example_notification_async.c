/**
 * @file
 *
 * @brief Implementation of notification functions as defined in kdbnotification.h
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 *
 */

#include <kdb.h>
#include <kdbhelper.h>       // elektraFree
#include <kdbio.h>	   // I/O binding functions (elektraIo*)
#include <kdbio_uv.h>	// I/O binding constructor for uv (elektraIoUvNew)
#include <kdbnotification.h> // notification functions

#include <uv.h> // uv functions

#include <signal.h> // signal()
#include <stdio.h>  // printf() & co

// from https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_BLUE "\x1b[34m"

static void setTerminalColor (Key * color, void * context ELEKTRA_UNUSED)
{
	const char * value = keyString (color);
	printf ("Callback called. Changing color to %s\n", value);

	if (elektraStrCmp (value, "red") == 0)
	{
		printf (ANSI_COLOR_RED);
	}
	else if (elektraStrCmp (value, "green") == 0)
	{
		printf (ANSI_COLOR_GREEN);
	}
	else if (elektraStrCmp (value, "blue") == 0)
	{
		printf (ANSI_COLOR_BLUE);
	}
	else
	{
		printf ("Specified color (%s) did not match \"red\", \"green\" or \"blue\". Using default color.\n", value);
		printf (ANSI_COLOR_RESET);
	}
}

static void resetTerminalColor (void)
{
	printf (ANSI_COLOR_RESET "\n");
}

static void onSIGNAL (int signal)
{
	if (signal == SIGINT)
	{
		uv_stop (uv_default_loop ());
	}
}

static void printVariable (ElektraIoTimerOperation * timer)
{
	int value = *(int *)elektraIoTimerGetData (timer);
	printf ("\nMy integer value is %d\n", value);
}

int main (void)
{
	// Cleanup on SIGINT
	signal (SIGINT, onSIGNAL);
	signal (SIGQUIT, onSIGNAL);

	KeySet * config = ksNew (20, KS_END);

	Key * key = keyNew ("/sw/tests/example_notification/#0/current", KEY_END);
	KDB * kdb = kdbOpen (key);
	if (kdb == NULL)
	{
		printf ("could not open KDB. aborting\n");
		return -1;
	}

	uv_loop_t * loop = uv_default_loop ();
	ElektraIoInterface * binding = elektraIoUvNew (loop);
	elektraIoSetBinding (kdb, binding);

	int result = elektraNotificationOpen (kdb);
	if (!result)
	{
		printf ("could init notification. aborting\n");
		return -1;
	}

	int value = 0;
	Key * intKeyToWatch = keyNew ("/sw/tests/example_notification/#0/current/value", KEY_END);
	result = elektraNotificationRegisterInt (kdb, intKeyToWatch, &value);
	if (!result)
	{
		printf ("could not register variable. aborting\n");
		return -1;
	}

	Key * callbackKeyToWatch = keyNew ("/sw/tests/example_notification/#0/current/color", KEY_END);
	result = elektraNotificationRegisterCallback (kdb, callbackKeyToWatch, &setTerminalColor, NULL);
	if (!result)
	{
		printf ("could not register callback. aborting!");
		return -1;
	}

	// Setup timer that repeatedly prints the variable
	ElektraIoTimerOperation * timer = elektraIoNewTimerOperation (2000, 1, printVariable, &value);
	elektraIoBindingAddTimer (binding, timer);

	kdbGet (kdb, config, key);

	printf ("Asynchronous Notification Example Application\n");
	printf ("- Set \"%s\" to red, blue or green to change the text color\n", keyName (callbackKeyToWatch));
	printf ("- Set \"%s\" to any integer value\n", keyName (intKeyToWatch));
	printf ("Send SIGINT (Ctl+C) to exit.\n\n");

	uv_run (loop, UV_RUN_DEFAULT);

	// Cleanup
	resetTerminalColor ();
	elektraIoBindingRemoveTimer (timer);
	elektraFree (timer);
	elektraNotificationClose (kdb);
	kdbClose (kdb, key);

	elektraIoBindingCleanup (binding);
	uv_run (loop, UV_RUN_ONCE); // allow cleanup
#ifdef HAVE_LIBUV1
	uv_loop_close (uv_default_loop ());
#elif HAVE_LIBUV0
	uv_loop_delete (uv_default_loop ());
#endif

	ksDel (config);
	keyDel (intKeyToWatch);
	keyDel (callbackKeyToWatch);
	keyDel (key);
	printf ("cleanup done!\n");
}
