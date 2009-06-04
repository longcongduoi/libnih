/* nih-dbus-tool
 *
 * test_test.c - test suite for nih-dbus-tool/type.c
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

#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/list.h>

#include "type.h"


void
test_const (void)
{
	TEST_FUNCTION ("type_const");

	/* Check that the correct value is returned for a Byte. */
	TEST_FEATURE ("with byte");
	TEST_EQ_STR (type_const (DBUS_TYPE_BYTE), "DBUS_TYPE_BYTE");

	/* Check that the correct value is returned for a Boolean. */
	TEST_FEATURE ("with boolean");
	TEST_EQ_STR (type_const (DBUS_TYPE_BOOLEAN), "DBUS_TYPE_BOOLEAN");

	/* Check that the correct value is returned for an Int16. */
	TEST_FEATURE ("with int16");
	TEST_EQ_STR (type_const (DBUS_TYPE_INT16), "DBUS_TYPE_INT16");

	/* Check that the correct value is returned for a UInt16. */
	TEST_FEATURE ("with uint16");
	TEST_EQ_STR (type_const (DBUS_TYPE_UINT16), "DBUS_TYPE_UINT16");

	/* Check that the correct value is returned for an Int32. */
	TEST_FEATURE ("with int32");
	TEST_EQ_STR (type_const (DBUS_TYPE_INT32), "DBUS_TYPE_INT32");

	/* Check that the correct value is returned for a UInt32. */
	TEST_FEATURE ("with uint32");
	TEST_EQ_STR (type_const (DBUS_TYPE_UINT32), "DBUS_TYPE_UINT32");

	/* Check that the correct value is returned for an Int64. */
	TEST_FEATURE ("with int64");
	TEST_EQ_STR (type_const (DBUS_TYPE_INT64), "DBUS_TYPE_INT64");

	/* Check that the correct value is returned for a UInt64. */
	TEST_FEATURE ("with uint64");
	TEST_EQ_STR (type_const (DBUS_TYPE_UINT64), "DBUS_TYPE_UINT64");

	/* Check that the correct value is returned for a Double. */
	TEST_FEATURE ("with double");
	TEST_EQ_STR (type_const (DBUS_TYPE_DOUBLE), "DBUS_TYPE_DOUBLE");

	/* Check that the correct value is returned for a String. */
	TEST_FEATURE ("with stirng");
	TEST_EQ_STR (type_const (DBUS_TYPE_STRING), "DBUS_TYPE_STRING");

	/* Check that the correct value is returned for an Object Path. */
	TEST_FEATURE ("with object path");
	TEST_EQ_STR (type_const (DBUS_TYPE_OBJECT_PATH),
		     "DBUS_TYPE_OBJECT_PATH");

	/* Check that the correct value is returned for a Signature. */
	TEST_FEATURE ("with signature");
	TEST_EQ_STR (type_const (DBUS_TYPE_SIGNATURE), "DBUS_TYPE_SIGNATURE");

	/* Check that the correct value is returned for an Array. */
	TEST_FEATURE ("with array");
	TEST_EQ_STR (type_const (DBUS_TYPE_ARRAY), "DBUS_TYPE_ARRAY");

	/* Check that the correct value is returned for a Struct. */
	TEST_FEATURE ("with struct");
	TEST_EQ_STR (type_const (DBUS_TYPE_STRUCT), "DBUS_TYPE_STRUCT");

	/* Check that the correct value is returned for a Dict Entry. */
	TEST_FEATURE ("with dict entry");
	TEST_EQ_STR (type_const (DBUS_TYPE_DICT_ENTRY),
		     "DBUS_TYPE_DICT_ENTRY");
}


