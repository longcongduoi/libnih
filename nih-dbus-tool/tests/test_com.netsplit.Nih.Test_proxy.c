/* libnih
 *
 * test_com.netsplit.Nih.Test_proxy.c - test suite for auto-generated
 * proxy bindings.
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <nih/test.h>
#include <nih-dbus/test_dbus.h>

#include <dbus/dbus.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/signal.h>
#include <nih/main.h>
#include <nih/error.h>

#include <nih-dbus/dbus_connection.h>
#include <nih-dbus/dbus_object.h>
#include <nih-dbus/dbus_proxy.h>
#include <nih-dbus/dbus_error.h>
#include <nih-dbus/errors.h>

#include "com.netsplit.Nih.Test_proxy.h"
#include "com.netsplit.Nih.Test_object.h"
#include "com.netsplit.Nih.Test_impl.h"


static int       error_handler_called;
static void *    last_data;
static NihError *last_error;

static void
my_error_handler (void *          data,
		  NihDBusMessage *message)
{
	error_handler_called++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;

	last_error = nih_error_steal ();
}


static int ordinary_method_replied;
const char *last_str_value;

static void
my_ordinary_method_reply (void *          data,
			  NihDBusMessage *message,
			  const char *    output)
{
	ordinary_method_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_ordinary_method (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_ordinary_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		ordinary_method_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_ordinary_method (proxy,
							   "she needs more of ze punishment",
							   my_ordinary_method_reply,
							   my_error_handler,
							   parent,
							   0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (ordinary_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "she needs more of ze punishment");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		ordinary_method_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_ordinary_method (proxy,
							   "",
							   my_ordinary_method_reply,
							   my_error_handler,
							   parent,
							   0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (ordinary_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.OrdinaryMethod.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		ordinary_method_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_ordinary_method (proxy,
							   "invalid",
							   my_ordinary_method_reply,
							   my_error_handler,
							   parent,
							   0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (ordinary_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		ordinary_method_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_ordinary_method (proxy,
							   "she needs more of ze punishment",
							   my_ordinary_method_reply,
							   my_error_handler,
							   parent,
							   50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (ordinary_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		ordinary_method_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_ordinary_method (proxy,
							   "she needs more of ze punishment",
							   my_ordinary_method_reply,
							   my_error_handler,
							   parent,
							   50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (ordinary_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_ordinary_method_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_ordinary_method_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_ordinary_method_sync (parent, proxy,
						       "she needs more of ze punishment",
						       &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "she needs more of ze punishment");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_ordinary_method_sync (parent, proxy,
						       "",
						       &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.OrdinaryMethod.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_ordinary_method_sync (parent, proxy,
						       "invalid",
						       &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int nameless_method_replied;

static void
my_nameless_method_reply (void *          data,
			  NihDBusMessage *message,
			  const char *    arg2)
{
	nameless_method_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (arg2, NULL);
	TEST_ALLOC_PARENT (arg2, message);

	last_data = data;
	last_str_value = arg2;
	nih_ref (last_str_value, last_data);
}

void
test_nameless_method (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_nameless_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		nameless_method_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_nameless_method (proxy,
							   "she needs more of ze punishment",
							   my_nameless_method_reply,
							   my_error_handler,
							   parent,
							   0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (nameless_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "she needs more of ze punishment");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		nameless_method_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_nameless_method (proxy,
							   "",
							   my_nameless_method_reply,
							   my_error_handler,
							   parent,
							   0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (nameless_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.NamelessMethod.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		nameless_method_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_nameless_method (proxy,
							   "invalid",
							   my_nameless_method_reply,
							   my_error_handler,
							   parent,
							   0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (nameless_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		nameless_method_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_nameless_method (proxy,
							   "she needs more of ze punishment",
							   my_nameless_method_reply,
							   my_error_handler,
							   parent,
							   50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (nameless_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		nameless_method_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_nameless_method (proxy,
							   "she needs more of ze punishment",
							   my_nameless_method_reply,
							   my_error_handler,
							   parent,
							   50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (nameless_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_nameless_method_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_nameless_method_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_nameless_method_sync (parent, proxy,
						       "she needs more of ze punishment",
						       &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "she needs more of ze punishment");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_nameless_method_sync (parent, proxy,
						       "",
						       &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.NamelessMethod.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_nameless_method_sync (parent, proxy,
						       "invalid",
						       &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int async_method_replied;

static void
my_async_method_reply (void *          data,
		       NihDBusMessage *message,
		       const char *    output)
{
	async_method_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_async_method (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_async_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		async_method_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_async_method (proxy,
							"she needs more of ze punishment",
							my_async_method_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		async_method_input = NULL;
		async_method_message = NULL;

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);

		assert (async_method_input != NULL);
		assert (async_method_message != NULL);

		TEST_ALLOC_SAFE {
			assert0 (my_test_async_method_reply (
					 async_method_message,
					 async_method_input));
		}

		nih_free (async_method_message);
		nih_free (async_method_input);

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (async_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "she needs more of ze punishment");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		async_method_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_async_method (proxy,
							"",
							my_async_method_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (async_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.AsyncMethod.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		async_method_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_async_method (proxy,
							"invalid",
							my_async_method_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (async_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		async_method_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_async_method (proxy,
							"she needs more of ze punishment",
							my_async_method_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (async_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		async_method_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_async_method (proxy,
							"she needs more of ze punishment",
							my_async_method_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (async_method_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_async_method_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_async_method_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		async_method_main_loop = TRUE;
		async_method_input = NULL;
		async_method_message = NULL;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_async_method_sync (parent, proxy,
						    "she needs more of ze punishment",
						    &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "she needs more of ze punishment");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_async_method_sync (parent, proxy,
						    "",
						    &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.AsyncMethod.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_async_method_sync (parent, proxy,
						    "invalid",
						    &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int byte_to_str_replied;

static void
my_byte_to_str_reply (void *          data,
		      NihDBusMessage *message,
		      const char *    output)
{
	byte_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_byte_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_byte_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		byte_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_byte_to_str (proxy,
						       97,
						       my_byte_to_str_reply,
						       my_error_handler,
						       parent,
						       0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (byte_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "97");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		byte_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_byte_to_str (proxy,
						       0,
						       my_byte_to_str_reply,
						       my_error_handler,
						       parent,
						       0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (byte_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.ByteToStr.ZeroInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		byte_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_byte_to_str (proxy,
						       4,
						       my_byte_to_str_reply,
						       my_error_handler,
						       parent,
						       0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (byte_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		byte_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_byte_to_str (proxy,
						       97,
						       my_byte_to_str_reply,
						       my_error_handler,
						       parent,
						       50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (byte_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		byte_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_byte_to_str (proxy,
						       97,
						       my_byte_to_str_reply,
						       my_error_handler,
						       parent,
						       50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (byte_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_byte_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_byte_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_byte_to_str_sync (parent, proxy,
						   97,
						   &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "97");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_byte_to_str_sync (parent, proxy,
						   0,
						   &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.ByteToStr.ZeroInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_byte_to_str_sync (parent, proxy,
						   4,
						   &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_byte_replied;
static uint8_t last_byte_value;

static void
my_str_to_byte_reply (void *          data,
		      NihDBusMessage *message,
		      uint8_t         output)
{
	str_to_byte_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_byte_value = output;
}

void
test_str_to_byte (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_byte");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_byte_replied = FALSE;
		last_data = NULL;
		last_byte_value = 0;

		pending_call = proxy_test_str_to_byte (proxy,
						       "97",
						       my_str_to_byte_reply,
						       my_error_handler,
						       parent,
						       0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_byte_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_byte_value, 97);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_byte_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_byte (proxy,
						       "",
						       my_str_to_byte_reply,
						       my_error_handler,
						       parent,
						       0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_byte_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToByte.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_byte_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_byte (proxy,
						       "invalid",
						       my_str_to_byte_reply,
						       my_error_handler,
						       parent,
						       0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_byte_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_byte_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_byte (proxy,
						       "97",
						       my_str_to_byte_reply,
						       my_error_handler,
						       parent,
						       50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_byte_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_byte_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_byte (proxy,
						       "97",
						       my_str_to_byte_reply,
						       my_error_handler,
						       parent,
						       50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_byte_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_byte_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	uint8_t         byte_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_byte_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		byte_value = 0;

		ret = proxy_test_str_to_byte_sync (parent, proxy,
						   "97",
						   &byte_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (byte_value, 97);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_byte_sync (parent, proxy,
						   "",
						   &byte_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToByte.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_byte_sync (parent, proxy,
						   "invalid",
						   &byte_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int boolean_to_str_replied;

static void
my_boolean_to_str_reply (void *          data,
			 NihDBusMessage *message,
			 const char *    output)
{
	boolean_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_boolean_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_boolean_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		boolean_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_boolean_to_str (proxy,
							  TRUE,
							  my_boolean_to_str_reply,
							  my_error_handler,
							  parent,
							  0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (boolean_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "True");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		boolean_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_boolean_to_str (proxy,
							  FALSE,
							  my_boolean_to_str_reply,
							  my_error_handler,
							  parent,
							  0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (boolean_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.BooleanToStr.ZeroInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		boolean_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_boolean_to_str (proxy,
							  TRUE,
							  my_boolean_to_str_reply,
							  my_error_handler,
							  parent,
							  50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (boolean_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		boolean_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_boolean_to_str (proxy,
							  TRUE,
							  my_boolean_to_str_reply,
							  my_error_handler,
							  parent,
							  50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (boolean_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_boolean_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_boolean_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_boolean_to_str_sync (parent, proxy,
						      TRUE,
						      &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "True");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_boolean_to_str_sync (parent, proxy,
						      FALSE,
						      &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.BooleanToStr.ZeroInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_boolean_replied;
static int last_boolean_value;

static void
my_str_to_boolean_reply (void *          data,
			 NihDBusMessage *message,
			 int             output)
{
	str_to_boolean_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_boolean_value = output;
}

void
test_str_to_boolean (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_boolean");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_boolean_replied = FALSE;
		last_data = NULL;
		last_boolean_value = 0;

		pending_call = proxy_test_str_to_boolean (proxy,
							  "True",
							  my_str_to_boolean_reply,
							  my_error_handler,
							  parent,
							  0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_boolean_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_boolean_value, TRUE);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_boolean_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_boolean (proxy,
							  "",
							  my_str_to_boolean_reply,
							  my_error_handler,
							  parent,
							  0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_boolean_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToBoolean.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_boolean_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_boolean (proxy,
							  "invalid",
							  my_str_to_boolean_reply,
							  my_error_handler,
							  parent,
							  0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_boolean_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_boolean_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_boolean (proxy,
							  "True",
							  my_str_to_boolean_reply,
							  my_error_handler,
							  parent,
							  50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_boolean_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_boolean_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_boolean (proxy,
							  "True",
							  my_str_to_boolean_reply,
							  my_error_handler,
							  parent,
							  50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_boolean_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_boolean_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int             boolean_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_boolean_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		boolean_value = 0;

		ret = proxy_test_str_to_boolean_sync (parent, proxy,
						      "True",
						      &boolean_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (boolean_value, TRUE);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_boolean_sync (parent, proxy,
						      "",
						      &boolean_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToBoolean.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_boolean_sync (parent, proxy,
						      "invalid",
						      &boolean_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int int16_to_str_replied;

static void
my_int16_to_str_reply (void *          data,
		       NihDBusMessage *message,
		       const char *    output)
{
	int16_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_int16_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_int16_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int16_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_int16_to_str (proxy,
							-42,
							my_int16_to_str_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (int16_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "-42");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int16_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int16_to_str (proxy,
							0,
							my_int16_to_str_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int16_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int16ToStr.ZeroInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int16_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int16_to_str (proxy,
							4,
							my_int16_to_str_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int16_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int16_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int16_to_str (proxy,
							-42,
							my_int16_to_str_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int16_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int16_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int16_to_str (proxy,
							-42,
							my_int16_to_str_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int16_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_int16_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_int16_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_int16_to_str_sync (parent, proxy,
						    -42,
						    &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "-42");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_int16_to_str_sync (parent, proxy,
						    0,
						    &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int16ToStr.ZeroInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_int16_to_str_sync (parent, proxy,
						    4,
						    &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_int16_replied;
static int16_t last_int16_value;

static void
my_str_to_int16_reply (void *          data,
		       NihDBusMessage *message,
		       int16_t         output)
{
	str_to_int16_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_int16_value = output;
}

void
test_str_to_int16 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_int16");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int16_replied = FALSE;
		last_data = NULL;
		last_int16_value = 0;

		pending_call = proxy_test_str_to_int16 (proxy,
							"-42",
							my_str_to_int16_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_int16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_int16_value, -42);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int16 (proxy,
							"",
							my_str_to_int16_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToInt16.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int16 (proxy,
							"invalid",
							my_str_to_int16_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int16 (proxy,
							"-42",
							my_str_to_int16_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int16 (proxy,
							"-42",
							my_str_to_int16_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_int16_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int16_t         int16_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_int16_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int16_value = 0;

		ret = proxy_test_str_to_int16_sync (parent, proxy,
						    "-42",
						    &int16_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (int16_value, -42);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_int16_sync (parent, proxy,
						    "",
						    &int16_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToInt16.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_int16_sync (parent, proxy,
						    "invalid",
						    &int16_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int uint16_to_str_replied;

static void
my_uint16_to_str_reply (void *          data,
			NihDBusMessage *message,
			const char *    output)
{
	uint16_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_uint16_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_uint16_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint16_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_uint16_to_str (proxy,
							 42,
							 my_uint16_to_str_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (uint16_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "42");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint16_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_uint16_to_str (proxy,
							 0,
							 my_uint16_to_str_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (uint16_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.UInt16ToStr.ZeroInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint16_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_uint16_to_str (proxy,
							 4,
							 my_uint16_to_str_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (uint16_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint16_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_uint16_to_str (proxy,
							 42,
							 my_uint16_to_str_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (uint16_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint16_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_uint16_to_str (proxy,
							 42,
							 my_uint16_to_str_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (uint16_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_uint16_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_uint16_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_uint16_to_str_sync (parent, proxy,
						     42,
						     &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "42");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_uint16_to_str_sync (parent, proxy,
						     0,
						     &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.UInt16ToStr.ZeroInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_uint16_to_str_sync (parent, proxy,
						     4,
						     &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_uint16_replied;
static int16_t last_uint16_value;

static void
my_str_to_uint16_reply (void *          data,
			NihDBusMessage *message,
			uint16_t        output)
{
	str_to_uint16_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_uint16_value = output;
}

void
test_str_to_uint16 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_uint16");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint16_replied = FALSE;
		last_data = NULL;
		last_uint16_value = 0;

		pending_call = proxy_test_str_to_uint16 (proxy,
							 "42",
							 my_str_to_uint16_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_uint16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_uint16_value, 42);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_uint16 (proxy,
							 "",
							 my_str_to_uint16_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_uint16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToUInt16.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_uint16 (proxy,
							 "invalid",
							 my_str_to_uint16_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_uint16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_uint16 (proxy,
							 "42",
							 my_str_to_uint16_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_uint16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_uint16 (proxy,
							 "42",
							 my_str_to_uint16_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_uint16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_uint16_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	uint16_t        uint16_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_uint16_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		uint16_value = 0;

		ret = proxy_test_str_to_uint16_sync (parent, proxy,
						     "42",
						     &uint16_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (uint16_value, 42);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_uint16_sync (parent, proxy,
						     "",
						     &uint16_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToUInt16.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_uint16_sync (parent, proxy,
						     "invalid",
						     &uint16_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int int32_to_str_replied;

static void
my_int32_to_str_reply (void *          data,
		       NihDBusMessage *message,
		       const char *    output)
{
	int32_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_int32_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_int32_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int32_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_int32_to_str (proxy,
							-1048576,
							my_int32_to_str_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (int32_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "-1048576");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int32_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int32_to_str (proxy,
							0,
							my_int32_to_str_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int32_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int32ToStr.ZeroInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int32_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int32_to_str (proxy,
							4,
							my_int32_to_str_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int32_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int32_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int32_to_str (proxy,
							-1048576,
							my_int32_to_str_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int32_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int32_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int32_to_str (proxy,
							-1048576,
							my_int32_to_str_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int32_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_int32_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_int32_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_int32_to_str_sync (parent, proxy,
						    -1048576,
						    &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "-1048576");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_int32_to_str_sync (parent, proxy,
						    0,
						    &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int32ToStr.ZeroInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_int32_to_str_sync (parent, proxy,
						    4,
						    &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_int32_replied;
static int32_t last_int32_value;

static void
my_str_to_int32_reply (void *          data,
		       NihDBusMessage *message,
		       int32_t         output)
{
	str_to_int32_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_int32_value = output;
}

void
test_str_to_int32 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_int32");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_replied = FALSE;
		last_data = NULL;
		last_int32_value = 0;

		pending_call = proxy_test_str_to_int32 (proxy,
							"-1048576",
							my_str_to_int32_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_int32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_int32_value, -1048576);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int32 (proxy,
							"",
							my_str_to_int32_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToInt32.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int32 (proxy,
							"invalid",
							my_str_to_int32_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int32 (proxy,
							"-1048576",
							my_str_to_int32_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int32 (proxy,
							"-1048576",
							my_str_to_int32_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_int32_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int32_t         int32_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_int32_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int32_value = 0;

		ret = proxy_test_str_to_int32_sync (parent, proxy,
						    "-1048576",
						    &int32_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (int32_value, -1048576);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_int32_sync (parent, proxy,
						    "",
						    &int32_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToInt32.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_int32_sync (parent, proxy,
						    "invalid",
						    &int32_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int uint32_to_str_replied;

static void
my_uint32_to_str_reply (void *          data,
			NihDBusMessage *message,
			const char *    output)
{
	uint32_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_uint32_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_uint32_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint32_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_uint32_to_str (proxy,
							 1048576,
							 my_uint32_to_str_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (uint32_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "1048576");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint32_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_uint32_to_str (proxy,
							 0,
							 my_uint32_to_str_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (uint32_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.UInt32ToStr.ZeroInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint32_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_uint32_to_str (proxy,
							 4,
							 my_uint32_to_str_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (uint32_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint32_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_uint32_to_str (proxy,
							 1048576,
							 my_uint32_to_str_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (uint32_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint32_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_uint32_to_str (proxy,
							 1048576,
							 my_uint32_to_str_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (uint32_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_uint32_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_uint32_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_uint32_to_str_sync (parent, proxy,
						     1048576,
						     &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "1048576");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_uint32_to_str_sync (parent, proxy,
						     0,
						     &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.UInt32ToStr.ZeroInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_uint32_to_str_sync (parent, proxy,
						     4,
						     &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_uint32_replied;
static int32_t last_uint32_value;

static void
my_str_to_uint32_reply (void *          data,
			NihDBusMessage *message,
			uint32_t        output)
{
	str_to_uint32_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_uint32_value = output;
}

void
test_str_to_uint32 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_uint32");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint32_replied = FALSE;
		last_data = NULL;
		last_uint32_value = 0;

		pending_call = proxy_test_str_to_uint32 (proxy,
							 "1048576",
							 my_str_to_uint32_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_uint32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_uint32_value, 1048576);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_uint32 (proxy,
							 "",
							 my_str_to_uint32_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_uint32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToUInt32.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_uint32 (proxy,
							 "invalid",
							 my_str_to_uint32_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_uint32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_uint32 (proxy,
							 "1048576",
							 my_str_to_uint32_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_uint32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_uint32 (proxy,
							 "1048576",
							 my_str_to_uint32_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_uint32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_uint32_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	uint32_t        uint32_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_uint32_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		uint32_value = 0;

		ret = proxy_test_str_to_uint32_sync (parent, proxy,
						     "1048576",
						     &uint32_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (uint32_value, 1048576);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_uint32_sync (parent, proxy,
						     "",
						     &uint32_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToUInt32.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_uint32_sync (parent, proxy,
						     "invalid",
						     &uint32_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int int64_to_str_replied;

static void
my_int64_to_str_reply (void *          data,
		       NihDBusMessage *message,
		       const char *    output)
{
	int64_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_int64_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_int64_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int64_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_int64_to_str (proxy,
							-4815162342L,
							my_int64_to_str_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (int64_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "-4815162342");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int64_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int64_to_str (proxy,
							0,
							my_int64_to_str_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int64_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int64ToStr.ZeroInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int64_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int64_to_str (proxy,
							4,
							my_int64_to_str_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int64_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int64_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int64_to_str (proxy,
							-4815162342L,
							my_int64_to_str_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int64_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int64_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int64_to_str (proxy,
							-4815162342L,
							my_int64_to_str_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int64_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_int64_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_int64_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_int64_to_str_sync (parent, proxy,
						    -4815162342L,
						    &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "-4815162342");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_int64_to_str_sync (parent, proxy,
						    0,
						    &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int64ToStr.ZeroInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_int64_to_str_sync (parent, proxy,
						    4,
						    &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_int64_replied;
static int64_t last_int64_value;

static void
my_str_to_int64_reply (void *          data,
		       NihDBusMessage *message,
		       int64_t         output)
{
	str_to_int64_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_int64_value = output;
}

void
test_str_to_int64 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_int64");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int64_replied = FALSE;
		last_data = NULL;
		last_int64_value = 0;

		pending_call = proxy_test_str_to_int64 (proxy,
							"-4815162342",
							my_str_to_int64_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_int64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_int64_value, -4815162342L);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int64 (proxy,
							"",
							my_str_to_int64_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToInt64.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int64 (proxy,
							"invalid",
							my_str_to_int64_reply,
							my_error_handler,
							parent,
							NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int64 (proxy,
							"-4815162342",
							my_str_to_int64_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int64 (proxy,
							"-4815162342",
							my_str_to_int64_reply,
							my_error_handler,
							parent,
							50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_int64_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int64_t         int64_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_int64_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int64_value = 0;

		ret = proxy_test_str_to_int64_sync (parent, proxy,
						    "-4815162342",
						    &int64_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (int64_value, -4815162342L);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_int64_sync (parent, proxy,
						    "",
						    &int64_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToInt64.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_int64_sync (parent, proxy,
						    "invalid",
						    &int64_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int uint64_to_str_replied;

static void
my_uint64_to_str_reply (void *          data,
			NihDBusMessage *message,
			const char *    output)
{
	uint64_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_uint64_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_uint64_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint64_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_uint64_to_str (proxy,
							 4815162342L,
							 my_uint64_to_str_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (uint64_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "4815162342");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint64_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_uint64_to_str (proxy,
							 0,
							 my_uint64_to_str_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (uint64_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.UInt64ToStr.ZeroInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint64_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_uint64_to_str (proxy,
							 4,
							 my_uint64_to_str_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (uint64_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint64_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_uint64_to_str (proxy,
							 4815162342L,
							 my_uint64_to_str_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (uint64_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		uint64_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_uint64_to_str (proxy,
							 4815162342L,
							 my_uint64_to_str_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (uint64_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_uint64_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_uint64_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_uint64_to_str_sync (parent, proxy,
						     4815162342L,
						     &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "4815162342");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_uint64_to_str_sync (parent, proxy,
						     0,
						     &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.UInt64ToStr.ZeroInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_uint64_to_str_sync (parent, proxy,
						     4,
						     &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_uint64_replied;
static int64_t last_uint64_value;

static void
my_str_to_uint64_reply (void *          data,
			NihDBusMessage *message,
			uint64_t        output)
{
	str_to_uint64_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_uint64_value = output;
}

void
test_str_to_uint64 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_uint64");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint64_replied = FALSE;
		last_data = NULL;
		last_uint64_value = 0;

		pending_call = proxy_test_str_to_uint64 (proxy,
							 "4815162342",
							 my_str_to_uint64_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_uint64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_uint64_value, 4815162342L);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_uint64 (proxy,
							 "",
							 my_str_to_uint64_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_uint64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToUInt64.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_uint64 (proxy,
							 "invalid",
							 my_str_to_uint64_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_uint64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_uint64 (proxy,
							 "4815162342",
							 my_str_to_uint64_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_uint64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_uint64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_uint64 (proxy,
							 "4815162342",
							 my_str_to_uint64_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_uint64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_uint64_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	uint64_t        uint64_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_uint64_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		uint64_value = 0;

		ret = proxy_test_str_to_uint64_sync (parent, proxy,
						     "4815162342",
						     &uint64_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (uint64_value, 4815162342L);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_uint64_sync (parent, proxy,
						     "",
						     &uint64_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToUInt64.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_uint64_sync (parent, proxy,
						     "invalid",
						     &uint64_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int double_to_str_replied;

static void
my_double_to_str_reply (void *          data,
			NihDBusMessage *message,
			const char *    output)
{
	double_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_double_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_double_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		double_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_double_to_str (proxy,
							 3.141597,
							 my_double_to_str_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (double_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "3.141597");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		double_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_double_to_str (proxy,
							 0,
							 my_double_to_str_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (double_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.DoubleToStr.ZeroInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		double_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_double_to_str (proxy,
							 4,
							 my_double_to_str_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (double_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		double_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_double_to_str (proxy,
							 3.141597,
							 my_double_to_str_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (double_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		double_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_double_to_str (proxy,
							 3.141597,
							 my_double_to_str_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (double_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_double_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_double_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_double_to_str_sync (parent, proxy,
						     3.141597,
						     &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "3.141597");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_double_to_str_sync (parent, proxy,
						     0,
						     &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.DoubleToStr.ZeroInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_double_to_str_sync (parent, proxy,
						     4,
						     &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_double_replied;
static double last_double_value;

static void
my_str_to_double_reply (void *          data,
			NihDBusMessage *message,
			double          output)
{
	str_to_double_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_double_value = output;
}

void
test_str_to_double (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_double");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_double_replied = FALSE;
		last_data = NULL;
		last_double_value = 0;

		pending_call = proxy_test_str_to_double (proxy,
							 "3.141597",
							 my_str_to_double_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_double_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_double_value, 3.141597);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_double_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_double (proxy,
							 "",
							 my_str_to_double_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_double_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToDouble.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_double_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_double (proxy,
							 "invalid",
							 my_str_to_double_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_double_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_double_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_double (proxy,
							 "3.141597",
							 my_str_to_double_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_double_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_double_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_double (proxy,
							 "3.141597",
							 my_str_to_double_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_double_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_double_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	double          double_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_double_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		double_value = 0;

		ret = proxy_test_str_to_double_sync (parent, proxy,
						     "3.141597",
						     &double_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (double_value, 3.141597);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_double_sync (parent, proxy,
						     "",
						     &double_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToDouble.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_double_sync (parent, proxy,
						     "invalid",
						     &double_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int object_path_to_str_replied;

static void
my_object_path_to_str_reply (void *          data,
			     NihDBusMessage *message,
			     const char *    output)
{
	object_path_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_object_path_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_object_path_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		object_path_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_object_path_to_str (proxy,
							      "/com/netsplit/Nih/Test",
							      my_object_path_to_str_reply,
							      my_error_handler,
							      parent,
							      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (object_path_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "/com/netsplit/Nih/Test");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		object_path_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_object_path_to_str (proxy,
							      "/",
							      my_object_path_to_str_reply,
							      my_error_handler,
							      parent,
							      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (object_path_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.ObjectPathToStr.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		object_path_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_object_path_to_str (proxy,
							      "/invalid",
							      my_object_path_to_str_reply,
							      my_error_handler,
							      parent,
							      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (object_path_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		object_path_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_object_path_to_str (proxy,
							      "/com/netsplit/Nih/Test",
							      my_object_path_to_str_reply,
							      my_error_handler,
							      parent,
							      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (object_path_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		object_path_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_object_path_to_str (proxy,
							      "/com/netsplit/Nih/Test",
							      my_object_path_to_str_reply,
							      my_error_handler,
							      parent,
							      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (object_path_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_object_path_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_object_path_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_object_path_to_str_sync (parent, proxy,
							  "/com/netsplit/Nih/Test",
							  &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "/com/netsplit/Nih/Test");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_object_path_to_str_sync (parent, proxy,
							  "/",
							  &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.ObjectPathToStr.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_object_path_to_str_sync (parent, proxy,
							  "/invalid",
							  &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_object_path_replied;

static void
my_str_to_object_path_reply (void *          data,
			     NihDBusMessage *message,
			     const char *    output)
{
	str_to_object_path_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_str_to_object_path (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_object_path");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_object_path_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_str_to_object_path (proxy,
							      "/com/netsplit/Nih/Test",
							      my_str_to_object_path_reply,
							      my_error_handler,
							      parent,
							      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_object_path_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "/com/netsplit/Nih/Test");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_object_path_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_object_path (proxy,
							      "",
							      my_str_to_object_path_reply,
							      my_error_handler,
							      parent,
							      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_object_path_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToObjectPath.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_object_path_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_object_path (proxy,
							      "invalid",
							      my_str_to_object_path_reply,
							      my_error_handler,
							      parent,
							      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_object_path_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_object_path_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_object_path (proxy,
							      "/com/netsplit/Nih/Test",
							      my_str_to_object_path_reply,
							      my_error_handler,
							      parent,
							      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_object_path_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_object_path_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_object_path (proxy,
							      "/com/netsplit/Nih/Test",
							      my_str_to_object_path_reply,
							      my_error_handler,
							      parent,
							      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_object_path_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_object_path_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_object_path_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_str_to_object_path_sync (parent, proxy,
							  "/com/netsplit/Nih/Test",
							  &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "/com/netsplit/Nih/Test");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_object_path_sync (parent, proxy,
							  "",
							  &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToObjectPath.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_object_path_sync (parent, proxy,
							  "invalid",
							  &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int signature_to_str_replied;

static void
my_signature_to_str_reply (void *          data,
			   NihDBusMessage *message,
			   const char *    output)
{
	signature_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_signature_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_signature_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		signature_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_signature_to_str (proxy,
							    "a(ib)",
							    my_signature_to_str_reply,
							    my_error_handler,
							    parent,
							    0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (signature_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "a(ib)");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		signature_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_signature_to_str (proxy,
							    "",
							    my_signature_to_str_reply,
							    my_error_handler,
							    parent,
							    0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (signature_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.SignatureToStr.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		signature_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_signature_to_str (proxy,
							    "inva(x)id",
							    my_signature_to_str_reply,
							    my_error_handler,
							    parent,
							    0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (signature_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		signature_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_signature_to_str (proxy,
							    "a(ib)",
							    my_signature_to_str_reply,
							    my_error_handler,
							    parent,
							    50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (signature_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		signature_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_signature_to_str (proxy,
							    "a(ib)",
							    my_signature_to_str_reply,
							    my_error_handler,
							    parent,
							    50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (signature_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_signature_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_signature_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_signature_to_str_sync (parent, proxy,
							"a(ib)",
							&str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "a(ib)");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_signature_to_str_sync (parent, proxy,
							"",
							&str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.SignatureToStr.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_signature_to_str_sync (parent, proxy,
							"inva(x)id",
							&str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_signature_replied;

static void
my_str_to_signature_reply (void *          data,
			   NihDBusMessage *message,
			   const char *    output)
{
	str_to_signature_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_str_to_signature (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_signature");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_signature_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_str_to_signature (proxy,
							    "a(ib)",
							    my_str_to_signature_reply,
							    my_error_handler,
							    parent,
							    0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_signature_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "a(ib)");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_signature_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_signature (proxy,
							    "",
							    my_str_to_signature_reply,
							    my_error_handler,
							    parent,
							    0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_signature_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToSignature.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_signature_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_signature (proxy,
							    "invalid",
							    my_str_to_signature_reply,
							    my_error_handler,
							    parent,
							    0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_signature_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_signature_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_signature (proxy,
							    "a(ib)",
							    my_str_to_signature_reply,
							    my_error_handler,
							    parent,
							    50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_signature_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_signature_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_signature (proxy,
							    "a(ib)",
							    my_str_to_signature_reply,
							    my_error_handler,
							    parent,
							    50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_signature_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_signature_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_signature_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_str_to_signature_sync (parent, proxy,
							"a(ib)",
							&str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "a(ib)");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_signature_sync (parent, proxy,
							"",
							&str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToSignature.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_str_to_signature_sync (parent, proxy,
							"invalid",
							&str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int int32_array_to_str_replied;

static void
my_int32_array_to_str_reply (void *          data,
			     NihDBusMessage *message,
			     const char *    output)
{
	int32_array_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_int32_array_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	int32_t *        int32_array;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_int32_array_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array = nih_alloc (parent, sizeof (int32_t) * 6);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;
			int32_array[4] = 23;
			int32_array[5] = 42;
		}

		error_handler_called = FALSE;
		int32_array_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_int32_array_to_str (
			proxy,
			int32_array, 6,
			my_int32_array_to_str_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (int32_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "4 8 15 16 23 42");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		int32_array_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int32_array_to_str (
			proxy,
			NULL, 0,
			my_int32_array_to_str_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int32_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int32ArrayToStr.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array = nih_alloc (parent, sizeof (int32_t) * 4);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;
		}

		error_handler_called = FALSE;
		int32_array_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int32_array_to_str (
			proxy,
			int32_array, 4,
			my_int32_array_to_str_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int32_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array = nih_alloc (parent, sizeof (int32_t) * 6);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;
			int32_array[4] = 23;
			int32_array[5] = 42;
		}

		error_handler_called = FALSE;
		int32_array_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int32_array_to_str (
			proxy,
			int32_array, 6,
			my_int32_array_to_str_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int32_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array = nih_alloc (parent, sizeof (int32_t) * 6);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;
			int32_array[4] = 23;
			int32_array[5] = 42;
		}

		error_handler_called = FALSE;
		int32_array_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int32_array_to_str (
			proxy,
			int32_array, 6,
			my_int32_array_to_str_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int32_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_int32_array_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int32_t *       int32_array;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_int32_array_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array = nih_alloc (parent, sizeof (int32_t) * 6);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;
			int32_array[4] = 23;
			int32_array[5] = 42;
		}

		str_value = NULL;

		ret = proxy_test_int32_array_to_str_sync (parent, proxy,
							  int32_array, 6,
							  &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "4 8 15 16 23 42");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_int32_array_to_str_sync (parent, proxy,
							  NULL, 0,
							  &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int32ArrayToStr.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array = nih_alloc (parent, sizeof (int32_t) * 6);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;
		}

		ret = proxy_test_int32_array_to_str_sync (parent, proxy,
							  int32_array, 4,
							  &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_int32_array_replied;
static const int32_t *last_int32_array;
static size_t         last_int32_array_len;

static void
my_str_to_int32_array_reply (void *          data,
			     NihDBusMessage *message,
			     const int32_t * output,
			     size_t          output_len)
{
	str_to_int32_array_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	if (output_len)
		TEST_NE_P (output, NULL);

	last_data = data;
	last_int32_array = output;
	nih_ref (last_int32_array, last_data);
	last_int32_array_len = output_len;
}

void
test_str_to_int32_array (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_int32_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_array_replied = FALSE;
		last_data = NULL;
		last_int32_array = NULL;
		last_int32_array_len = 0;

		pending_call = proxy_test_str_to_int32_array (proxy,
							      "4 8 15 16 23 42",
							      my_str_to_int32_array_reply,
							      my_error_handler,
							      parent,
							      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_int32_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_int32_array_len, 6);
		TEST_ALLOC_SIZE (last_int32_array, sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (last_int32_array, parent);
		TEST_EQ (last_int32_array[0], 4);
		TEST_EQ (last_int32_array[1], 8);
		TEST_EQ (last_int32_array[2], 15);
		TEST_EQ (last_int32_array[3], 16);
		TEST_EQ (last_int32_array[4], 23);
		TEST_EQ (last_int32_array[5], 42);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int32_array (proxy,
							      "",
							      my_str_to_int32_array_reply,
							      my_error_handler,
							      parent,
							      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int32_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToInt32Array.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int32_array (proxy,
							      "invalid",
							      my_str_to_int32_array_reply,
							      my_error_handler,
							      parent,
							      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int32_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int32_array (proxy,
							      "4 8 15 16 23 42",
							      my_str_to_int32_array_reply,
							      my_error_handler,
							      parent,
							      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int32_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int32_array (proxy,
							      "4 8 15 16 23 42",
							      my_str_to_int32_array_reply,
							      my_error_handler,
							      parent,
							      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int32_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_int32_array_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int32_t *       int32_array;
 	size_t          int32_array_len;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_int32_array_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int32_array = NULL;
		int32_array_len = 0;

		ret = proxy_test_str_to_int32_array_sync (parent, proxy,
							  "4 8 15 16 23 42",
							  &int32_array,
							  &int32_array_len);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (int32_array_len, 6);
		TEST_ALLOC_SIZE (int32_array, sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (int32_array, parent);
		TEST_EQ (int32_array[0], 4);
		TEST_EQ (int32_array[1], 8);
		TEST_EQ (int32_array[2], 15);
		TEST_EQ (int32_array[3], 16);
		TEST_EQ (int32_array[4], 23);
		TEST_EQ (int32_array[5], 42);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int32_array = NULL;
		int32_array_len = 0;

		ret = proxy_test_str_to_int32_array_sync (parent, proxy,
							  "",
							  &int32_array,
							  &int32_array_len);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToInt32Array.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int32_array = NULL;
		int32_array_len = 0;

		ret = proxy_test_str_to_int32_array_sync (parent, proxy,
							  "invalid",
							  &int32_array,
							  &int32_array_len);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_array_to_str_replied;

static void
my_str_array_to_str_reply (void *          data,
			     NihDBusMessage *message,
			     const char *    output)
{
	str_array_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_str_array_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	char **          str_array;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_array_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 7);
			str_array[0] = "she";
			str_array[1] = "needs";
			str_array[2] = "more";
			str_array[3] = "of";
			str_array[4] = "ze";
			str_array[5] = "punishment";
			str_array[6] = NULL;
		}

		error_handler_called = FALSE;
		str_array_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_str_array_to_str (
			proxy,
			str_array,
			my_str_array_to_str_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "she needs more of ze punishment");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 1);
			str_array[0] = NULL;
		}

		error_handler_called = FALSE;
		str_array_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_array_to_str (
			proxy,
			str_array,
			my_str_array_to_str_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrArrayToStr.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 5);
			str_array[0] = "this";
			str_array[1] = "is";
			str_array[2] = "a";
			str_array[3] = "test";
			str_array[4] = NULL;
		}

		error_handler_called = FALSE;
		str_array_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_array_to_str (
			proxy,
			str_array,
			my_str_array_to_str_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 7);
			str_array[0] = "she";
			str_array[1] = "needs";
			str_array[2] = "more";
			str_array[3] = "of";
			str_array[4] = "ze";
			str_array[5] = "punishment";
			str_array[6] = NULL;
		}

		error_handler_called = FALSE;
		str_array_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_array_to_str (
			proxy,
			str_array,
			my_str_array_to_str_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 7);
			str_array[0] = "she";
			str_array[1] = "needs";
			str_array[2] = "more";
			str_array[3] = "of";
			str_array[4] = "ze";
			str_array[5] = "punishment";
			str_array[6] = NULL;
		}

		error_handler_called = FALSE;
		str_array_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_array_to_str (
			proxy,
			str_array,
			my_str_array_to_str_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_array_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char **         str_array;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_array_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 7);
			str_array[0] = "she";
			str_array[1] = "needs";
			str_array[2] = "more";
			str_array[3] = "of";
			str_array[4] = "ze";
			str_array[5] = "punishment";
			str_array[6] = NULL;
		}

		str_value = NULL;

		ret = proxy_test_str_array_to_str_sync (parent, proxy,
							str_array,
							&str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "she needs more of ze punishment");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 1);
			str_array[0] = NULL;
		}

		ret = proxy_test_str_array_to_str_sync (parent, proxy,
							str_array,
							&str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrArrayToStr.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 5);
			str_array[0] = "this";
			str_array[1] = "is";
			str_array[2] = "a";
			str_array[3] = "test";
			str_array[4] = NULL;
		}

		ret = proxy_test_str_array_to_str_sync (parent, proxy,
							str_array,
							&str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_str_array_replied;
static char * const *last_str_array;

static void
my_str_to_str_array_reply (void *          data,
			   NihDBusMessage *message,
			   char * const *  output)
{
	str_to_str_array_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_array = output;
	nih_ref (last_str_array, last_data);
}

void
test_str_to_str_array (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_str_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_str_array_replied = FALSE;
		last_data = NULL;
		last_str_array = NULL;

		pending_call = proxy_test_str_to_str_array (proxy,
							    "she needs more of ze punishment",
							    my_str_to_str_array_reply,
							    my_error_handler,
							    parent,
							    0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_str_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_ALLOC_SIZE (last_str_array, sizeof (char *) * 7);
		TEST_ALLOC_PARENT (last_str_array, parent);
		TEST_EQ_STR (last_str_array[0], "she");
		TEST_EQ_STR (last_str_array[1], "needs");
		TEST_EQ_STR (last_str_array[2], "more");
		TEST_EQ_STR (last_str_array[3], "of");
		TEST_EQ_STR (last_str_array[4], "ze");
		TEST_EQ_STR (last_str_array[5], "punishment");
		TEST_EQ_P (last_str_array[6], NULL);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_str_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_str_array (proxy,
							    "",
							    my_str_to_str_array_reply,
							    my_error_handler,
							    parent,
							    0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_str_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToStrArray.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_str_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_str_array (proxy,
							    "invalid",
							    my_str_to_str_array_reply,
							    my_error_handler,
							    parent,
							    0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_str_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_str_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_str_array (proxy,
							    "she needs more of ze punishment",
							    my_str_to_str_array_reply,
							    my_error_handler,
							    parent,
							    50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_str_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_str_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_str_array (proxy,
							    "she needs more of ze punishment",
							    my_str_to_str_array_reply,
							    my_error_handler,
							    parent,
							    50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_str_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_str_array_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char **         str_array;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_str_array_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_array = NULL;

		ret = proxy_test_str_to_str_array_sync (parent, proxy,
							"she needs more of ze punishment",
							&str_array);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_SIZE (str_array, sizeof (char *) * 7);
		TEST_ALLOC_PARENT (str_array, parent);
		TEST_EQ_STR (str_array[0], "she");
		TEST_EQ_STR (str_array[1], "needs");
		TEST_EQ_STR (str_array[2], "more");
		TEST_EQ_STR (str_array[3], "of");
		TEST_EQ_STR (str_array[4], "ze");
		TEST_EQ_STR (str_array[5], "punishment");
		TEST_EQ_P (str_array[6], NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_array = NULL;

		ret = proxy_test_str_to_str_array_sync (parent, proxy,
							"",
							&str_array);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToStrArray.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_array = NULL;

		ret = proxy_test_str_to_str_array_sync (parent, proxy,
							"invalid",
							&str_array);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int int32_array_array_to_str_replied;

static void
my_int32_array_array_to_str_reply (void *          data,
				   NihDBusMessage *message,
				   const char *    output)
{
	int32_array_array_to_str_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);

	last_data = data;
	last_str_value = output;
	nih_ref (last_str_value, last_data);
}

void
test_int32_array_array_to_str (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	int32_t **       int32_array_array;
	size_t *         int32_array_array_len;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_int32_array_array_to_str");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 3);
			int32_array_array_len = nih_alloc (parent, sizeof (size_t) * 2);

			int32_array_array[0] = nih_alloc (int32_array_array, sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;
			int32_array_array_len[0] = 6;

			int32_array_array[1] = nih_alloc (int32_array_array, sizeof (int32_t) * 6);
			int32_array_array[1][0] = 1;
			int32_array_array[1][1] = 1;
			int32_array_array[1][2] = 2;
			int32_array_array[1][3] = 3;
			int32_array_array[1][4] = 5;
			int32_array_array[1][5] = 8;
			int32_array_array_len[1] = 6;

			int32_array_array[2] = NULL;
		}

		error_handler_called = FALSE;
		int32_array_array_to_str_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_int32_array_array_to_str (
			proxy,
			int32_array_array, int32_array_array_len,
			my_int32_array_array_to_str_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (int32_array_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "4 8 15 16 23 42\n1 1 2 3 5 8");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 1);
			int32_array_array_len = NULL;

			int32_array_array[0] = NULL;
		}

		error_handler_called = FALSE;
		int32_array_array_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int32_array_array_to_str (
			proxy,
			int32_array_array, int32_array_array_len,
			my_int32_array_array_to_str_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int32_array_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int32ArrayArrayToStr.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 2);
			int32_array_array_len = nih_alloc (parent, sizeof (size_t) * 1);

			int32_array_array[0] = nih_alloc (int32_array_array, sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;
			int32_array_array_len[0] = 6;

			int32_array_array[1] = NULL;
		}

		error_handler_called = FALSE;
		int32_array_array_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int32_array_array_to_str (
			proxy,
			int32_array_array, int32_array_array_len,
			my_int32_array_array_to_str_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int32_array_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 3);
			int32_array_array_len = nih_alloc (parent, sizeof (size_t) * 2);

			int32_array_array[0] = nih_alloc (int32_array_array, sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;
			int32_array_array_len[0] = 6;

			int32_array_array[1] = nih_alloc (int32_array_array, sizeof (int32_t) * 6);
			int32_array_array[1][0] = 1;
			int32_array_array[1][1] = 1;
			int32_array_array[1][2] = 2;
			int32_array_array[1][3] = 3;
			int32_array_array[1][4] = 5;
			int32_array_array[1][5] = 8;
			int32_array_array_len[1] = 6;

			int32_array_array[2] = NULL;
		}

		error_handler_called = FALSE;
		int32_array_array_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int32_array_array_to_str (
			proxy,
			int32_array_array, int32_array_array_len,
			my_int32_array_array_to_str_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int32_array_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 3);
			int32_array_array_len = nih_alloc (parent, sizeof (size_t) * 2);

			int32_array_array[0] = nih_alloc (int32_array_array, sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;
			int32_array_array_len[0] = 6;

			int32_array_array[1] = nih_alloc (int32_array_array, sizeof (int32_t) * 6);
			int32_array_array[1][0] = 1;
			int32_array_array[1][1] = 1;
			int32_array_array[1][2] = 2;
			int32_array_array[1][3] = 3;
			int32_array_array[1][4] = 5;
			int32_array_array[1][5] = 8;
			int32_array_array_len[1] = 6;

			int32_array_array[2] = NULL;
		}

		error_handler_called = FALSE;
		int32_array_array_to_str_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_int32_array_array_to_str (
			proxy,
			int32_array_array, int32_array_array_len,
			my_int32_array_array_to_str_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (int32_array_array_to_str_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_int32_array_array_to_str_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int32_t **      int32_array_array;
	size_t *        int32_array_array_len;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_int32_array_array_to_str_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 3);
			int32_array_array_len = nih_alloc (parent, sizeof (size_t) * 2);

			int32_array_array[0] = nih_alloc (int32_array_array, sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;
			int32_array_array_len[0] = 6;

			int32_array_array[1] = nih_alloc (int32_array_array, sizeof (int32_t) * 6);
			int32_array_array[1][0] = 1;
			int32_array_array[1][1] = 1;
			int32_array_array[1][2] = 2;
			int32_array_array[1][3] = 3;
			int32_array_array[1][4] = 5;
			int32_array_array[1][5] = 8;
			int32_array_array_len[1] = 6;

			int32_array_array[2] = NULL;
		}

		str_value = NULL;

		ret = proxy_test_int32_array_array_to_str_sync (
			parent, proxy,
			int32_array_array, int32_array_array_len,
			&str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (str_value, "4 8 15 16 23 42\n1 1 2 3 5 8");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 1);
			int32_array_array_len = NULL;

			int32_array_array[0] = NULL;
		}

		ret = proxy_test_int32_array_array_to_str_sync (
			parent, proxy,
			int32_array_array, int32_array_array_len,
			&str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int32ArrayArrayToStr.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 2);
			int32_array_array_len = nih_alloc (parent, sizeof (size_t) * 1);

			int32_array_array[0] = nih_alloc (int32_array_array, sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;
			int32_array_array_len[0] = 6;

			int32_array_array[1] = NULL;
		}

		ret = proxy_test_int32_array_array_to_str_sync (
			parent, proxy,
			int32_array_array, int32_array_array_len,
			&str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int str_to_int32_array_array_replied;
static int32_t * const *last_int32_array_array;
static const size_t *   last_int32_array_array_len;

static void
my_str_to_int32_array_array_reply (void *           data,
				   NihDBusMessage * message,
				   int32_t * const *output,
				   const size_t *   output_len)
{
	str_to_int32_array_array_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);
	if (*output) {
		TEST_NE_P (output_len, NULL);
		TEST_ALLOC_PARENT (output_len, output);
	}

	last_data = data;
	last_int32_array_array = output;
	nih_ref (last_int32_array_array, last_data);
	last_int32_array_array_len = output_len;
	nih_ref (last_int32_array_array_len, last_data);
}

void
test_str_to_int32_array_array (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_str_to_int32_array_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_array_array_replied = FALSE;
		last_data = NULL;
		last_int32_array_array = NULL;
		last_int32_array_array_len = NULL;

		pending_call = proxy_test_str_to_int32_array_array (
			proxy,
			"4 8 15 16 23 42\n1 1 2 3 5 8",
			my_str_to_int32_array_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (str_to_int32_array_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_ALLOC_SIZE (last_int32_array_array_len, sizeof (size_t) * 2);
		TEST_ALLOC_PARENT (last_int32_array_array_len, parent);
		TEST_ALLOC_SIZE (last_int32_array_array, sizeof (int32_t *) * 3);

		TEST_EQ (last_int32_array_array_len[0], 6);
		TEST_ALLOC_SIZE (last_int32_array_array[0], sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (last_int32_array_array[0], last_int32_array_array);
		TEST_EQ (last_int32_array_array[0][0], 4);
		TEST_EQ (last_int32_array_array[0][1], 8);
		TEST_EQ (last_int32_array_array[0][2], 15);
		TEST_EQ (last_int32_array_array[0][3], 16);
		TEST_EQ (last_int32_array_array[0][4], 23);
		TEST_EQ (last_int32_array_array[0][5], 42);

		TEST_EQ (last_int32_array_array_len[1], 6);
		TEST_ALLOC_SIZE (last_int32_array_array[1], sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (last_int32_array_array[1], last_int32_array_array);
		TEST_EQ (last_int32_array_array[1][0], 1);
		TEST_EQ (last_int32_array_array[1][1], 1);
		TEST_EQ (last_int32_array_array[1][2], 2);
		TEST_EQ (last_int32_array_array[1][3], 3);
		TEST_EQ (last_int32_array_array[1][4], 5);
		TEST_EQ (last_int32_array_array[1][5], 8);

		TEST_EQ_P (last_int32_array_array[2], NULL);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_array_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int32_array_array (
			proxy,
			"",
			my_str_to_int32_array_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int32_array_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToInt32ArrayArray.EmptyInput");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_array_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int32_array_array (
			proxy,
			"invalid",
			my_str_to_int32_array_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int32_array_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_array_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int32_array_array (
			proxy,
			"4 8 15 16 23 42\n1 1 2 3 5 8",
			my_str_to_int32_array_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int32_array_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		str_to_int32_array_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_str_to_int32_array_array (
			proxy,
			"4 8 15 16 23 42\n1 1 2 3 5 8",
			my_str_to_int32_array_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (str_to_int32_array_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_int32_array_array_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int32_t **      int32_array_array;
 	size_t *        int32_array_array_len;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_str_to_int32_array_array_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int32_array_array = NULL;
		int32_array_array_len = NULL;

		ret = proxy_test_str_to_int32_array_array_sync (
			parent, proxy,
			"4 8 15 16 23 42\n1 1 2 3 5 8",
			&int32_array_array,
			&int32_array_array_len);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_SIZE (int32_array_array_len, sizeof (size_t) * 2);
		TEST_ALLOC_PARENT (int32_array_array_len, int32_array_array);

		TEST_ALLOC_SIZE (int32_array_array, sizeof (int32_t) * 3);
		TEST_ALLOC_PARENT (int32_array_array, parent);

		TEST_EQ (int32_array_array_len[0], 6);
		TEST_ALLOC_SIZE (int32_array_array[0], sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (int32_array_array[0], int32_array_array);
		TEST_EQ (int32_array_array[0][0], 4);
		TEST_EQ (int32_array_array[0][1], 8);
		TEST_EQ (int32_array_array[0][2], 15);
		TEST_EQ (int32_array_array[0][3], 16);
		TEST_EQ (int32_array_array[0][4], 23);
		TEST_EQ (int32_array_array[0][5], 42);

		TEST_EQ (int32_array_array_len[1], 6);
		TEST_ALLOC_SIZE (int32_array_array[1], sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (int32_array_array[1], int32_array_array);
		TEST_EQ (int32_array_array[1][0], 1);
		TEST_EQ (int32_array_array[1][1], 1);
		TEST_EQ (int32_array_array[1][2], 2);
		TEST_EQ (int32_array_array[1][3], 3);
		TEST_EQ (int32_array_array[1][4], 5);
		TEST_EQ (int32_array_array[1][5], 8);

		TEST_EQ_P (int32_array_array[2], NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int32_array_array = NULL;
		int32_array_array_len = NULL;

		ret = proxy_test_str_to_int32_array_array_sync (
			parent, proxy,
			"",
			&int32_array_array,
			&int32_array_array_len);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrToInt32ArrayArray.EmptyInput");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int32_array_array = NULL;
		int32_array_array_len = NULL;

		ret = proxy_test_str_to_int32_array_array_sync (
			parent, proxy,
			"invalid",
			&int32_array_array,
			&int32_array_array_len);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}



static int new_byte_handled;

static void
my_new_byte_handler (void *          data,
		     NihDBusMessage *message,
		     uint8_t         value)
{
	new_byte_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_byte_value = value;
}

void
test_new_byte (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewByte_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewByte",
							(NihDBusSignalHandler)my_new_byte_handler,
							parent));

			assert0 (my_test_emit_new_byte (server_conn,
							"/com/netsplit/Nih/Test",
							97));
		}

		new_byte_handled = FALSE;
		last_data = NULL;
		last_byte_value = 0;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_byte_handled);
		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_byte_value, 97);

		nih_free (parent);
		nih_free (proxy);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_boolean_handled;

static void
my_new_boolean_handler (void *          data,
			NihDBusMessage *message,
			int             value)
{
	new_boolean_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_boolean_value = value;
}

void
test_new_boolean (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewBoolean_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewBoolean",
							(NihDBusSignalHandler)my_new_boolean_handler,
							parent));

			assert0 (my_test_emit_new_boolean (server_conn,
							   "/com/netsplit/Nih/Test",
							   TRUE));
		}

		new_boolean_handled = FALSE;
		last_data = NULL;
		last_boolean_value = 0;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_boolean_handled);
		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_boolean_value, TRUE);

		nih_free (parent);
		nih_free (proxy);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_int16_handled;

static void
my_new_int16_handler (void *          data,
		      NihDBusMessage *message,
		      int16_t         value)
{
	new_int16_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_int16_value = value;
}

void
test_new_int16 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewInt16_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewInt16",
							(NihDBusSignalHandler)my_new_int16_handler,
							parent));

			assert0 (my_test_emit_new_int16 (server_conn,
							 "/com/netsplit/Nih/Test",
							 -42));
		}

		new_int16_handled = FALSE;
		last_data = NULL;
		last_int16_value = 0;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_int16_handled);
		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_int16_value, -42);

		nih_free (parent);
		nih_free (proxy);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_uint16_handled;

static void
my_new_uint16_handler (void *          data,
		       NihDBusMessage *message,
		       uint16_t        value)
{
	new_uint16_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_uint16_value = value;
}

void
test_new_uint16 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewUInt16_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewUInt16",
							(NihDBusSignalHandler)my_new_uint16_handler,
							parent));

			assert0 (my_test_emit_new_uint16 (server_conn,
							  "/com/netsplit/Nih/Test",
							  42));
		}

		new_uint16_handled = FALSE;
		last_data = NULL;
		last_uint16_value = 0;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_uint16_handled);
		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_uint16_value, 42);

		nih_free (parent);
		nih_free (proxy);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_int32_handled;

static void
my_new_int32_handler (void *          data,
		      NihDBusMessage *message,
		      int32_t         value)
{
	new_int32_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_int32_value = value;
}

void
test_new_int32 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewInt32_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewInt32",
							(NihDBusSignalHandler)my_new_int32_handler,
							parent));

			assert0 (my_test_emit_new_int32 (server_conn,
							 "/com/netsplit/Nih/Test",
							 -1048576));
		}

		new_int32_handled = FALSE;
		last_data = NULL;
		last_int32_value = 0;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_int32_handled);
		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_int32_value, -1048576);

		nih_free (parent);
		nih_free (proxy);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_uint32_handled;

static void
my_new_uint32_handler (void *          data,
		       NihDBusMessage *message,
		       uint32_t        value)
{
	new_uint32_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_uint32_value = value;
}

void
test_new_uint32 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewUInt32_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewUInt32",
							(NihDBusSignalHandler)my_new_uint32_handler,
							parent));

			assert0 (my_test_emit_new_uint32 (server_conn,
							  "/com/netsplit/Nih/Test",
							  1048576));
		}

		new_uint32_handled = FALSE;
		last_data = NULL;
		last_uint32_value = 0;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_uint32_handled);
		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_uint32_value, 1048576);

		nih_free (parent);
		nih_free (proxy);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_int64_handled;

static void
my_new_int64_handler (void *          data,
		      NihDBusMessage *message,
		      int64_t         value)
{
	new_int64_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_int64_value = value;
}

void
test_new_int64 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewInt64_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewInt64",
							(NihDBusSignalHandler)my_new_int64_handler,
							parent));

			assert0 (my_test_emit_new_int64 (server_conn,
							 "/com/netsplit/Nih/Test",
							 -4815162342L));
		}

		new_int64_handled = FALSE;
		last_data = NULL;
		last_int64_value = 0;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_int64_handled);
		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_int64_value, -4815162342L);

		nih_free (parent);
		nih_free (proxy);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_uint64_handled;

static void
my_new_uint64_handler (void *          data,
		       NihDBusMessage *message,
		       uint64_t        value)
{
	new_uint64_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_uint64_value = value;
}

void
test_new_uint64 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewUInt64_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewUInt64",
							(NihDBusSignalHandler)my_new_uint64_handler,
							parent));

			assert0 (my_test_emit_new_uint64 (server_conn,
							  "/com/netsplit/Nih/Test",
							  48151623L));
		}

		new_uint64_handled = FALSE;
		last_data = NULL;
		last_uint64_value = 0;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_uint64_handled);
		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_uint64_value, 48151623L);

		nih_free (parent);
		nih_free (proxy);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_double_handled;

static void
my_new_double_handler (void *          data,
		       NihDBusMessage *message,
		       double          value)
{
	new_double_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_double_value = value;
}

void
test_new_double (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewDouble_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewDouble",
							(NihDBusSignalHandler)my_new_double_handler,
							parent));

			assert0 (my_test_emit_new_double (server_conn,
							  "/com/netsplit/Nih/Test",
							  3.141597));
		}

		new_double_handled = FALSE;
		last_data = NULL;
		last_double_value = 0;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_double_handled);
		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_double_value, 3.141597);

		nih_free (parent);
		nih_free (proxy);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_string_handled;

static void
my_new_string_handler (void *          data,
		       NihDBusMessage *message,
		       const char *    value)
{
	new_string_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);
	TEST_ALLOC_PARENT (value, message);

	last_data = data;
	last_str_value = value;
	nih_ref (last_str_value, last_data);
}

void
test_new_string (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewString_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewString",
							(NihDBusSignalHandler)my_new_string_handler,
							parent));

			assert0 (my_test_emit_new_string (server_conn,
							  "/com/netsplit/Nih/Test",
							  "she needs more of ze punishment"));
		}

		new_string_handled = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_string_handled);
		TEST_EQ_P (last_data, parent);
		TEST_ALLOC_PARENT (last_str_value, parent);
		TEST_EQ_STR (last_str_value, "she needs more of ze punishment");

		nih_free (parent);
		nih_free (proxy);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_object_path_handled;

static void
my_new_object_path_handler (void *          data,
			    NihDBusMessage *message,
			    const char *    value)
{
	new_object_path_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);
	TEST_ALLOC_PARENT (value, message);

	last_data = data;
	last_str_value = value;
	nih_ref (last_str_value, last_data);
}

void
test_new_object_path (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewObjectPath_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewObjectPath",
							(NihDBusSignalHandler)my_new_object_path_handler,
							parent));

			assert0 (my_test_emit_new_object_path (server_conn,
							       "/com/netsplit/Nih/Test",
							       "/com/netsplit/Nih/Test"));
		}

		new_object_path_handled = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_object_path_handled);
		TEST_EQ_P (last_data, parent);
		TEST_ALLOC_PARENT (last_str_value, parent);
		TEST_EQ_STR (last_str_value, "/com/netsplit/Nih/Test");

		nih_free (parent);
		nih_free (proxy);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_signature_handled;

static void
my_new_signature_handler (void *          data,
			  NihDBusMessage *message,
			  const char *    value)
{
	new_signature_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);
	TEST_ALLOC_PARENT (value, message);

	last_data = data;
	last_str_value = value;
	nih_ref (last_str_value, last_data);
}

void
test_new_signature (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewSignature_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewSignature",
							(NihDBusSignalHandler)my_new_signature_handler,
							parent));

			assert0 (my_test_emit_new_signature (server_conn,
							     "/com/netsplit/Nih/Test",
							     "a(ib)"));
		}

		new_signature_handled = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_signature_handled);
		TEST_EQ_P (last_data, parent);
		TEST_ALLOC_PARENT (last_str_value, parent);
		TEST_EQ_STR (last_str_value, "a(ib)");

		nih_free (parent);
		nih_free (proxy);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_int32_array_handled;

static void
my_new_int32_array_handler (void *          data,
			    NihDBusMessage *message,
			    int32_t *       value,
			    size_t          value_len)
{
	new_int32_array_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	if (value_len) {
		TEST_NE_P (value, NULL);
		TEST_ALLOC_PARENT (value, message);
	}

	last_data = data;
	last_int32_array = value;
	if (value)
		nih_ref (last_int32_array, data);
	last_int32_array_len = value_len;
}

void
test_new_int32_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;
	int32_t *       int32_array;
	size_t          int32_array_len;

	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewInt32Array_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FEATURE ("with array");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewInt32Array",
							(NihDBusSignalHandler)my_new_int32_array_handler,
							parent));

			int32_array = nih_alloc (parent, sizeof (int32_t) * 6);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;
			int32_array[4] = 23;
			int32_array[5] = 42;

			int32_array_len = 6;

			assert0 (my_test_emit_new_int32_array (
					 server_conn,
					 "/com/netsplit/Nih/Test",
					 int32_array,
					 int32_array_len));
		}

		new_int32_array_handled = FALSE;
		last_data = NULL;
		last_int32_array = NULL;
		last_int32_array_len = 0;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_int32_array_handled);
		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_int32_array_len, 6);
		TEST_ALLOC_SIZE (last_int32_array, sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (last_int32_array, parent);
		TEST_EQ (last_int32_array[0], 4);
		TEST_EQ (last_int32_array[1], 8);
		TEST_EQ (last_int32_array[2], 15);
		TEST_EQ (last_int32_array[3], 16);
		TEST_EQ (last_int32_array[4], 23);
		TEST_EQ (last_int32_array[5], 42);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that we can receive an empty array, which is actually
	 * NULL with a zero length.
	 */
	TEST_FEATURE ("with empty array");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewInt32Array",
							(NihDBusSignalHandler)my_new_int32_array_handler,
							parent));

			int32_array = NULL;
			int32_array_len = 0;

			assert0 (my_test_emit_new_int32_array (
					 server_conn,
					 "/com/netsplit/Nih/Test",
					 int32_array,
					 int32_array_len));
		}

		new_int32_array_handled = FALSE;
		last_data = NULL;
		last_int32_array = NULL;
		last_int32_array_len = 0;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_int32_array_handled);
		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_int32_array_len, 0);
		TEST_EQ_P (last_int32_array, NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_str_array_handled;

static void
my_new_str_array_handler (void *          data,
			  NihDBusMessage *message,
			  char * const *  value)
{
	new_str_array_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);
	TEST_ALLOC_PARENT (value, message);

	last_data = data;
	last_str_array = value;
	nih_ref (last_str_array, data);
}