void
test_of (void)
{
	DBusSignatureIter iter;
	char *            str;

	TEST_FUNCTION ("type_of");


	/* Check that the expected C type is returned for a D-Bus Byte
	 * type.
	 */
	TEST_FEATURE ("with byte");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_BYTE_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "uint8_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Boolean
	 * type.
	 */
	TEST_FEATURE ("with boolean");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_BOOLEAN_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "int");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Int16
	 * type.
	 */
	TEST_FEATURE ("with int16");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_INT16_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "int16_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus UInt16
	 * type.
	 */
	TEST_FEATURE ("with uint16");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_UINT16_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "uint16_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Int32
	 * type.
	 */
	TEST_FEATURE ("with int32");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_INT32_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "int32_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus UInt32
	 * type.
	 */
	TEST_FEATURE ("with uint32");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_UINT32_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "uint32_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Int64
	 * type.
	 */
	TEST_FEATURE ("with int64");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_INT64_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "int64_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus UInt64
	 * type.
	 */
	TEST_FEATURE ("with uint64");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_UINT64_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "uint64_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Double
	 * type.
	 */
	TEST_FEATURE ("with double");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_DOUBLE_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "double");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus String
	 * type.
	 */
	TEST_FEATURE ("with string");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_STRING_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "char *");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Object Path
	 * type.
	 */
	TEST_FEATURE ("with object path");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_OBJECT_PATH_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "char *");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Signature
	 * type.
	 */
	TEST_FEATURE ("with signature");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_SIGNATURE_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "char *");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Int16 Array
	 * type.
	 */
	TEST_FEATURE ("with int16 array");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, (DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_INT16_AS_STRING));

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "int16_t *");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Int16
	 * Array Array type.
	 */
	TEST_FEATURE ("with int16 array array");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, (DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_INT16_AS_STRING));

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "int16_t **");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus String
	 * Array type.
	 */
	TEST_FEATURE ("with string array");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, (DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_STRING_AS_STRING));

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "char **");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus String
	 * Array Array type.
	 */
	TEST_FEATURE ("with string array array");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, (DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_STRING_AS_STRING));

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "char ***");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Structure,
	 * which is a struct definition including the type signature.
	 */
	TEST_FEATURE ("with structure");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						  DBUS_TYPE_STRING_AS_STRING
						  DBUS_TYPE_UINT32_AS_STRING
						  DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_STRING_AS_STRING
						  DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_INT16_AS_STRING
						  DBUS_STRUCT_END_CHAR_AS_STRING));

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "struct dbus_struct_suasan *");

		nih_free (str);
	}
}


void
test_var_new (void)
{
	TypeVar *var;

	TEST_FUNCTION ("type_var_new");


	/* Check to make sure that a TypeVar structure is allocated
	 * correctly and returned.
	 */
	TEST_ALLOC_FAIL {
		var = type_var_new (NULL, "char *", "foo");

		if (test_alloc_failed) {
			TEST_EQ_P (var, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_LIST_EMPTY (&var->entry);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "foo");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_FALSE (var->array);

		nih_free (var);
	}
}

void
test_var_to_string (void)
{
	TypeVar *var = NULL;
	char *   str;

	TEST_FUNCTION ("type_var_to_string");


	/* Check to make sure that a non-pointer variable is returned with
	 * the type and name separated by a space.
	 */
	TEST_FEATURE ("with non-pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			var = type_var_new (NULL, "int", "foo");
		}

		str = type_var_to_string (NULL, var);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (var);
			continue;
		}

		TEST_EQ_STR (str, "int foo");

		nih_free (str);
		nih_free (var);
	}


	/* Check to make sure that a pointer variable is returned with
	 * the type and name separated by no spaces.
	 */
	TEST_FEATURE ("with pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			var = type_var_new (NULL, "char *", "foo");
		}

		str = type_var_to_string (NULL, var);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (var);
			continue;
		}

		TEST_EQ_STR (str, "char *foo");

		nih_free (str);
		nih_free (var);
	}


	/* Check to make sure that an array variable is returned with []
	 * after the type and name.
	 */
	TEST_FEATURE ("with array");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			var = type_var_new (NULL, "char *", "foo");
			var->array = TRUE;
		}

		str = type_var_to_string (NULL, var);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (var);
			continue;
		}

		TEST_EQ_STR (str, "char *foo[]");

		nih_free (str);
		nih_free (var);
	}
}

void
test_var_layout (void)
{
	NihList  vars;
	TypeVar *var1 = NULL;
	TypeVar *var2 = NULL;
	TypeVar *var3 = NULL;
	char *   str;

	TEST_FUNCTION ("test_var_layout");


	/* Check that we can lay out a couple of different non-pointer
	 * variables with them lined up with the longest type name
	 * separated by a space and the other names lined up.
	 */
	TEST_FEATURE ("with set of non-pointers");
	TEST_ALLOC_FAIL {
		nih_list_init (&vars);

		TEST_ALLOC_SAFE {
			var1 = type_var_new (NULL, "int", "foo");
			nih_list_add (&vars, &var1->entry);

			var2 = type_var_new (NULL, "struct bar", "bar");
			var2->array = TRUE;
			nih_list_add (&vars, &var2->entry);

			var3 = type_var_new (NULL, "uint32_t", "baz");
			nih_list_add (&vars, &var3->entry);
		}

		str = type_var_layout (NULL, &vars);

 		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (var1);
			nih_free (var2);
			nih_free (var3);
			continue;
		}

		TEST_EQ_STR (str, ("int        foo;\n"
				   "struct bar bar[];\n"
				   "uint32_t   baz;\n"));

		nih_free (var1);
		nih_free (var2);
		nih_free (var3);

		nih_free (str);
	}


	/* Check that we can lay out a couple of different pointer
	 * variables with them lined up with the longest type name
	 * followed by the name with the others lined up under it.
	 */
	TEST_FEATURE ("with set of pointers");
	TEST_ALLOC_FAIL {
		nih_list_init (&vars);

		TEST_ALLOC_SAFE {
			var1 = type_var_new (NULL, "int *", "foo");
			nih_list_add (&vars, &var1->entry);

			var2 = type_var_new (NULL, "struct bar *", "bar");
			var2->array = TRUE;
			nih_list_add (&vars, &var2->entry);

			var3 = type_var_new (NULL, "uint32_t *", "baz");
			nih_list_add (&vars, &var3->entry);
		}

		str = type_var_layout (NULL, &vars);

 		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (var1);
			nih_free (var2);
			nih_free (var3);
			continue;
		}

		TEST_EQ_STR (str, ("int *       foo;\n"
				   "struct bar *bar[];\n"
				   "uint32_t *  baz;\n"));

		nih_free (var1);
		nih_free (var2);
		nih_free (var3);

		nih_free (str);
	}


	/* Check that we can lay out a mix of pointer and non-pointer
	 * variables with them lined up with the longest type name
	 * followed by the name with the others lined up under it.
	 */
	TEST_FEATURE ("with mixed set");
	TEST_ALLOC_FAIL {
		nih_list_init (&vars);

		TEST_ALLOC_SAFE {
			var1 = type_var_new (NULL, "int *", "foo");
			nih_list_add (&vars, &var1->entry);

			var2 = type_var_new (NULL, "struct bar", "bar");
			var2->array = TRUE;
			nih_list_add (&vars, &var2->entry);

			var3 = type_var_new (NULL, "uint32_t *", "baz");
			nih_list_add (&vars, &var3->entry);
		}

		str = type_var_layout (NULL, &vars);

 		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (var1);
			nih_free (var2);
			nih_free (var3);
			continue;
		}

		TEST_EQ_STR (str, ("int *      foo;\n"
				   "struct bar bar[];\n"
				   "uint32_t * baz;\n"));

		nih_free (var1);
		nih_free (var2);
		nih_free (var3);

		nih_free (str);
	}


	/* Check that we can accept an empty set, and have the empty
	 * string returned.
	 */
	TEST_FEATURE ("with empty list");
	TEST_ALLOC_FAIL {
		nih_list_init (&vars);

		str = type_var_layout (NULL, &vars);

 		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "");

		nih_free (str);
	}
}