void
test_new_str_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;
	char **         str_array;

	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewStrArray_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FEATURE ("with array");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewStrArray",
							(NihDBusSignalHandler)my_new_str_array_handler,
							parent));

			str_array = nih_alloc (parent, sizeof (char *) * 7);
			str_array[0] = "she";
			str_array[1] = "needs";
			str_array[2] = "more";
			str_array[3] = "of";
			str_array[4] = "ze";
			str_array[5] = "punishment";
			str_array[6] = NULL;

			assert0 (my_test_emit_new_str_array (
					 server_conn,
					 "/com/netsplit/Nih/Test",
					 str_array));
		}

		new_str_array_handled = FALSE;
		last_data = NULL;
		last_str_array = NULL;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_str_array_handled);
		TEST_EQ_P (last_data, parent);
		TEST_ALLOC_SIZE (last_str_array, sizeof (char *) * 7);
		TEST_ALLOC_PARENT (last_str_array, parent);
		TEST_EQ_STR (last_str_array[0], "she");
		TEST_ALLOC_PARENT (last_str_array[0], last_str_array);
		TEST_EQ_STR (last_str_array[1], "needs");
		TEST_ALLOC_PARENT (last_str_array[1], last_str_array);
		TEST_EQ_STR (last_str_array[2], "more");
		TEST_ALLOC_PARENT (last_str_array[2], last_str_array);
		TEST_EQ_STR (last_str_array[3], "of");
		TEST_ALLOC_PARENT (last_str_array[3], last_str_array);
		TEST_EQ_STR (last_str_array[4], "ze");
		TEST_ALLOC_PARENT (last_str_array[4], last_str_array);
		TEST_EQ_STR (last_str_array[5], "punishment");
		TEST_ALLOC_PARENT (last_str_array[5], last_str_array);
		TEST_EQ_P (last_str_array[6], NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that we can receive an empty string array consisting of
	 * only the NULL pointer.
	 */
	TEST_FEATURE ("with empty array");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewStrArray",
							(NihDBusSignalHandler)my_new_str_array_handler,
							parent));

			str_array = nih_alloc (parent, sizeof (char *) * 1);
			str_array[0] = NULL;

			assert0 (my_test_emit_new_str_array (
					 server_conn,
					 "/com/netsplit/Nih/Test",
					 str_array));
		}

		new_str_array_handled = FALSE;
		last_data = NULL;
		last_str_array = NULL;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_str_array_handled);
		TEST_EQ_P (last_data, parent);
		TEST_ALLOC_SIZE (last_str_array, sizeof (char *) * 1);
		TEST_ALLOC_PARENT (last_str_array, parent);
		TEST_EQ_P (last_str_array[0], NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int new_int32_array_array_handled;

static void
my_new_int32_array_array_handler (void *            data,
				  NihDBusMessage *  message,
				  int32_t * const *value,
				  size_t *         value_len)
{
	new_int32_array_array_handled++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);
	TEST_ALLOC_PARENT (value, message);

	if (*value) {
		TEST_NE_P (value_len, NULL);
		TEST_ALLOC_PARENT (value_len, value);
	}

	last_data = data;
	last_int32_array_array = value;
	nih_ref (last_int32_array_array, data);
	last_int32_array_array_len = value_len;
	if (value_len)
		nih_ref (last_int32_array_array_len, data);
}