void
test_func_new (void)
{
	TypeFunc *func;

	TEST_FUNCTION ("type_func_new");


	/* Check to make sure that a TypeFunc structure is allocated
	 * correctly and returned.
	 */
	TEST_ALLOC_FAIL {
		func = type_func_new (NULL, "char *", "foo");

		if (test_alloc_failed) {
			TEST_EQ_P (func, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_LIST_EMPTY (&func->entry);
		TEST_EQ_STR (func->type, "char *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "foo");
		TEST_ALLOC_PARENT (func->name, func);
		TEST_LIST_EMPTY (&func->args);
		TEST_LIST_EMPTY (&func->attribs);

		nih_free (func);
	}
}

void
test_func_to_string (void)
{
	TypeFunc *    func = NULL;
	TypeVar *     arg = NULL;
	NihListEntry *attrib = NULL;
	char *        str;

	TEST_FUNCTION ("type_func_to_string");


	/* Make sure that a function declaration with a set of non-pointer
	 * arguments is formatted correctly.
	 */
	TEST_FEATURE ("with non-pointer arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			func = type_func_new (NULL, "int", "function_name");

			arg = type_var_new (func, "int", "foo");
			nih_list_add (&func->args, &arg->entry);;

			arg = type_var_new (func, "struct bar", "bar");
			nih_list_add (&func->args, &arg->entry);

			arg = type_var_new (func, "uint32_t", "baz");
			nih_list_add (&func->args, &arg->entry);
		}

		str = type_func_to_string (NULL, func);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (func);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "function_name (int        foo,\n"
				   "               struct bar bar,\n"
				   "               uint32_t   baz)\n"));

		nih_free (str);
		nih_free (func);
	}


	/* Make sure that a function declaration with a set of pointer
	 * arguments is formatted correctly.
	 */
	TEST_FEATURE ("with pointer arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			func = type_func_new (NULL, "int", "function_name");

			arg = type_var_new (func, "int *", "foo");
			nih_list_add (&func->args, &arg->entry);;

			arg = type_var_new (func, "struct bar *", "bar");
			nih_list_add (&func->args, &arg->entry);

			arg = type_var_new (func, "uint32_t *", "baz");
			nih_list_add (&func->args, &arg->entry);
		}

		str = type_func_to_string (NULL, func);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (func);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "function_name (int *       foo,\n"
				   "               struct bar *bar,\n"
				   "               uint32_t *  baz)\n"));

		nih_free (str);
		nih_free (func);
	}


	/* Make sure that a function declaration with a mixed set of
	 * non-pointer and pointer arguments is formatted correctly.
	 */
	TEST_FEATURE ("with mixed arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			func = type_func_new (NULL, "int", "function_name");

			arg = type_var_new (func, "int", "foo");
			nih_list_add (&func->args, &arg->entry);;

			arg = type_var_new (func, "struct bar *", "bar");
			nih_list_add (&func->args, &arg->entry);

			arg = type_var_new (func, "uint32_t *", "baz");
			nih_list_add (&func->args, &arg->entry);
		}

		str = type_func_to_string (NULL, func);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (func);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "function_name (int         foo,\n"
				   "               struct bar *bar,\n"
				   "               uint32_t *  baz)\n"));

		nih_free (str);
		nih_free (func);
	}


	/* Check that a function declaration with a single non-pointer
	 * argument is formatted correctly.
	 */
	TEST_FEATURE ("with single non-pointer argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			func = type_func_new (NULL, "int", "function_name");

			arg = type_var_new (func, "int", "foo");
			nih_list_add (&func->args, &arg->entry);;
		}

		str = type_func_to_string (NULL, func);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (func);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "function_name (int foo)\n"));

		nih_free (str);
		nih_free (func);
	}


	/* Check that a function declaration with a single pointer
	 * argument is formatted correctly.
	 */
	TEST_FEATURE ("with single pointer argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			func = type_func_new (NULL, "int", "function_name");

			arg = type_var_new (func, "int *", "foo");
			nih_list_add (&func->args, &arg->entry);;
		}

		str = type_func_to_string (NULL, func);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (func);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "function_name (int *foo)\n"));

		nih_free (str);
		nih_free (func);
	}


	/* Check that a function declaration with no arguments is
	 * formatted correctly.
	 */
	TEST_FEATURE ("with no arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			func = type_func_new (NULL, "int", "function_name");
		}

		str = type_func_to_string (NULL, func);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (func);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "function_name (void)\n"));

		nih_free (str);
		nih_free (func);
	}


	/* Check that function attributes have no bearing on the declaration
	 * since they only appear in the prototype.
	 */
	TEST_FEATURE ("with attributes");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			func = type_func_new (NULL, "void", "function_name");

			attrib = nih_list_entry_new (func);
			attrib->str = nih_strdup (attrib, "warn_unused_result");
			nih_list_add (&func->attribs, &attrib->entry);
		}

		str = type_func_to_string (NULL, func);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (func);
			continue;
		}

		TEST_EQ_STR (str, ("void\n"
				   "function_name (void)\n"));

		nih_free (str);
		nih_free (func);
	}
}

void
test_func_layout (void)
{
	NihList       funcs;
	TypeFunc *    func1 = NULL;
	TypeFunc *    func2 = NULL;
	TypeFunc *    func3 = NULL;
	TypeVar *     arg = NULL;
	NihListEntry *attrib = NULL;
	char *        str;

	TEST_FUNCTION ("type_func_layout");


	/* Check that functions with non-pointer return types are lined
	 * up properly both by name and type.
	 */
	TEST_FEATURE ("with non-pointer return types");
	TEST_ALLOC_FAIL {
		nih_list_init (&funcs);

		TEST_ALLOC_SAFE {
			func1 = type_func_new (NULL, "int",
					       "first_function_name");
			nih_list_add (&funcs, &func1->entry);

			arg = type_var_new (func1, "int", "foo");
			nih_list_add (&func1->args, &arg->entry);

			arg = type_var_new (func1, "char *", "bar");
			nih_list_add (&func1->args, &arg->entry);


			func2 = type_func_new (NULL, "double",
					       "second_function_name");
			nih_list_add (&funcs, &func2->entry);

			arg = type_var_new (func2, "int", "foo");
			nih_list_add (&func2->args, &arg->entry);

			arg = type_var_new (func2, "char *", "bar");
			nih_list_add (&func2->args, &arg->entry);


			func3 = type_func_new (NULL, "uint32_t",
					       "third_function_name");
			nih_list_add (&funcs, &func3->entry);

			arg = type_var_new (func3, "int", "foo");
			nih_list_add (&func3->args, &arg->entry);

			arg = type_var_new (func3, "char *", "bar");
			nih_list_add (&func3->args, &arg->entry);
		}

		str = type_func_layout (NULL, &funcs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (func1);
			nih_free (func2);
			nih_free (func3);
			continue;
		}

		TEST_EQ_STR (str, ("int      first_function_name  (int foo, char *bar);\n"
				   "double   second_function_name (int foo, char *bar);\n"
				   "uint32_t third_function_name  (int foo, char *bar);\n"));

		nih_free (str);
		nih_free (func1);
		nih_free (func2);
		nih_free (func3);
	}


	/* Check that functions with pointer return types are lined
	 * up properly both by name and type.
	 */
	TEST_FEATURE ("with pointer return types");
	TEST_ALLOC_FAIL {
		nih_list_init (&funcs);

		TEST_ALLOC_SAFE {
			func1 = type_func_new (NULL, "int *",
					       "first_function_name");
			nih_list_add (&funcs, &func1->entry);

			arg = type_var_new (func1, "int", "foo");
			nih_list_add (&func1->args, &arg->entry);

			arg = type_var_new (func1, "char *", "bar");
			nih_list_add (&func1->args, &arg->entry);


			func2 = type_func_new (NULL, "struct foo *",
					       "second_function_name");
			nih_list_add (&funcs, &func2->entry);

			arg = type_var_new (func2, "int", "foo");
			nih_list_add (&func2->args, &arg->entry);

			arg = type_var_new (func2, "char *", "bar");
			nih_list_add (&func2->args, &arg->entry);


			func3 = type_func_new (NULL, "uint32_t *",
					       "third_function_name");
			nih_list_add (&funcs, &func3->entry);

			arg = type_var_new (func3, "int", "foo");
			nih_list_add (&func3->args, &arg->entry);

			arg = type_var_new (func3, "char *", "bar");
			nih_list_add (&func3->args, &arg->entry);
		}

		str = type_func_layout (NULL, &funcs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (func1);
			nih_free (func2);
			nih_free (func3);
			continue;
		}

		TEST_EQ_STR (str, ("int *       first_function_name  (int foo, char *bar);\n"
				   "struct foo *second_function_name (int foo, char *bar);\n"
				   "uint32_t *  third_function_name  (int foo, char *bar);\n"));

		nih_free (str);
		nih_free (func1);
		nih_free (func2);
		nih_free (func3);
	}


	/* Check that functions with a mix of pointer and non-pointer
	 * return types are lined up properly both by name and type.
	 */
	TEST_FEATURE ("with mixed return types");
	TEST_ALLOC_FAIL {
		nih_list_init (&funcs);

		TEST_ALLOC_SAFE {
			func1 = type_func_new (NULL, "int *",
					       "first_function_name");
			nih_list_add (&funcs, &func1->entry);

			arg = type_var_new (func1, "int", "foo");
			nih_list_add (&func1->args, &arg->entry);

			arg = type_var_new (func1, "char *", "bar");
			nih_list_add (&func1->args, &arg->entry);


			func2 = type_func_new (NULL, "struct foo *",
					       "second_function_name");
			nih_list_add (&funcs, &func2->entry);

			arg = type_var_new (func2, "int", "foo");
			nih_list_add (&func2->args, &arg->entry);

			arg = type_var_new (func2, "char *", "bar");
			nih_list_add (&func2->args, &arg->entry);


			func3 = type_func_new (NULL, "uint32_t",
					       "third_function_name");
			nih_list_add (&funcs, &func3->entry);

			arg = type_var_new (func3, "int", "foo");
			nih_list_add (&func3->args, &arg->entry);

			arg = type_var_new (func3, "char *", "bar");
			nih_list_add (&func3->args, &arg->entry);
		}

		str = type_func_layout (NULL, &funcs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (func1);
			nih_free (func2);
			nih_free (func3);
			continue;
		}

		TEST_EQ_STR (str, ("int *       first_function_name  (int foo, char *bar);\n"
				   "struct foo *second_function_name (int foo, char *bar);\n"
				   "uint32_t    third_function_name  (int foo, char *bar);\n"));

		nih_free (str);
		nih_free (func1);
		nih_free (func2);
		nih_free (func3);
	}


	/* Check that functions with attributes have them lined up
	 * beneath the function declaration indented by a tab.
	 */
	TEST_FEATURE ("with function attributes");
	TEST_ALLOC_FAIL {
		nih_list_init (&funcs);

		TEST_ALLOC_SAFE {
			func1 = type_func_new (NULL, "int *",
					       "first_function_name");
			nih_list_add (&funcs, &func1->entry);

			arg = type_var_new (func1, "int", "foo");
			nih_list_add (&func1->args, &arg->entry);

			arg = type_var_new (func1, "char *", "bar");
			nih_list_add (&func1->args, &arg->entry);

			attrib = nih_list_entry_new (func1);
			attrib->str = nih_strdup (attrib, "warn_unused_result");
			nih_list_add (&func1->attribs, &attrib->entry);


			func2 = type_func_new (NULL, "struct foo *",
					       "second_function_name");
			nih_list_add (&funcs, &func2->entry);

			arg = type_var_new (func2, "int", "foo");
			nih_list_add (&func2->args, &arg->entry);

			arg = type_var_new (func2, "char *", "bar");
			nih_list_add (&func2->args, &arg->entry);

			attrib = nih_list_entry_new (func2);
			attrib->str = nih_strdup (attrib, "warn_unused_result");
			nih_list_add (&func2->attribs, &attrib->entry);

			attrib = nih_list_entry_new (func2);
			attrib->str = nih_strdup (attrib, "malloc");
			nih_list_add (&func2->attribs, &attrib->entry);


			func3 = type_func_new (NULL, "uint32_t",
					       "third_function_name");
			nih_list_add (&funcs, &func3->entry);

			arg = type_var_new (func3, "int", "foo");
			nih_list_add (&func3->args, &arg->entry);

			arg = type_var_new (func3, "char *", "bar");
			nih_list_add (&func3->args, &arg->entry);

			attrib = nih_list_entry_new (func3);
			attrib->str = nih_strdup (attrib, "deprecated");
			nih_list_add (&func3->attribs, &attrib->entry);
		}

		str = type_func_layout (NULL, &funcs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (func1);
			nih_free (func2);
			nih_free (func3);
			continue;
		}

		TEST_EQ_STR (str, ("int *       first_function_name  (int foo, char *bar)\n"
				   "\t__attribute__ ((warn_unused_result));\n"
				   "struct foo *second_function_name (int foo, char *bar)\n"
				   "\t__attribute__ ((warn_unused_result, malloc));\n"
				   "uint32_t    third_function_name  (int foo, char *bar)\n"
				   "\t__attribute__ ((deprecated));\n"));

		nih_free (str);
		nih_free (func1);
		nih_free (func2);
		nih_free (func3);
	}


	/* Check that an empty list of functions return an empty string.
	 */
	TEST_FEATURE ("with empty function list");
	TEST_ALLOC_FAIL {
		nih_list_init (&funcs);

		str = type_func_layout (NULL, &funcs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "");

		nih_free (str);
	}
}