void
test_new_int32_array_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusProxy *  proxy;
	void *          parent;
	int32_t **      int32_array_array;
	size_t *        int32_array_array_len;

	TEST_FUNCTION ("proxy_com_netsplit_Nih_Test_NewInt32ArrayArray_signal");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	/* Check that we can use nih_dbus_proxy_connect() to have our handler
	 * function called when the matching signal is emitted by the server.
	 */
	TEST_FEATURE ("with array");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewInt32ArrayArray",
							(NihDBusSignalHandler)my_new_int32_array_array_handler,
							parent));

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 3);
			int32_array_array_len = nih_alloc (parent, sizeof (size_t) * 2);

			int32_array_array[0] = nih_alloc (int32_array_array, sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;

			int32_array_array_len[0] = 6;

			int32_array_array[1] = nih_alloc (int32_array_array, sizeof (int32_t) * 6);
			int32_array_array[1][0] = 1;
			int32_array_array[1][1] = 1;
			int32_array_array[1][2] = 2;
			int32_array_array[1][3] = 3;
			int32_array_array[1][4] = 5;
			int32_array_array[1][5] = 8;

			int32_array_array_len[1] = 6;

			int32_array_array[2] = NULL;

			assert0 (my_test_emit_new_int32_array_array (
					 server_conn,
					 "/com/netsplit/Nih/Test",
					 int32_array_array,
					 int32_array_array_len));
		}

		new_int32_array_array_handled = FALSE;
		last_data = NULL;
		last_int32_array_array = NULL;
		last_int32_array_array_len = NULL;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_int32_array_array_handled);
		TEST_EQ_P (last_data, parent);
		TEST_ALLOC_SIZE (last_int32_array_array, sizeof (int32_t *) * 3);
		TEST_ALLOC_PARENT (last_int32_array_array, parent);

		TEST_ALLOC_SIZE (last_int32_array_array_len, sizeof (size_t) * 2);
		TEST_ALLOC_PARENT (last_int32_array_array_len, last_int32_array_array);

		TEST_EQ (last_int32_array_array_len[0], 6);
		TEST_ALLOC_SIZE (last_int32_array_array[0], sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (last_int32_array_array[0], last_int32_array_array);
		TEST_EQ (last_int32_array_array[0][0], 4);
		TEST_EQ (last_int32_array_array[0][1], 8);
		TEST_EQ (last_int32_array_array[0][2], 15);
		TEST_EQ (last_int32_array_array[0][3], 16);
		TEST_EQ (last_int32_array_array[0][4], 23);
		TEST_EQ (last_int32_array_array[0][5], 42);

		TEST_EQ (last_int32_array_array_len[1], 6);
		TEST_ALLOC_SIZE (last_int32_array_array[1], sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (last_int32_array_array[1], last_int32_array_array);
		TEST_EQ (last_int32_array_array[1][0], 1);
		TEST_EQ (last_int32_array_array[1][1], 1);
		TEST_EQ (last_int32_array_array[1][2], 2);
		TEST_EQ (last_int32_array_array[1][3], 3);
		TEST_EQ (last_int32_array_array[1][4], 5);
		TEST_EQ (last_int32_array_array[1][5], 8);

		TEST_EQ_P (last_int32_array_array[2], NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that we can receive an empty array which has NULL as its
	 * first element and NULL for its size array.
	 */
	TEST_FEATURE ("with array");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			assert (nih_dbus_proxy_connect (proxy, proxy,
							&proxy_com_netsplit_Nih_Test,
							"NewInt32ArrayArray",
							(NihDBusSignalHandler)my_new_int32_array_array_handler,
							parent));

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 1);
			int32_array_array_len = NULL;

			int32_array_array[0] = NULL;

			assert0 (my_test_emit_new_int32_array_array (
					 server_conn,
					 "/com/netsplit/Nih/Test",
					 int32_array_array,
					 int32_array_array_len));
		}

		new_int32_array_array_handled = FALSE;
		last_data = NULL;
		last_int32_array_array = NULL;
		last_int32_array_array_len = NULL;

		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (new_int32_array_array_handled);
		TEST_EQ_P (last_data, parent);
		TEST_ALLOC_SIZE (last_int32_array_array, sizeof (int32_t *) * 1);
		TEST_ALLOC_PARENT (last_int32_array_array, parent);

		TEST_EQ_P (last_int32_array_array[0], NULL);
		TEST_EQ_P (last_int32_array_array_len, NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();

}


static int get_byte_replied;

static void
my_get_byte_reply (void *          data,
		   NihDBusMessage *message,
		   uint8_t         value)
{
	get_byte_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_byte_value = value;
}

void
test_get_byte (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_byte");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		byte_property = 97;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_byte_replied = FALSE;
		last_data = NULL;
		last_byte_value = 0;

		pending_call = proxy_test_get_byte (proxy,
						    my_get_byte_reply,
						    my_error_handler,
						    parent,
						    0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_byte_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_byte_value, 97);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_byte_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_byte (proxy,
						    my_get_byte_reply,
						    my_error_handler,
						    parent,
						    50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_byte_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_byte_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_byte (proxy,
						    my_get_byte_reply,
						    my_error_handler,
						    parent,
						    50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_byte_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_byte_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	uint8_t         byte_value;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_byte_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		byte_property = 97;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		byte_value = 0;

		ret = proxy_test_get_byte_sync (parent, proxy,
						&byte_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (byte_value, 97);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_byte_replied;

static void
my_set_byte_reply (void *          data,
		   NihDBusMessage *message)
{
	set_byte_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_byte (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_byte");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		byte_property = 0;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_byte_replied = FALSE;
		last_data = NULL;
		last_byte_value = 0;

		pending_call = proxy_test_set_byte (proxy,
						    97,
						    my_set_byte_reply,
						    my_error_handler,
						    parent,
						    0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_byte_replied);

		TEST_EQ_P (last_data, parent);

		TEST_EQ (byte_property, 97);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_byte_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_byte (proxy,
						    0,
						    my_set_byte_reply,
						    my_error_handler,
						    parent,
						    0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_byte_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Byte.Zero");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_byte_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_byte (proxy,
						    4,
						    my_set_byte_reply,
						    my_error_handler,
						    parent,
						    0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_byte_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_byte_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_byte (proxy,
						    97,
						    my_set_byte_reply,
						    my_error_handler,
						    parent,
						    50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_byte_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_byte_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_byte (proxy,
						    97,
						    my_set_byte_reply,
						    my_error_handler,
						    parent,
						    50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_byte_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_byte_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	uint8_t         byte_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_byte_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		byte_property = 0;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		byte_value = 0;

		ret = proxy_test_set_byte_sync (parent, proxy, 97);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_byte_sync (parent, proxy,
							   &byte_value));
		}

		TEST_EQ (byte_value, 97);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_byte_sync (parent, proxy, 0);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Byte.Zero");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_byte_sync (parent, proxy, 4);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_boolean_replied;

static void
my_get_boolean_reply (void *          data,
		      NihDBusMessage *message,
		      int             value)
{
	get_boolean_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_boolean_value = value;
}

void
test_get_boolean (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_boolean");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		boolean_property = TRUE;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_boolean_replied = FALSE;
		last_data = NULL;
		last_boolean_value = 0;

		pending_call = proxy_test_get_boolean (proxy,
						       my_get_boolean_reply,
						       my_error_handler,
						       parent,
						       0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_boolean_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_boolean_value, TRUE);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_boolean_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_boolean (proxy,
						       my_get_boolean_reply,
						       my_error_handler,
						       parent,
						       50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_boolean_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_boolean_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_boolean (proxy,
						       my_get_boolean_reply,
						       my_error_handler,
						       parent,
						       50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_boolean_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_boolean_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int             boolean_value;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_boolean_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		boolean_property = TRUE;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		boolean_value = FALSE;

		ret = proxy_test_get_boolean_sync (parent, proxy,
						   &boolean_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (boolean_value, TRUE);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_boolean_replied;

static void
my_set_boolean_reply (void *          data,
		   NihDBusMessage *message)
{
	set_boolean_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_boolean (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_boolean");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		boolean_property = 0;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_boolean_replied = FALSE;
		last_data = NULL;
		last_boolean_value = FALSE;

		pending_call = proxy_test_set_boolean (proxy,
						       TRUE,
						       my_set_boolean_reply,
						       my_error_handler,
						       parent,
						       0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_boolean_replied);

		TEST_EQ_P (last_data, parent);

		TEST_EQ (boolean_property, TRUE);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_boolean_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_boolean (proxy,
						       FALSE,
						       my_set_boolean_reply,
						       my_error_handler,
						       parent,
						       0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_boolean_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Boolean.Zero");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_boolean_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_boolean (proxy,
						       TRUE,
						       my_set_boolean_reply,
						       my_error_handler,
						       parent,
						       50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_boolean_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_boolean_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_boolean (proxy,
						       TRUE,
						       my_set_boolean_reply,
						       my_error_handler,
						       parent,
						       50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_boolean_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_boolean_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int             boolean_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_boolean_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		boolean_property = 0;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		boolean_value = FALSE;

		ret = proxy_test_set_boolean_sync (parent, proxy, TRUE);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_boolean_sync (parent, proxy,
							      &boolean_value));
		}

		TEST_EQ (boolean_value, TRUE);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_boolean_sync (parent, proxy, FALSE);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Boolean.Zero");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_int16_replied;

static void
my_get_int16_reply (void *          data,
		    NihDBusMessage *message,
		    int16_t         value)
{
	get_int16_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_int16_value = value;
}

void
test_get_int16 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_int16");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		int16_property = -42;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int16_replied = FALSE;
		last_data = NULL;
		last_int16_value = 0;

		pending_call = proxy_test_get_int16 (proxy,
						     my_get_int16_reply,
						     my_error_handler,
						     parent,
						     0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_int16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_int16_value, -42);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_int16 (proxy,
						     my_get_int16_reply,
						     my_error_handler,
						     parent,
						     50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_int16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_int16 (proxy,
						    my_get_int16_reply,
						    my_error_handler,
						    parent,
						    50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_int16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_int16_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int16_t         int16_value;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_int16_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		int16_property = -42;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int16_value = 0;

		ret = proxy_test_get_int16_sync (parent, proxy,
						 &int16_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (int16_value, -42);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_int16_replied;

static void
my_set_int16_reply (void *          data,
		   NihDBusMessage *message)
{
	set_int16_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_int16 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_int16");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		int16_property = 0;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int16_replied = FALSE;
		last_data = NULL;
		last_int16_value = 0;

		pending_call = proxy_test_set_int16 (proxy,
						     -42,
						     my_set_int16_reply,
						     my_error_handler,
						     parent,
						     0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_int16_replied);

		TEST_EQ_P (last_data, parent);

		TEST_EQ (int16_property, -42);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int16 (proxy,
						     0,
						     my_set_int16_reply,
						     my_error_handler,
						     parent,
						     0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int16.Zero");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int16 (proxy,
						     4,
						     my_set_int16_reply,
						     my_error_handler,
						     parent,
						     0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int16 (proxy,
						     -42,
						     my_set_int16_reply,
						     my_error_handler,
						     parent,
						     50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int16 (proxy,
						     -42,
						     my_set_int16_reply,
						     my_error_handler,
						     parent,
						     50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_int16_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int16_t         int16_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_int16_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		int16_property = 0;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int16_value = 0;

		ret = proxy_test_set_int16_sync (parent, proxy, -42);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_int16_sync (parent, proxy,
							    &int16_value));
		}

		TEST_EQ (int16_value, -42);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_int16_sync (parent, proxy, 0);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int16.Zero");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_int16_sync (parent, proxy, 4);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_uint16_replied;

static void
my_get_uint16_reply (void *          data,
		     NihDBusMessage *message,
		     uint16_t        value)
{
	get_uint16_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_uint16_value = value;
}

void
test_get_uint16 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_uint16");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		uint16_property = 42;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_uint16_replied = FALSE;
		last_data = NULL;
		last_uint16_value = 0;

		pending_call = proxy_test_get_uint16 (proxy,
						      my_get_uint16_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_uint16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_uint16_value, 42);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_uint16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_uint16 (proxy,
						      my_get_uint16_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_uint16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_uint16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_uint16 (proxy,
						      my_get_uint16_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_uint16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_uint16_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	uint16_t        uint16_value;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_uint16_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		uint16_property = 42;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		uint16_value = 0;

		ret = proxy_test_get_uint16_sync (parent, proxy,
						 &uint16_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (uint16_value, 42);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_uint16_replied;

static void
my_set_uint16_reply (void *          data,
		     NihDBusMessage *message)
{
	set_uint16_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_uint16 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_uint16");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		uint16_property = 0;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint16_replied = FALSE;
		last_data = NULL;
		last_uint16_value = 0;

		pending_call = proxy_test_set_uint16 (proxy,
						      42,
						      my_set_uint16_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_uint16_replied);

		TEST_EQ_P (last_data, parent);

		TEST_EQ (uint16_property, 42);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_uint16 (proxy,
						      0,
						      my_set_uint16_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_uint16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.UInt16.Zero");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_uint16 (proxy,
						      4,
						      my_set_uint16_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_uint16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_uint16 (proxy,
						      42,
						      my_set_uint16_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_uint16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint16_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_uint16 (proxy,
						      42,
						      my_set_uint16_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_uint16_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_uint16_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	uint16_t        uint16_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_uint16_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		uint16_property = 0;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		uint16_value = 0;

		ret = proxy_test_set_uint16_sync (parent, proxy, 42);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_uint16_sync (parent, proxy,
							    &uint16_value));
		}

		TEST_EQ (uint16_value, 42);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_uint16_sync (parent, proxy, 0);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.UInt16.Zero");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_uint16_sync (parent, proxy, 4);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_int32_replied;

static void
my_get_int32_reply (void *          data,
		    NihDBusMessage *message,
		    int32_t         value)
{
	get_int32_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_int32_value = value;
}

void
test_get_int32 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_int32");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		int32_property = -1048576;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int32_replied = FALSE;
		last_data = NULL;
		last_int32_value = 0;

		pending_call = proxy_test_get_int32 (proxy,
						     my_get_int32_reply,
						     my_error_handler,
						     parent,
						     0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_int32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_int32_value, -1048576);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_int32 (proxy,
						     my_get_int32_reply,
						     my_error_handler,
						     parent,
						     50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_int32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_int32 (proxy,
						    my_get_int32_reply,
						    my_error_handler,
						    parent,
						    50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_int32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_int32_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int32_t         int32_value;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_int32_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		int32_property = -1048576;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int32_value = 0;

		ret = proxy_test_get_int32_sync (parent, proxy,
						 &int32_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (int32_value, -1048576);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_int32_replied;

static void
my_set_int32_reply (void *          data,
		   NihDBusMessage *message)
{
	set_int32_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_int32 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_int32");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		int32_property = 0;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int32_replied = FALSE;
		last_data = NULL;
		last_int32_value = 0;

		pending_call = proxy_test_set_int32 (proxy,
						     -1048576,
						     my_set_int32_reply,
						     my_error_handler,
						     parent,
						     0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_int32_replied);

		TEST_EQ_P (last_data, parent);

		TEST_EQ (int32_property, -1048576);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int32 (proxy,
						     0,
						     my_set_int32_reply,
						     my_error_handler,
						     parent,
						     0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int32.Zero");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int32 (proxy,
						     4,
						     my_set_int32_reply,
						     my_error_handler,
						     parent,
						     0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int32 (proxy,
						     -1048576,
						     my_set_int32_reply,
						     my_error_handler,
						     parent,
						     50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int32 (proxy,
						     -1048576,
						     my_set_int32_reply,
						     my_error_handler,
						     parent,
						     50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_int32_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int32_t         int32_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_int32_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		int32_property = 0;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int32_value = 0;

		ret = proxy_test_set_int32_sync (parent, proxy, -1048576);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_int32_sync (parent, proxy,
							    &int32_value));
		}

		TEST_EQ (int32_value, -1048576);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_int32_sync (parent, proxy, 0);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int32.Zero");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_int32_sync (parent, proxy, 4);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_uint32_replied;

static void
my_get_uint32_reply (void *          data,
		     NihDBusMessage *message,
		     uint32_t        value)
{
	get_uint32_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_uint32_value = value;
}

void
test_get_uint32 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_uint32");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		uint32_property = 1048576;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_uint32_replied = FALSE;
		last_data = NULL;
		last_uint32_value = 0;

		pending_call = proxy_test_get_uint32 (proxy,
						      my_get_uint32_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_uint32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_uint32_value, 1048576);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_uint32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_uint32 (proxy,
						      my_get_uint32_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_uint32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_uint32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_uint32 (proxy,
						      my_get_uint32_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_uint32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_uint32_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	uint32_t        uint32_value;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_uint32_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		uint32_property = 1048576;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		uint32_value = 0;

		ret = proxy_test_get_uint32_sync (parent, proxy,
						 &uint32_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (uint32_value, 1048576);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_uint32_replied;

static void
my_set_uint32_reply (void *          data,
		     NihDBusMessage *message)
{
	set_uint32_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_uint32 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_uint32");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		uint32_property = 0;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint32_replied = FALSE;
		last_data = NULL;
		last_uint32_value = 0;

		pending_call = proxy_test_set_uint32 (proxy,
						      1048576,
						      my_set_uint32_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_uint32_replied);

		TEST_EQ_P (last_data, parent);

		TEST_EQ (uint32_property, 1048576);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_uint32 (proxy,
						      0,
						      my_set_uint32_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_uint32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.UInt32.Zero");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_uint32 (proxy,
						      4,
						      my_set_uint32_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_uint32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_uint32 (proxy,
						      1048576,
						      my_set_uint32_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_uint32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint32_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_uint32 (proxy,
						      1048576,
						      my_set_uint32_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_uint32_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_uint32_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	uint32_t        uint32_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_uint32_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		uint32_property = 0;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		uint32_value = 0;

		ret = proxy_test_set_uint32_sync (parent, proxy, 1048576);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_uint32_sync (parent, proxy,
							    &uint32_value));
		}

		TEST_EQ (uint32_value, 1048576);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_uint32_sync (parent, proxy, 0);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.UInt32.Zero");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_uint32_sync (parent, proxy, 4);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_int64_replied;

static void
my_get_int64_reply (void *          data,
		    NihDBusMessage *message,
		    int64_t         value)
{
	get_int64_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_int64_value = value;
}

void
test_get_int64 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_int64");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		int64_property = -4815162342;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int64_replied = FALSE;
		last_data = NULL;
		last_int64_value = 0;

		pending_call = proxy_test_get_int64 (proxy,
						     my_get_int64_reply,
						     my_error_handler,
						     parent,
						     0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_int64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_int64_value, -4815162342);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_int64 (proxy,
						     my_get_int64_reply,
						     my_error_handler,
						     parent,
						     50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_int64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_int64 (proxy,
						    my_get_int64_reply,
						    my_error_handler,
						    parent,
						    50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_int64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_int64_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int64_t         int64_value;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_int64_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		int64_property = -4815162342;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int64_value = 0;

		ret = proxy_test_get_int64_sync (parent, proxy,
						 &int64_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (int64_value, -4815162342);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_int64_replied;

static void
my_set_int64_reply (void *          data,
		   NihDBusMessage *message)
{
	set_int64_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_int64 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_int64");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		int64_property = 0;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int64_replied = FALSE;
		last_data = NULL;
		last_int64_value = 0;

		pending_call = proxy_test_set_int64 (proxy,
						     -4815162342L,
						     my_set_int64_reply,
						     my_error_handler,
						     parent,
						     0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_int64_replied);

		TEST_EQ_P (last_data, parent);

		TEST_EQ (int64_property, -4815162342);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int64 (proxy,
						     0,
						     my_set_int64_reply,
						     my_error_handler,
						     parent,
						     0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int64.Zero");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int64 (proxy,
						     4,
						     my_set_int64_reply,
						     my_error_handler,
						     parent,
						     0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int64 (proxy,
						     -4815162342L,
						     my_set_int64_reply,
						     my_error_handler,
						     parent,
						     50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int64 (proxy,
						     -4815162342L,
						     my_set_int64_reply,
						     my_error_handler,
						     parent,
						     50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_int64_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int64_t         int64_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_int64_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		int64_property = 0;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int64_value = 0;

		ret = proxy_test_set_int64_sync (parent, proxy, -4815162342);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_int64_sync (parent, proxy,
							    &int64_value));
		}

		TEST_EQ (int64_value, -4815162342);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_int64_sync (parent, proxy, 0);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int64.Zero");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_int64_sync (parent, proxy, 4);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_uint64_replied;

static void
my_get_uint64_reply (void *          data,
		     NihDBusMessage *message,
		     uint64_t        value)
{
	get_uint64_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_uint64_value = value;
}

void
test_get_uint64 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_uint64");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		uint64_property = 4815162342;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_uint64_replied = FALSE;
		last_data = NULL;
		last_uint64_value = 0;

		pending_call = proxy_test_get_uint64 (proxy,
						      my_get_uint64_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_uint64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_uint64_value, 4815162342);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_uint64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_uint64 (proxy,
						      my_get_uint64_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_uint64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_uint64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_uint64 (proxy,
						      my_get_uint64_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_uint64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_uint64_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	uint64_t        uint64_value;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_uint64_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		uint64_property = 4815162342;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		uint64_value = 0;

		ret = proxy_test_get_uint64_sync (parent, proxy,
						 &uint64_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (uint64_value, 4815162342);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_uint64_replied;

static void
my_set_uint64_reply (void *          data,
		     NihDBusMessage *message)
{
	set_uint64_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_uint64 (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_uint64");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		uint64_property = 0;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint64_replied = FALSE;
		last_data = NULL;
		last_uint64_value = 0;

		pending_call = proxy_test_set_uint64 (proxy,
						      4815162342L,
						      my_set_uint64_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_uint64_replied);

		TEST_EQ_P (last_data, parent);

		TEST_EQ (uint64_property, 4815162342);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_uint64 (proxy,
						      0,
						      my_set_uint64_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_uint64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.UInt64.Zero");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_uint64 (proxy,
						      4,
						      my_set_uint64_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_uint64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_uint64 (proxy,
						      4815162342L,
						      my_set_uint64_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_uint64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_uint64_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_uint64 (proxy,
						      4815162342L,
						      my_set_uint64_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_uint64_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_uint64_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	uint64_t        uint64_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_uint64_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		uint64_property = 0;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		uint64_value = 0;

		ret = proxy_test_set_uint64_sync (parent, proxy, 4815162342);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_uint64_sync (parent, proxy,
							    &uint64_value));
		}

		TEST_EQ (uint64_value, 4815162342);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_uint64_sync (parent, proxy, 0);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.UInt64.Zero");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_uint64_sync (parent, proxy, 4);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_double_replied;

static void
my_get_double_reply (void *          data,
		     NihDBusMessage *message,
		     double          value)
{
	get_double_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
	last_double_value = value;
}

void
test_get_double (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_double");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		double_property = 3.141597;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_double_replied = FALSE;
		last_data = NULL;
		last_double_value = 0;

		pending_call = proxy_test_get_double (proxy,
						      my_get_double_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_double_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_double_value, 3.141597);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_double_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_double (proxy,
						      my_get_double_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_double_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_double_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_double (proxy,
						      my_get_double_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_double_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_double_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	double          double_value;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_double_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		double_property = 3.141597;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		double_value = 0;

		ret = proxy_test_get_double_sync (parent, proxy,
						  &double_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (double_value, 3.141597);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_double_replied;

static void
my_set_double_reply (void *          data,
		   NihDBusMessage *message)
{
	set_double_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_double (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_double");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		double_property = 0;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_double_replied = FALSE;
		last_data = NULL;
		last_double_value = 0;

		pending_call = proxy_test_set_double (proxy,
						      3.141597,
						      my_set_double_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_double_replied);

		TEST_EQ_P (last_data, parent);

		TEST_EQ (double_property, 3.141597);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_double_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_double (proxy,
						      0,
						      my_set_double_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_double_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Double.Zero");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_double_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_double (proxy,
						      4,
						      my_set_double_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_double_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_double_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_double (proxy,
						      3.141597,
						      my_set_double_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_double_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_double_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_double (proxy,
						      3.141597,
						      my_set_double_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_double_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_double_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	double          double_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_double_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		double_property = 0;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		double_value = 0;

		ret = proxy_test_set_double_sync (parent, proxy, 3.141597);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_double_sync (parent, proxy,
							     &double_value));
		}

		TEST_EQ (double_value, 3.141597);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_double_sync (parent, proxy, 0);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Double.Zero");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_double_sync (parent, proxy, 4);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_string_replied;

static void
my_get_string_reply (void *          data,
		     NihDBusMessage *message,
		     const char *    value)
{
	get_string_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);
	TEST_ALLOC_PARENT (value, message);

	last_data = data;
	last_str_value = value;
	nih_ref (last_str_value, last_data);
}

void
test_get_string (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_string");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		str_property = "she needs more of ze punishment";

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_string_replied = FALSE;
		last_data = NULL;
		last_str_value = FALSE;

		pending_call = proxy_test_get_string (proxy,
						      my_get_string_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_string_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "she needs more of ze punishment");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_string_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_string (proxy,
						      my_get_string_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_string_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_string_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_string (proxy,
						      my_get_string_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_string_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_string_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_string_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		str_property = "she needs more of ze punishment";

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_get_string_sync (parent, proxy,
						  &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ_STR (str_value, "she needs more of ze punishment");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_string_replied;

static void
my_set_string_reply (void *          data,
		     NihDBusMessage *message)
{
	set_string_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_string (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_string");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		str_property = NULL;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_string_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_set_string (proxy,
						      "she needs more of ze punishment",
						      my_set_string_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_string_replied);

		TEST_EQ_P (last_data, parent);

		TEST_EQ_STR (str_property, "she needs more of ze punishment");
		nih_free (str_property);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_string_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_string (proxy,
						      "",
						      my_set_string_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_string_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.String.Empty");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_string_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_string (proxy,
						      "invalid",
						      my_set_string_reply,
						      my_error_handler,
						      parent,
						      0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_string_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_string_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_string (proxy,
						      "she needs more of ze punishment",
						      my_set_string_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_string_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_string_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_string (proxy,
						      "she needs more of ze punishment",
						      my_set_string_reply,
						      my_error_handler,
						      parent,
						      50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_string_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_string_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_string_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		str_property = NULL;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		if (str_property)
			nih_free (str_property);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_set_string_sync (parent, proxy,
						  "she needs more of ze punishment");

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_string_sync (parent, proxy,
							     &str_value));
		}

		TEST_EQ_STR (str_value, "she needs more of ze punishment");
		TEST_ALLOC_PARENT (str_value, NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_string_sync (parent, proxy, "");

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.String.Empty");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_string_sync (parent, proxy, "invalid");

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_object_path_replied;

static void
my_get_object_path_reply (void *          data,
			  NihDBusMessage *message,
			  const char *    value)
{
	get_object_path_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);
	TEST_ALLOC_PARENT (value, message);

	last_data = data;
	last_str_value = value;
	nih_ref (last_str_value, last_data);
}

void
test_get_object_path (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_object_path");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		object_path_property = "/com/netsplit/Nih/Test";

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_object_path_replied = FALSE;
		last_data = NULL;
		last_str_value = FALSE;

		pending_call = proxy_test_get_object_path (proxy,
							   my_get_object_path_reply,
							   my_error_handler,
							   parent,
							   0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_object_path_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "/com/netsplit/Nih/Test");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_object_path_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_object_path (proxy,
							   my_get_object_path_reply,
							   my_error_handler,
							   parent,
							   50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_object_path_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_object_path_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_object_path (proxy,
							   my_get_object_path_reply,
							   my_error_handler,
							   parent,
							   50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_object_path_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_object_path_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_object_path_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		object_path_property = "/com/netsplit/Nih/Test";

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_get_object_path_sync (parent, proxy,
						       &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ_STR (str_value, "/com/netsplit/Nih/Test");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_object_path_replied;

static void
my_set_object_path_reply (void *          data,
			  NihDBusMessage *message)
{
	set_object_path_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_object_path (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_object_path");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		object_path_property = NULL;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_object_path_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_set_object_path (proxy,
							   "/com/netsplit/Nih/Test",
							   my_set_object_path_reply,
							   my_error_handler,
							   parent,
							   0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_object_path_replied);

		TEST_EQ_P (last_data, parent);

		TEST_EQ_STR (object_path_property, "/com/netsplit/Nih/Test");
		nih_free (object_path_property);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_object_path_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_object_path (proxy,
							   "/",
							   my_set_object_path_reply,
							   my_error_handler,
							   parent,
							   0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_object_path_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.ObjectPath.Empty");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_object_path_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_object_path (proxy,
							   "/invalid",
							   my_set_object_path_reply,
							   my_error_handler,
							   parent,
							   0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_object_path_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_object_path_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_object_path (proxy,
							   "/com/netsplit/Nih/Test",
							   my_set_object_path_reply,
							   my_error_handler,
							   parent,
							   50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_object_path_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_object_path_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_object_path (proxy,
							   "/com/netsplit/Nih/Test",
							   my_set_object_path_reply,
							   my_error_handler,
							   parent,
							   50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_object_path_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_object_path_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_object_path_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		object_path_property = NULL;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		if (object_path_property)
			nih_free (object_path_property);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_set_object_path_sync (parent, proxy,
						       "/com/netsplit/Nih/Test");

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_object_path_sync (parent, proxy,
								  &str_value));
		}

		TEST_EQ_STR (str_value, "/com/netsplit/Nih/Test");
		TEST_ALLOC_PARENT (str_value, NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_object_path_sync (parent, proxy, "/");

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.ObjectPath.Empty");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_object_path_sync (parent, proxy,
						       "/invalid");

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_signature_replied;

static void
my_get_signature_reply (void *          data,
		     NihDBusMessage *message,
		     const char *    value)
{
	get_signature_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);
	TEST_ALLOC_PARENT (value, message);

	last_data = data;
	last_str_value = value;
	nih_ref (last_str_value, last_data);
}

void
test_get_signature (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_signature");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		signature_property = "a(ib)";

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_signature_replied = FALSE;
		last_data = NULL;
		last_str_value = FALSE;

		pending_call = proxy_test_get_signature (proxy,
							 my_get_signature_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_signature_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ_STR (last_str_value, "a(ib)");
		TEST_ALLOC_PARENT (last_str_value, parent);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_signature_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_signature (proxy,
							 my_get_signature_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_signature_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_signature_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_signature (proxy,
							 my_get_signature_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_signature_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_signature_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_signature_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		signature_property = "a(ib)";

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_get_signature_sync (parent, proxy,
						     &str_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ_STR (str_value, "a(ib)");
		TEST_ALLOC_PARENT (str_value, parent);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_signature_replied;

static void
my_set_signature_reply (void *          data,
		     NihDBusMessage *message)
{
	set_signature_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_signature (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_signature");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		signature_property = NULL;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_signature_replied = FALSE;
		last_data = NULL;
		last_str_value = NULL;

		pending_call = proxy_test_set_signature (proxy,
							 "a(ib)",
							 my_set_signature_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_signature_replied);

		TEST_EQ_P (last_data, parent);

		TEST_EQ_STR (signature_property, "a(ib)");
		nih_free (signature_property);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_signature_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_signature (proxy,
							 "",
							 my_set_signature_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_signature_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Signature.Empty");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_signature_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_signature (proxy,
							 "inva(x)id",
							 my_set_signature_reply,
							 my_error_handler,
							 parent,
							 0);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_signature_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_signature_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_signature (proxy,
							 "a(ib)",
							 my_set_signature_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_signature_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_signature_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_signature (proxy,
							 "a(ib)",
							 my_set_signature_reply,
							 my_error_handler,
							 parent,
							 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_signature_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_signature_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char *          str_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_signature_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		signature_property = NULL;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		if (signature_property)
			nih_free (signature_property);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_value = NULL;

		ret = proxy_test_set_signature_sync (parent, proxy,
						     "a(ib)");

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_signature_sync (parent, proxy,
								&str_value));
		}

		TEST_EQ_STR (str_value, "a(ib)");
		TEST_ALLOC_PARENT (str_value, NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_signature_sync (parent, proxy, "");

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Signature.Empty");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_signature_sync (parent, proxy,
						     "inva(x)id");

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_int32_array_replied;

static void
my_get_int32_array_reply (void *          data,
			  NihDBusMessage *message,
			  const int32_t * value,
			  size_t          value_len)
{
	get_int32_array_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	if (value_len) {
		TEST_NE_P (value, NULL);
		TEST_ALLOC_PARENT (value, message);
	}

	last_data = data;
	last_int32_array = value;
	if (value)
		nih_ref (last_int32_array, data);
	last_int32_array_len = value_len;
}

void
test_get_int32_array (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_int32_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			int32_array_property = nih_alloc (NULL, sizeof (int32_t) * 6);
			int32_array_property[0] = 4;
			int32_array_property[1] = 8;
			int32_array_property[2] = 15;
			int32_array_property[3] = 16;
			int32_array_property[4] = 23;
			int32_array_property[5] = 42;

			int32_array_property_len = 6;

			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int32_array_replied = FALSE;
		last_data = NULL;
		last_int32_array = NULL;
		last_int32_array_len = 0;

		pending_call = proxy_test_get_int32_array (
			proxy,
			my_get_int32_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			nih_free (int32_array_property);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_int32_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (last_int32_array_len, 6);
		TEST_ALLOC_SIZE (last_int32_array, sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (last_int32_array, parent);
		TEST_EQ (last_int32_array[0], 4);
		TEST_EQ (last_int32_array[1], 8);
		TEST_EQ (last_int32_array[2], 15);
		TEST_EQ (last_int32_array[3], 16);
		TEST_EQ (last_int32_array[4], 23);
		TEST_EQ (last_int32_array[5], 42);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
		nih_free (int32_array_property);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int32_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_int32_array (
			proxy,
			my_get_int32_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_int32_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int32_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_int32_array (
			proxy,
			my_get_int32_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_int32_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_int32_array_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int32_t *       int32_array;
	size_t          int32_array_len;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_int32_array_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		int32_array_property = nih_alloc (NULL, sizeof (int32_t) * 6);
		int32_array_property[0] = 4;
		int32_array_property[1] = 8;
		int32_array_property[2] = 15;
		int32_array_property[3] = 16;
		int32_array_property[4] = 23;
		int32_array_property[5] = 42;

		int32_array_property_len = 6;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		nih_free (int32_array_property);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int32_array = NULL;
		int32_array_len = 0;

		ret = proxy_test_get_int32_array_sync (parent, proxy,
						       &int32_array,
						       &int32_array_len);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (int32_array_len, 6);
		TEST_ALLOC_SIZE (int32_array, sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (int32_array, parent);
		TEST_EQ (int32_array[0], 4);
		TEST_EQ (int32_array[1], 8);
		TEST_EQ (int32_array[2], 15);
		TEST_EQ (int32_array[3], 16);
		TEST_EQ (int32_array[4], 23);
		TEST_EQ (int32_array[5], 42);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_int32_array_replied;

static void
my_set_int32_array_reply (void *          data,
			  NihDBusMessage *message)
{
	set_int32_array_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_int32_array (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	int32_t *        int32_array;
	size_t           int32_array_len;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_int32_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		int32_array_property = NULL;
		int32_array_property_len = 0;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array = nih_alloc (parent, sizeof (int32_t) * 6);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;
			int32_array[4] = 23;
			int32_array[5] = 42;

			int32_array_len = 6;
		}

		error_handler_called = FALSE;
		set_int32_array_replied = FALSE;
		last_data = NULL;
		last_int32_array = NULL;
		last_int32_array_len = 0;

		pending_call = proxy_test_set_int32_array (
			proxy,
			int32_array, int32_array_len,
			my_set_int32_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_int32_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_EQ (int32_array_property_len, 6);
		TEST_ALLOC_SIZE (int32_array_property, sizeof (int32_t) * 6);
		TEST_EQ (int32_array_property[0], 4);
		TEST_EQ (int32_array_property[1], 8);
		TEST_EQ (int32_array_property[2], 15);
		TEST_EQ (int32_array_property[3], 16);
		TEST_EQ (int32_array_property[4], 23);
		TEST_EQ (int32_array_property[5], 42);

		nih_free (int32_array_property);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		set_int32_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int32_array (
			proxy,
			NULL, 0,
			my_set_int32_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int32_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int32Array.Empty");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array = nih_alloc (parent, sizeof (int32_t) * 4);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;

			int32_array_len = 4;
		}

		error_handler_called = FALSE;
		set_int32_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int32_array (
			proxy,
			int32_array, int32_array_len,
			my_set_int32_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int32_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array = nih_alloc (parent, sizeof (int32_t) * 6);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;
			int32_array[4] = 23;
			int32_array[5] = 42;

			int32_array_len = 6;
		}

		error_handler_called = FALSE;
		set_int32_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int32_array (
			proxy,
			int32_array, int32_array_len,
			my_set_int32_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int32_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array = nih_alloc (parent, sizeof (int32_t) * 6);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;
			int32_array[4] = 23;
			int32_array[5] = 42;

			int32_array_len = 6;
		}

		error_handler_called = FALSE;
		set_int32_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int32_array (
			proxy,
			int32_array, int32_array_len,
			my_set_int32_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int32_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_int32_array_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int32_t *       int32_array;
	size_t          int32_array_len;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_int32_array_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		int32_array_property = NULL;
		int32_array_property_len = 0;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		if (int32_array_property)
			nih_free (int32_array_property);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array = nih_alloc (parent, sizeof (int32_t) * 6);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;
			int32_array[4] = 23;
			int32_array[5] = 42;

			int32_array_len = 6;
		}

		ret = proxy_test_set_int32_array_sync (parent, proxy,
						       int32_array, int32_array_len);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		nih_free (int32_array);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_int32_array_sync (
					 parent, proxy,
					 &int32_array, &int32_array_len));
		}

		TEST_EQ (int32_array_len, 6);
		TEST_ALLOC_SIZE (int32_array, sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (int32_array, parent);
		TEST_EQ (int32_array[0], 4);
		TEST_EQ (int32_array[1], 8);
		TEST_EQ (int32_array[2], 15);
		TEST_EQ (int32_array[3], 16);
		TEST_EQ (int32_array[4], 23);
		TEST_EQ (int32_array[5], 42);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		ret = proxy_test_set_int32_array_sync (parent, proxy, NULL, 0);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int32Array.Empty");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array = nih_alloc (parent, sizeof (int32_t) * 4);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;

			int32_array_len = 4;
		}

		ret = proxy_test_set_int32_array_sync (parent, proxy,
						       int32_array, int32_array_len);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_str_array_replied;

static void
my_get_str_array_reply (void *          data,
			NihDBusMessage *message,
			char * const *  value)
{
	get_str_array_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);
	TEST_ALLOC_PARENT (value, message);

	last_data = data;
	last_str_array = value;
	nih_ref (last_str_array, data);
}

void
test_get_str_array (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_str_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str_array_property = nih_alloc (NULL, sizeof (char *) * 7);
			str_array_property[0] = "she";
			str_array_property[1] = "needs";
			str_array_property[2] = "more";
			str_array_property[3] = "of";
			str_array_property[4] = "ze";
			str_array_property[5] = "punishment";
			str_array_property[6] = NULL;

			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_str_array_replied = FALSE;
		last_data = NULL;
		last_str_array = NULL;

		pending_call = proxy_test_get_str_array (
			proxy,
			my_get_str_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			nih_free (str_array_property);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_str_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_ALLOC_SIZE (last_str_array, sizeof (char *) * 7);
		TEST_ALLOC_PARENT (last_str_array, parent);
		TEST_EQ_STR (last_str_array[0], "she");
		TEST_ALLOC_PARENT (last_str_array[0], last_str_array);
		TEST_EQ_STR (last_str_array[1], "needs");
		TEST_ALLOC_PARENT (last_str_array[1], last_str_array);
		TEST_EQ_STR (last_str_array[2], "more");
		TEST_ALLOC_PARENT (last_str_array[2], last_str_array);
		TEST_EQ_STR (last_str_array[3], "of");
		TEST_ALLOC_PARENT (last_str_array[3], last_str_array);
		TEST_EQ_STR (last_str_array[4], "ze");
		TEST_ALLOC_PARENT (last_str_array[4], last_str_array);
		TEST_EQ_STR (last_str_array[5], "punishment");
		TEST_ALLOC_PARENT (last_str_array[5], last_str_array);
		TEST_EQ_P (last_str_array[6], NULL);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
		nih_free (str_array_property);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_str_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_str_array (
			proxy,
			my_get_str_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_str_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_str_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_str_array (
			proxy,
			my_get_str_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_str_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_str_array_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char **         str_array;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_str_array_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		str_array_property = nih_alloc (NULL, sizeof (char *) * 7);
		str_array_property[0] = "she";
		str_array_property[1] = "needs";
		str_array_property[2] = "more";
		str_array_property[3] = "of";
		str_array_property[4] = "ze";
		str_array_property[5] = "punishment";
		str_array_property[6] = NULL;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		nih_free (str_array_property);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		str_array = NULL;

		ret = proxy_test_get_str_array_sync (parent, proxy,
						     &str_array);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_SIZE (str_array, sizeof (char *) * 7);
		TEST_ALLOC_PARENT (str_array, parent);
		TEST_EQ_STR (str_array[0], "she");
		TEST_ALLOC_PARENT (str_array[0], str_array);
		TEST_EQ_STR (str_array[1], "needs");
		TEST_ALLOC_PARENT (str_array[1], str_array);
		TEST_EQ_STR (str_array[2], "more");
		TEST_ALLOC_PARENT (str_array[2], str_array);
		TEST_EQ_STR (str_array[3], "of");
		TEST_ALLOC_PARENT (str_array[3], str_array);
		TEST_EQ_STR (str_array[4], "ze");
		TEST_ALLOC_PARENT (str_array[4], str_array);
		TEST_EQ_STR (str_array[5], "punishment");
		TEST_ALLOC_PARENT (str_array[5], str_array);
		TEST_EQ_P (str_array[6], NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_str_array_replied;

static void
my_set_str_array_reply (void *          data,
			NihDBusMessage *message)
{
	set_str_array_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_str_array (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	char **          str_array;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_str_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		str_array_property = NULL;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 7);
			str_array[0] = "she";
			str_array[1] = "needs";
			str_array[2] = "more";
			str_array[3] = "of";
			str_array[4] = "ze";
			str_array[5] = "punishment";
			str_array[6] = NULL;
		}

		error_handler_called = FALSE;
		set_str_array_replied = FALSE;
		last_data = NULL;
		last_str_array = NULL;

		pending_call = proxy_test_set_str_array (
			proxy,
			str_array,
			my_set_str_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_str_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_ALLOC_SIZE (str_array_property, sizeof (char *) * 7);
		TEST_EQ_STR (str_array_property[0], "she");
		TEST_ALLOC_PARENT (str_array_property[0], str_array_property);
		TEST_EQ_STR (str_array_property[1], "needs");
		TEST_ALLOC_PARENT (str_array_property[1], str_array_property);
		TEST_EQ_STR (str_array_property[2], "more");
		TEST_ALLOC_PARENT (str_array_property[2], str_array_property);
		TEST_EQ_STR (str_array_property[3], "of");
		TEST_ALLOC_PARENT (str_array_property[3], str_array_property);
		TEST_EQ_STR (str_array_property[4], "ze");
		TEST_ALLOC_PARENT (str_array_property[4], str_array_property);
		TEST_EQ_STR (str_array_property[5], "punishment");
		TEST_ALLOC_PARENT (str_array_property[5], str_array_property);
		TEST_EQ_P (str_array_property[6], NULL);

		nih_free (str_array_property);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 1);
			str_array[0] = NULL;
		}

		error_handler_called = FALSE;
		set_str_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_str_array (
			proxy,
			str_array,
			my_set_str_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_str_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrArray.Empty");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 5);
			str_array[0] = "this";
			str_array[1] = "is";
			str_array[2] = "a";
			str_array[3] = "test";
			str_array[4] = NULL;
		}

		error_handler_called = FALSE;
		set_str_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_str_array (
			proxy,
			str_array,
			my_set_str_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_str_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 7);
			str_array[0] = "she";
			str_array[1] = "needs";
			str_array[2] = "more";
			str_array[3] = "of";
			str_array[4] = "ze";
			str_array[5] = "punishment";
			str_array[6] = NULL;
		}

		error_handler_called = FALSE;
		set_str_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_str_array (
			proxy,
			str_array,
			my_set_str_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_str_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 7);
			str_array[0] = "she";
			str_array[1] = "needs";
			str_array[2] = "more";
			str_array[3] = "of";
			str_array[4] = "ze";
			str_array[5] = "punishment";
			str_array[6] = NULL;
		}

		error_handler_called = FALSE;
		set_str_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_str_array (
			proxy,
			str_array,
			my_set_str_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_str_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_str_array_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	char **         str_array;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_str_array_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		str_array_property = NULL;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		if (str_array_property)
			nih_free (str_array_property);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 7);
			str_array[0] = "she";
			str_array[1] = "needs";
			str_array[2] = "more";
			str_array[3] = "of";
			str_array[4] = "ze";
			str_array[5] = "punishment";
			str_array[6] = NULL;
		}

		ret = proxy_test_set_str_array_sync (parent, proxy,
						     str_array);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		nih_free (str_array);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_str_array_sync (
					 parent, proxy,
					 &str_array));
		}

		TEST_ALLOC_SIZE (str_array, sizeof (char *) * 7);
		TEST_ALLOC_PARENT (str_array, parent);
		TEST_EQ_STR (str_array[0], "she");
		TEST_ALLOC_PARENT (str_array[0], str_array);
		TEST_EQ_STR (str_array[1], "needs");
		TEST_ALLOC_PARENT (str_array[1], str_array);
		TEST_EQ_STR (str_array[2], "more");
		TEST_ALLOC_PARENT (str_array[2], str_array);
		TEST_EQ_STR (str_array[3], "of");
		TEST_ALLOC_PARENT (str_array[3], str_array);
		TEST_EQ_STR (str_array[4], "ze");
		TEST_ALLOC_PARENT (str_array[4], str_array);
		TEST_EQ_STR (str_array[5], "punishment");
		TEST_ALLOC_PARENT (str_array[5], str_array);
		TEST_EQ_P (str_array[6], NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 1);
			str_array[0] = NULL;
		}

		ret = proxy_test_set_str_array_sync (parent, proxy, str_array);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.StrArray.Empty");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			str_array = nih_alloc (parent, sizeof (char *) * 5);
			str_array[0] = "this";
			str_array[1] = "is";
			str_array[2] = "a";
			str_array[3] = "test";
			str_array[4] = NULL;
		}

		ret = proxy_test_set_str_array_sync (parent, proxy,
						     str_array);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int get_int32_array_array_replied;

static void
my_get_int32_array_array_reply (void *           data,
				NihDBusMessage * message,
				int32_t * const *value,
				const size_t *   value_len)
{
	get_int32_array_array_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);
	TEST_ALLOC_PARENT (value, message);

	if (*value) {
		TEST_NE_P (value_len, NULL);
		TEST_ALLOC_PARENT (value_len, value);
	}

	last_data = data;
	last_int32_array_array = value;
	nih_ref (last_int32_array_array, data);
	last_int32_array_array_len = value_len;
	if (value_len)
		nih_ref (last_int32_array_array_len, data);
}

void
test_get_int32_array_array (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_get_int32_array_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			int32_array_array_property = nih_alloc (NULL, sizeof (int32_t *) * 3);
			int32_array_array_property_len = nih_alloc (int32_array_array_property,
								    sizeof (size_t) * 2);

			int32_array_array_property[0] = nih_alloc (int32_array_array_property,
								   sizeof (int32_t) * 6);
			int32_array_array_property[0][0] = 4;
			int32_array_array_property[0][1] = 8;
			int32_array_array_property[0][2] = 15;
			int32_array_array_property[0][3] = 16;
			int32_array_array_property[0][4] = 23;
			int32_array_array_property[0][5] = 42;

			int32_array_array_property_len[0] = 6;

			int32_array_array_property[1] = nih_alloc (int32_array_array_property,
								   sizeof (int32_t) * 6);
			int32_array_array_property[1][0] = 1;
			int32_array_array_property[1][1] = 1;
			int32_array_array_property[1][2] = 2;
			int32_array_array_property[1][3] = 3;
			int32_array_array_property[1][4] = 5;
			int32_array_array_property[1][5] = 8;

			int32_array_array_property_len[1] = 6;

			int32_array_array_property[2] = NULL;

			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int32_array_array_replied = FALSE;
		last_data = NULL;
		last_int32_array_array = NULL;
		last_int32_array_array_len = NULL;

		pending_call = proxy_test_get_int32_array_array (
			proxy,
			my_get_int32_array_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			nih_free (int32_array_array_property);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (get_int32_array_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_ALLOC_SIZE (last_int32_array_array, sizeof (int32_t *) * 3);
		TEST_ALLOC_PARENT (last_int32_array_array, parent);
		TEST_ALLOC_SIZE (last_int32_array_array_len, sizeof (size_t) * 2);
		TEST_ALLOC_PARENT (last_int32_array_array_len,
				   last_int32_array_array);

		TEST_EQ (last_int32_array_array_len[0], 6);
		TEST_ALLOC_SIZE (last_int32_array_array[0],
				 sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (last_int32_array_array[0],
				   last_int32_array_array);
		TEST_EQ (last_int32_array_array[0][0], 4);
		TEST_EQ (last_int32_array_array[0][1], 8);
		TEST_EQ (last_int32_array_array[0][2], 15);
		TEST_EQ (last_int32_array_array[0][3], 16);
		TEST_EQ (last_int32_array_array[0][4], 23);
		TEST_EQ (last_int32_array_array[0][5], 42);

		TEST_EQ (last_int32_array_array_len[1], 6);
		TEST_ALLOC_SIZE (last_int32_array_array[1],
				 sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (last_int32_array_array[1],
				   last_int32_array_array);
		TEST_EQ (last_int32_array_array[1][0], 1);
		TEST_EQ (last_int32_array_array[1][1], 1);
		TEST_EQ (last_int32_array_array[1][2], 2);
		TEST_EQ (last_int32_array_array[1][3], 3);
		TEST_EQ (last_int32_array_array[1][4], 5);
		TEST_EQ (last_int32_array_array[1][5], 8);

		TEST_EQ_P (last_int32_array_array[2], NULL);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
		nih_free (int32_array_array_property);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int32_array_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_int32_array_array (
			proxy,
			my_get_int32_array_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_int32_array_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		error_handler_called = FALSE;
		get_int32_array_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_get_int32_array_array (
			proxy,
			my_get_int32_array_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (get_int32_array_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_get_int32_array_array_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int32_t **      int32_array_array;
	size_t *        int32_array_array_len;
	int             ret;
	NihError *      err;
	int             status;

	TEST_FUNCTION ("proxy_test_get_int32_array_array_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		int32_array_array_property = nih_alloc (NULL, sizeof (int32_t *) * 3);
		int32_array_array_property_len = nih_alloc (int32_array_array_property,
							    sizeof (size_t) * 2);

		int32_array_array_property[0] = nih_alloc (int32_array_array_property,
							   sizeof (int32_t) * 6);
		int32_array_array_property[0][0] = 4;
		int32_array_array_property[0][1] = 8;
		int32_array_array_property[0][2] = 15;
		int32_array_array_property[0][3] = 16;
		int32_array_array_property[0][4] = 23;
		int32_array_array_property[0][5] = 42;

		int32_array_array_property_len[0] = 6;

		int32_array_array_property[1] = nih_alloc (int32_array_array_property,
							   sizeof (int32_t) * 6);
		int32_array_array_property[1][0] = 1;
		int32_array_array_property[1][1] = 1;
		int32_array_array_property[1][2] = 2;
		int32_array_array_property[1][3] = 3;
		int32_array_array_property[1][4] = 5;
		int32_array_array_property[1][5] = 8;

		int32_array_array_property_len[1] = 6;

		int32_array_array_property[2] = NULL;


		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		nih_free (int32_array_array_property);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);
		}

		int32_array_array = NULL;
		int32_array_array_len = NULL;

		ret = proxy_test_get_int32_array_array_sync (
			parent, proxy,
			&int32_array_array,
			&int32_array_array_len);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_SIZE (int32_array_array, sizeof (int32_t *) * 3);
		TEST_ALLOC_PARENT (int32_array_array, parent);
		TEST_ALLOC_SIZE (int32_array_array_len, sizeof (size_t) * 2);
		TEST_ALLOC_PARENT (int32_array_array_len, int32_array_array);

		TEST_EQ (int32_array_array_len[0], 6);
		TEST_ALLOC_SIZE (int32_array_array[0], sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (int32_array_array[0], int32_array_array);
		TEST_EQ (int32_array_array[0][0], 4);
		TEST_EQ (int32_array_array[0][1], 8);
		TEST_EQ (int32_array_array[0][2], 15);
		TEST_EQ (int32_array_array[0][3], 16);
		TEST_EQ (int32_array_array[0][4], 23);
		TEST_EQ (int32_array_array[0][5], 42);

		TEST_EQ (int32_array_array_len[1], 6);
		TEST_ALLOC_SIZE (int32_array_array[1], sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (int32_array_array[1], int32_array_array);
		TEST_EQ (int32_array_array[1][0], 1);
		TEST_EQ (int32_array_array[1][1], 1);
		TEST_EQ (int32_array_array[1][2], 2);
		TEST_EQ (int32_array_array[1][3], 3);
		TEST_EQ (int32_array_array[1][4], 5);
		TEST_EQ (int32_array_array[1][5], 8);

		TEST_EQ_P (int32_array_array[2], NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int set_int32_array_array_replied;

static void
my_set_int32_array_array_reply (void *          data,
				NihDBusMessage *message)
{
	set_int32_array_array_replied++;

	TEST_NE_P (data, NULL);
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_data = data;
}

void
test_set_int32_array_array (void)
{
	pid_t            dbus_pid;
	DBusConnection * client_conn;
	DBusConnection * server_conn;
	DBusConnection * flakey_conn;
	NihDBusObject *  object;
	NihDBusProxy *   proxy;
	void *           parent;
	int32_t **       int32_array_array;
	size_t *         int32_array_array_len;
	DBusPendingCall *pending_call;
	NihError *       err;
	NihDBusError *   dbus_err;

	TEST_FUNCTION ("proxy_test_set_int32_array_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we receive a pending call object after giving a
	 * valid input, dispatching from the server side should result
	 * in the call being completed and our handler being called with
	 * the output arguments.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		int32_array_array_property = NULL;
		int32_array_array_property_len = NULL;

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 3);
			int32_array_array_len = nih_alloc (int32_array_array,
							   sizeof (size_t) * 2);

			int32_array_array[0] = nih_alloc (int32_array_array,
							  sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;

			int32_array_array_len[0] = 6;

			int32_array_array[1] = nih_alloc (int32_array_array,
							  sizeof (int32_t) * 6);
			int32_array_array[1][0] = 1;
			int32_array_array[1][1] = 1;
			int32_array_array[1][2] = 2;
			int32_array_array[1][3] = 3;
			int32_array_array[1][4] = 5;
			int32_array_array[1][5] = 8;

			int32_array_array_len[1] = 6;

			int32_array_array[2] = NULL;
		}

		error_handler_called = FALSE;
		set_int32_array_array_replied = FALSE;
		last_data = NULL;
		last_int32_array_array = NULL;
		last_int32_array_array_len = NULL;

		pending_call = proxy_test_set_int32_array_array (
			proxy,
			int32_array_array, int32_array_array_len,
			my_set_int32_array_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_FALSE (error_handler_called);
		TEST_TRUE (set_int32_array_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_ALLOC_SIZE (int32_array_array_property,
				 sizeof (int32_t *) * 3);
		TEST_ALLOC_SIZE (int32_array_array_property_len,
				 sizeof (size_t) * 2);

		TEST_EQ (int32_array_array_property_len[0], 6);
		TEST_ALLOC_SIZE (int32_array_array_property[0],
				 sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (int32_array_array_property[0],
				   int32_array_array_property);
		TEST_EQ (int32_array_array_property[0][0], 4);
		TEST_EQ (int32_array_array_property[0][1], 8);
		TEST_EQ (int32_array_array_property[0][2], 15);
		TEST_EQ (int32_array_array_property[0][3], 16);
		TEST_EQ (int32_array_array_property[0][4], 23);
		TEST_EQ (int32_array_array_property[0][5], 42);

		TEST_EQ (int32_array_array_property_len[1], 6);
		TEST_ALLOC_SIZE (int32_array_array_property[1],
				 sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (int32_array_array_property[1],
				   int32_array_array_property);
		TEST_EQ (int32_array_array_property[1][0], 1);
		TEST_EQ (int32_array_array_property[1][1], 1);
		TEST_EQ (int32_array_array_property[1][2], 2);
		TEST_EQ (int32_array_array_property[1][3], 3);
		TEST_EQ (int32_array_array_property[1][4], 5);
		TEST_EQ (int32_array_array_property[1][5], 8);

		TEST_EQ_P (int32_array_array_property[2], NULL);

		nih_free (int32_array_array_property_len);
		nih_free (int32_array_array_property);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a D-Bus error is returned resulting in the error
	 * handler being called with the error raised.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 1);
			int32_array_array_len = NULL;

			int32_array_array[0] = NULL;
		}

		error_handler_called = FALSE;
		set_int32_array_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int32_array_array (
			proxy,
			int32_array_array, int32_array_array_len,
			my_set_int32_array_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int32_array_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int32ArrayArray.Empty");

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a generic error is returned resulting in the error
	 * handler being called with the D-Bus "failed" error raised.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 2);
			int32_array_array_len = nih_alloc (int32_array_array,
							   sizeof (size_t) * 1);

			int32_array_array[0] = nih_alloc (int32_array_array,
							  sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;

			int32_array_array_len[0] = 6;

			int32_array_array[1] = NULL;
		}

		error_handler_called = FALSE;
		set_int32_array_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int32_array_array (
			proxy,
			int32_array_array, int32_array_array_len,
			my_set_int32_array_array_reply,
			my_error_handler,
			parent,
			NIH_DBUS_TIMEOUT_DEFAULT);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_connection_flush (client_conn);
		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);
		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int32_array_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a timeout (along with other D-Bus generated errors)
	 * also results in the error handler being called with the D-Bus
	 * error raised.
	 */
	TEST_FEATURE ("with timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 3);
			int32_array_array_len = nih_alloc (int32_array_array,
							   sizeof (size_t) * 2);

			int32_array_array[0] = nih_alloc (int32_array_array,
							  sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;

			int32_array_array_len[0] = 6;

			int32_array_array[1] = nih_alloc (int32_array_array,
							  sizeof (int32_t) * 6);
			int32_array_array[1][0] = 1;
			int32_array_array[1][1] = 1;
			int32_array_array[1][2] = 2;
			int32_array_array[1][3] = 3;
			int32_array_array[1][4] = 5;
			int32_array_array[1][5] = 8;

			int32_array_array_len[1] = 6;

			int32_array_array[2] = NULL;
		}

		error_handler_called = FALSE;
		set_int32_array_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int32_array_array (
			proxy,
			int32_array_array, int32_array_array_len,
			my_set_int32_array_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int32_array_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	/* Check that a remote end disconnection (along with other D-Bus
	 * generated errors) also results in the error handler being
	 * called with the D-Bus error raised.
	 */
	TEST_FEATURE ("with server disconnection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 3);
			int32_array_array_len = nih_alloc (int32_array_array,
							   sizeof (size_t) * 2);

			int32_array_array[0] = nih_alloc (int32_array_array,
							  sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;

			int32_array_array_len[0] = 6;

			int32_array_array[1] = nih_alloc (int32_array_array,
							  sizeof (int32_t) * 6);
			int32_array_array[1][0] = 1;
			int32_array_array[1][1] = 1;
			int32_array_array[1][2] = 2;
			int32_array_array[1][3] = 3;
			int32_array_array[1][4] = 5;
			int32_array_array[1][5] = 8;

			int32_array_array_len[1] = 6;

			int32_array_array[2] = NULL;
		}

		error_handler_called = FALSE;
		set_int32_array_array_replied = FALSE;
		last_data = NULL;
		last_error = NULL;

		pending_call = proxy_test_set_int32_array_array (
			proxy,
			int32_array_array, int32_array_array_len,
			my_set_int32_array_array_reply,
			my_error_handler,
			parent,
			50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			nih_free (object);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);
		TEST_FALSE (dbus_pending_call_get_completed (pending_call));

		TEST_DBUS_CLOSE (flakey_conn);
		dbus_pending_call_block (pending_call);

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		dbus_pending_call_unref (pending_call);

		TEST_TRUE (error_handler_called);
		TEST_FALSE (set_int32_array_array_replied);

		TEST_EQ_P (last_data, parent);
		TEST_NE_P (last_error, NULL);

		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);

		nih_free (last_error);

		nih_free (parent);
		nih_free (proxy);
		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_int32_array_array_sync (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	pid_t           server_pid;
	int             wait_fd;
	NihDBusObject * object;
	NihDBusProxy *  proxy;
	void *          parent;
	int32_t **      int32_array_array;
	size_t *        int32_array_array_len;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;
	int             status;

	TEST_FUNCTION ("proxy_test_set_int32_array_array_sync");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		int32_array_array_property = NULL;
		int32_array_array_property_len = NULL;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		assert0 (nih_dbus_setup (server_conn, NULL));

		object = nih_dbus_object_new (NULL, server_conn,
					      "/com/netsplit/Nih/Test",
					      my_interfaces,
					      NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		nih_free (object);

		if (int32_array_array_property_len)
			nih_free (int32_array_array_property_len);
		if (int32_array_array_property)
			nih_free (int32_array_array_property);

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);

		dbus_shutdown ();

		exit (0);
	}


	/* Check that when we give a valid input, we receive a valid
	 * output in our pointer, allocated as a child of the parent.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 3);
			int32_array_array_len = nih_alloc (int32_array_array,
							   sizeof (size_t) * 2);

			int32_array_array[0] = nih_alloc (int32_array_array,
							  sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;

			int32_array_array_len[0] = 6;

			int32_array_array[1] = nih_alloc (int32_array_array,
							  sizeof (int32_t) * 6);
			int32_array_array[1][0] = 1;
			int32_array_array[1][1] = 1;
			int32_array_array[1][2] = 2;
			int32_array_array[1][3] = 3;
			int32_array_array[1][4] = 5;
			int32_array_array[1][5] = 8;

			int32_array_array_len[1] = 6;

			int32_array_array[2] = NULL;
		}

		ret = proxy_test_set_int32_array_array_sync (parent, proxy,
							     int32_array_array,
							     int32_array_array_len);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (ret, 0);

		nih_free (int32_array_array);

		TEST_ALLOC_SAFE {
			assert0 (proxy_test_get_int32_array_array_sync (
					 parent, proxy,
					 &int32_array_array, &int32_array_array_len));
		}

		TEST_ALLOC_SIZE (int32_array_array, sizeof (int32_t *) * 3);
		TEST_ALLOC_PARENT (int32_array_array, parent);
		TEST_ALLOC_SIZE (int32_array_array_len, sizeof (size_t) * 2);
		TEST_ALLOC_PARENT (int32_array_array_len, int32_array_array);

		TEST_EQ (int32_array_array_len[0], 6);
		TEST_ALLOC_SIZE (int32_array_array[0], sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (int32_array_array[0], int32_array_array);
		TEST_EQ (int32_array_array[0][0], 4);
		TEST_EQ (int32_array_array[0][1], 8);
		TEST_EQ (int32_array_array[0][2], 15);
		TEST_EQ (int32_array_array[0][3], 16);
		TEST_EQ (int32_array_array[0][4], 23);
		TEST_EQ (int32_array_array[0][5], 42);

		TEST_EQ (int32_array_array_len[1], 6);
		TEST_ALLOC_SIZE (int32_array_array[1], sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (int32_array_array[1], int32_array_array);
		TEST_EQ (int32_array_array[1][0], 1);
		TEST_EQ (int32_array_array[1][1], 1);
		TEST_EQ (int32_array_array[1][2], 2);
		TEST_EQ (int32_array_array[1][3], 3);
		TEST_EQ (int32_array_array[1][4], 5);
		TEST_EQ (int32_array_array[1][5], 8);

		TEST_EQ_P (int32_array_array[2], NULL);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a D-Bus error is returned as a raised error, with
	 * an appropriate return value.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 1);
			int32_array_array_len = NULL;

			int32_array_array[0] = NULL;
		}

		ret = proxy_test_set_int32_array_array_sync (parent, proxy,
							     int32_array_array,
							     int32_array_array_len);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Int32ArrayArray.Empty");

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	/* Check that a generic error is returned as a raised D-Bus error
	 * with the "failed" name.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih/Test",
						    NULL, NULL);

			parent = nih_alloc (NULL, 0);

			int32_array_array = nih_alloc (parent, sizeof (int32_t *) * 2);
			int32_array_array_len = nih_alloc (int32_array_array,
							   sizeof (size_t) * 1);

			int32_array_array[0] = nih_alloc (int32_array_array,
							  sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;

			int32_array_array_len[0] = 6;

			int32_array_array[1] = NULL;
		}

		ret = proxy_test_set_int32_array_array_sync (parent, proxy,
							     int32_array_array,
							     int32_array_array_len);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (parent);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

		nih_free (err);

		nih_free (parent);
		nih_free (proxy);
	}


	kill (server_pid, SIGTERM);
	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


int
main (int   argc,
      char *argv[])
{
	nih_error_init ();

	test_ordinary_method ();
	test_ordinary_method_sync ();

	test_nameless_method ();
	test_nameless_method_sync ();

	test_async_method ();
	test_async_method_sync ();

	test_byte_to_str ();
	test_byte_to_str_sync ();

	test_str_to_byte ();
	test_str_to_byte_sync ();

	test_boolean_to_str ();
	test_boolean_to_str_sync ();

	test_str_to_boolean ();
	test_str_to_boolean_sync ();

	test_int16_to_str ();
	test_int16_to_str_sync ();

	test_str_to_int16 ();
	test_str_to_int16_sync ();

	test_uint16_to_str ();
	test_uint16_to_str_sync ();

	test_str_to_uint16 ();
	test_str_to_uint16_sync ();

	test_int32_to_str ();
	test_int32_to_str_sync ();

	test_str_to_int32 ();
	test_str_to_int32_sync ();

	test_uint32_to_str ();
	test_uint32_to_str_sync ();

	test_str_to_uint32 ();
	test_str_to_uint32_sync ();

	test_int64_to_str ();
	test_int64_to_str_sync ();

	test_str_to_int64 ();
	test_str_to_int64_sync ();

	test_uint64_to_str ();
	test_uint64_to_str_sync ();

	test_str_to_uint64 ();
	test_str_to_uint64_sync ();

	test_double_to_str ();
	test_double_to_str_sync ();

	test_str_to_double ();
	test_str_to_double_sync ();

	test_object_path_to_str ();
	test_object_path_to_str_sync ();

	test_str_to_object_path ();
	test_str_to_object_path_sync ();

	test_signature_to_str ();
	test_signature_to_str_sync ();

	test_str_to_signature ();
	test_str_to_signature_sync ();

	test_int32_array_to_str ();
	test_int32_array_to_str_sync ();

	test_str_to_int32_array ();
	test_str_to_int32_array_sync ();

	test_str_array_to_str ();
	test_str_array_to_str_sync ();

	test_str_to_str_array ();
	test_str_to_str_array_sync ();

	test_int32_array_array_to_str ();
	test_int32_array_array_to_str_sync ();

	test_str_to_int32_array_array ();
	test_str_to_int32_array_array_sync ();

	test_new_byte ();
	test_new_boolean ();
	test_new_int16 ();
	test_new_uint16 ();
	test_new_int32 ();
	test_new_uint32 ();
	test_new_int64 ();
	test_new_uint64 ();
	test_new_double ();
	test_new_string ();
	test_new_object_path ();
	test_new_signature ();
	test_new_int32_array ();
	test_new_str_array ();
	test_new_int32_array_array ();

	test_get_byte ();
	test_get_byte_sync ();

	test_set_byte ();
	test_set_byte_sync ();

	test_get_boolean ();
	test_get_boolean_sync ();

	test_set_boolean ();
	test_set_boolean_sync ();

	test_get_int16 ();
	test_get_int16_sync ();

	test_set_int16 ();
	test_set_int16_sync ();

	test_get_uint16 ();
	test_get_uint16_sync ();

	test_set_uint16 ();
	test_set_uint16_sync ();

	test_get_int32 ();
	test_get_int32_sync ();

	test_set_int32 ();
	test_set_int32_sync ();

	test_get_uint32 ();
	test_get_uint32_sync ();

	test_set_uint32 ();
	test_set_uint32_sync ();

	test_get_int64 ();
	test_get_int64_sync ();

	test_set_int64 ();
	test_set_int64_sync ();

	test_get_uint64 ();
	test_get_uint64_sync ();

	test_set_uint64 ();
	test_set_uint64_sync ();

	test_get_double ();
	test_get_double_sync ();

	test_set_double ();
	test_set_double_sync ();

	test_get_string ();
	test_get_string_sync ();

	test_set_string ();
	test_set_string_sync ();

	test_get_object_path ();
	test_get_object_path_sync ();

	test_set_object_path ();
	test_set_object_path_sync ();

	test_get_signature ();
	test_get_signature_sync ();

	test_set_signature ();
	test_set_signature_sync ();

	test_get_int32_array ();
	test_get_int32_array_sync ();

	test_set_int32_array ();
	test_set_int32_array_sync ();

	test_get_str_array ();
	test_get_str_array_sync ();

	test_set_str_array ();
	test_set_str_array_sync ();

	test_get_int32_array_array ();
	test_get_int32_array_array_sync ();

	test_set_int32_array_array ();
	test_set_int32_array_array_sync ();

	return 0;
}