void
test_to_const (void)
{
	char *str = NULL;
	char *ret;

	TEST_FUNCTION ("type_to_const");


	/* Check to make sure that a non-pointer declaration is returned
	 * unmodified.
	 */
	TEST_FEATURE ("with non-pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "int");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "int");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "int");

		nih_free (str);
	}


	/* Check that a simple first-level pointer has the const initialiser
	 * prepended onto the front, before the type name.
	 */
	TEST_FEATURE ("with pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "char *");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "char *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "const char *");

		nih_free (str);
	}


	/* Check that a two-level pointer has the const initialiser
	 * placed before the final pointer operator.
	 */
	TEST_FEATURE ("with pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "char **");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "char **");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "char * const *");

		nih_free (str);
	}


	/* Check that a three-level pointer has the const initialiser
	 * placed before the final pointer operator.
	 */
	TEST_FEATURE ("with pointer pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "char ***");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "char ***");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "char ** const *");

		nih_free (str);
	}


	/* Check that an already-const pointer declaration is returned
	 * unmodified.
	 */
	TEST_FEATURE ("with const pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "const struct foo *");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "const struct foo *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "const struct foo *");

		nih_free (str);
	}


	/* Check that an already-const pointer pointer declaration is returned
	 * unmodified.
	 */
	TEST_FEATURE ("with const pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "struct foo * const *");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "struct foo * const *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo * const *");

		nih_free (str);
	}


	/* Check that a pointer to a const pointer is modified to be a
	 * const pointer to a const pointer.
	 */
	TEST_FEATURE ("with pointer to const pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "const struct foo **");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "const struct foo **");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "const struct foo * const *");

		nih_free (str);
	}


	/* Check that an already-const pointer pointer pointer declaration
	 * is returned unmodified.
	 */
	TEST_FEATURE ("with const pointer pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "struct foo ** const *");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "struct foo ** const *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo ** const *");

		nih_free (str);
	}


	/* Check that a pointer to a const pointer pointer is modified
	 * to be a constant pointer to a const pointer pointer.
	 */
	TEST_FEATURE ("with pointer to const pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "struct foo * const **");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "struct foo * const **");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo * const * const *");

		nih_free (str);
	}
}

void
test_to_pointer (void)
{
	char *str = NULL;
	char *ret;

	TEST_FUNCTION ("type_to_pointer");


	/* Check to make sure that a non-pointer declaration is returned
	 * as a pointer.
	 */
	TEST_FEATURE ("with non-pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "int");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "int");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "int *");

		nih_free (str);
	}


	/* Check that a simple first-level pointer has a further pointer
	 * level added.
	 */
	TEST_FEATURE ("with pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "char *");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "char *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "char **");

		nih_free (str);

	}


	/* Check that a two-level pointer has yet another further pointer
	 * level added.
	 */
	TEST_FEATURE ("with pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "char **");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "char **");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "char ***");

		nih_free (str);
	}


	/* Check that a constant pointer has a further level of pointerness
	 * added, and the const moved to the new first level.
	 */
	TEST_FEATURE ("with const pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "const struct foo *");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "const struct foo *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo * const *");

		nih_free (str);

	}


	/* Check that a constant pointer pointer has a further level of
	 * pointerness added and the const moved to the new first level.
	 */
	TEST_FEATURE ("with const pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "struct foo * const *");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "struct foo * const *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo ** const *");

		nih_free (str);

	}


	/* Check that a constant pointer pointer pointer has a further level
	 * of pointerness added and the const moved to the new first level.
	 */
	TEST_FEATURE ("with const pointer pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "struct foo ** const *");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "struct foo ** const *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo *** const *");

		nih_free (str);

	}


	/* Check that a pointer to a constant pointer only has a further
	 * level of pointerness added, and the const pointer is not
	 * moved to the new first level.
	 */
	TEST_FEATURE ("with pointer to const pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "const struct foo **");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "const struct foo **");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "const struct foo ***");

		nih_free (str);

	}


	/* Check that a pointer to a constant pointer pointer only has a
	 * further level of pointerness added, and the const pointer is not
	 * moved to the new first level.
	 */
	TEST_FEATURE ("with pointer to const pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "struct foo * const **");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "struct foo * const **");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo * const ***");

		nih_free (str);

	}
}

void
test_to_static (void)
{
	char *str = NULL;
	char *ret;

	TEST_FUNCTION ("type_to_static");


	/* Check to make sure that a non-static declaration is returned
	 * with "static" prepended onto it.
	 */
	TEST_FEATURE ("with non-static type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "int");
		}

		ret = type_to_static (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "int");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "static int");

		nih_free (str);
	}


	/* Check to make sure that a static declaration is returned
	 * unmodified.
	 */
	TEST_FEATURE ("with static type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "static int");
		}

		ret = type_to_static (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "static int");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "static int");

		nih_free (str);
	}
}

void
test_to_extern (void)
{
	char *str = NULL;
	char *ret;

	TEST_FUNCTION ("type_to_extern");


	/* Check to make sure that a non-extern declaration is returned
	 * with "extern" prepended onto it.
	 */
	TEST_FEATURE ("with non-extern type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "int");
		}

		ret = type_to_extern (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "int");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "extern int");

		nih_free (str);
	}


	/* Check to make sure that an extern declaration is returned
	 * unmodified.
	 */
	TEST_FEATURE ("with extern type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "extern int");
		}

		ret = type_to_extern (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "extern int");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "extern int");

		nih_free (str);
	}
}


int
main (int   argc,
      char *argv[])
{
	test_const ();

	test_of ();

	test_var_new ();
	test_var_to_string ();
	test_var_layout ();

	test_func_new ();
	test_func_to_string ();
	test_func_layout ();

	test_to_const ();
	test_to_pointer ();
	test_to_static ();
	test_to_extern ();

	return 0;
}
