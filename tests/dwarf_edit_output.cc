/* Test program for dwarf_edit, dwarf_output transforms with dwarf_comparator.
   Copyright (C) 2010 Red Hat, Inc.
   This file is part of Red Hat elfutils.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "error.h"

#include "c++/dwarf_edit"
#include "c++/dwarf_output"
#include "c++/dwarf_comparator"
#include "c++/dwarf_tracker"

using namespace elfutils;
using namespace std;

// Only used for testing.
#include "print-die.hh"

dwarf_edit &
empty_cu (dwarf_edit &in)
{
  in.add_unit ();
  return in;
}

dwarf_edit &
empty_cus (dwarf_edit &in)
{
  in.add_unit ();
  in.add_unit ();
  in.add_unit ();
  return in;
}

dwarf_edit &
two_same_dies (dwarf_edit &in)
{
  dwarf_edit::compile_unit &cu = in.add_unit ();
  cu.attributes ()[DW_AT_producer].string () = "dwarf_edit_output_test";

  dwarf_edit::debug_info_entry::pointer attr1 = cu.add_entry (DW_TAG_base_type);
  attr1->attributes ()[DW_AT_name].identifier () = "int";
  // XXX Not a dwarf_constant? Prints out wrongly
  //attr1->attributes ()[DW_AT_encoding].dwarf_constant () = DW_ATE_signed;
  attr1->attributes ()[DW_AT_byte_size].constant () = 4;

  dwarf_edit::debug_info_entry::pointer attr2 = cu.add_entry (DW_TAG_base_type);
  attr2->attributes ()[DW_AT_name].identifier () = "int";
  attr2->attributes ()[DW_AT_byte_size].constant () = 4;

  return in;
}

dwarf_edit &
var_ref_type (dwarf_edit &in)
{
  dwarf_edit::compile_unit &cu = in.add_unit ();
  cu.attributes ()[DW_AT_producer].string () = "dwarf_edit_output_test";

  dwarf_edit::debug_info_entry::pointer type = cu.add_entry (DW_TAG_base_type);
  type->attributes ()[DW_AT_name].identifier () = "int";
  type->attributes ()[DW_AT_byte_size].constant () = 4;

  dwarf_edit::debug_info_entry &var = *cu.add_entry (DW_TAG_variable);
  var.attributes ()[DW_AT_name].identifier () = "var";
  var.attributes ()[DW_AT_type].reference () = type;

  return in;
}

dwarf_edit &
var_ref_type_after (dwarf_edit &in)
{
  dwarf_edit::compile_unit &cu = in.add_unit ();
  cu.attributes ()[DW_AT_producer].string () = "dwarf_edit_output_test";

  dwarf_edit::debug_info_entry &var = *cu.add_entry (DW_TAG_variable);
  var.attributes ()[DW_AT_name].identifier () = "var";

  dwarf_edit::debug_info_entry::pointer type = cu.add_entry (DW_TAG_base_type);
  type->attributes ()[DW_AT_name].identifier () = "int";
  type->attributes ()[DW_AT_byte_size].constant () = 4;

  var.attributes ()[DW_AT_type].reference () = type;

  return in;
}

dwarf_edit &
dup_same_type_vars (dwarf_edit &in)
{
  dwarf_edit::compile_unit &cu = in.add_unit ();
  cu.attributes ()[DW_AT_producer].string () = "dwarf_edit_output_test";

  dwarf_edit::debug_info_entry::pointer type1 = cu.add_entry (DW_TAG_base_type);
  type1->attributes ()[DW_AT_name].identifier () = "int";
  type1->attributes ()[DW_AT_byte_size].constant () = 4;

  dwarf_edit::debug_info_entry::pointer type2 = cu.add_entry (DW_TAG_base_type);
  type2->attributes ()[DW_AT_name].identifier () = "int";
  type2->attributes ()[DW_AT_byte_size].constant () = 4;

  dwarf_edit::debug_info_entry &var1 = *cu.add_entry (DW_TAG_variable);
  var1.attributes ()[DW_AT_name].identifier () = "var1";
  var1.attributes ()[DW_AT_type].reference () = type1;

  dwarf_edit::debug_info_entry &var2 = *cu.add_entry (DW_TAG_variable);
  var2.attributes ()[DW_AT_name].identifier () = "var2";
  var2.attributes ()[DW_AT_type].reference () = type2;

  return in;
}

dwarf_edit &
circular_struct (dwarf_edit &in)
{
  dwarf_edit::compile_unit &cu = in.add_unit ();
  cu.attributes ()[DW_AT_producer].string () = "dwarf_edit_output_test";

  dwarf_edit::debug_info_entry::pointer int_ref
    = cu.add_entry (DW_TAG_base_type);
  int_ref->attributes ()[DW_AT_name].identifier () = "int";
  int_ref->attributes ()[DW_AT_byte_size].constant () = 4;

  dwarf_edit::debug_info_entry::pointer struct_ptr_ref
    = cu.add_entry (DW_TAG_pointer_type);
  struct_ptr_ref->attributes ()[DW_AT_byte_size].constant () = 8;

  dwarf_edit::debug_info_entry::pointer list_ptr
    = cu.add_entry (DW_TAG_structure_type);
  dwarf_edit::debug_info_entry &list = *list_ptr;
  list.attributes ()[DW_AT_name].identifier () = "list";
  list.attributes ()[DW_AT_byte_size].constant () = 0x10;

  dwarf_edit::debug_info_entry &mi = *list.add_entry (DW_TAG_member);
  mi.attributes ()[DW_AT_name].identifier () = "i";
  mi.attributes ()[DW_AT_type].reference () = int_ref;

  dwarf_edit::debug_info_entry &mn = *list.add_entry (DW_TAG_member);
  mn.attributes ()[DW_AT_name].identifier () = "next";
  mn.attributes ()[DW_AT_type].reference () = struct_ptr_ref;

  struct_ptr_ref->attributes ()[DW_AT_type].reference () = list_ptr;

  return in;
}

// Same as above, but with struct pointer type defined after struct.
dwarf_edit &
circular_struct2 (dwarf_edit &in)
{
  dwarf_edit::compile_unit &cu = in.add_unit ();
  cu.attributes ()[DW_AT_producer].string () = "dwarf_edit_output_test";

  dwarf_edit::debug_info_entry::pointer int_ref
    = cu.add_entry (DW_TAG_base_type);
  int_ref->attributes ()[DW_AT_name].identifier () = "int";
  int_ref->attributes ()[DW_AT_byte_size].constant () = 4;

  dwarf_edit::debug_info_entry::pointer list_ptr
    = cu.add_entry (DW_TAG_structure_type);
  dwarf_edit::debug_info_entry &list = *list_ptr;
  list.attributes ()[DW_AT_name].identifier () = "list";
  list.attributes ()[DW_AT_byte_size].constant () = 0x10;

  dwarf_edit::debug_info_entry &mi = *list.add_entry (DW_TAG_member);
  mi.attributes ()[DW_AT_name].identifier () = "i";
  mi.attributes ()[DW_AT_type].reference () = int_ref;

  dwarf_edit::debug_info_entry &mn = *list.add_entry (DW_TAG_member);
  mn.attributes ()[DW_AT_name].identifier () = "next";

  dwarf_edit::debug_info_entry::pointer struct_ptr_ref
    = cu.add_entry (DW_TAG_pointer_type);
  struct_ptr_ref->attributes ()[DW_AT_byte_size].constant () = 8;
  struct_ptr_ref->attributes ()[DW_AT_type].reference () = list_ptr;

  mn.attributes ()[DW_AT_type].reference () = struct_ptr_ref;

  return in;
}

dwarf_edit &
two_circular_structs (dwarf_edit &in)
{
  circular_struct (in);
  circular_struct (in);
  return in;
}

dwarf_edit &
two_circular_structs2 (dwarf_edit &in)
{
  circular_struct (in);
  circular_struct2 (in);
  return in;
}

static int show_input, show_output;

void
test_run (int n, const char *name, dwarf_edit &in)
{
  if (show_input | show_output)
    printf("*%s*\n", name);

  if (show_input)
    print_file ("dwarf_edit", in, 0);

  dwarf_output_collector c;
  dwarf_output out (in, c);

  if (show_output)
    {
      // c.stats();
      print_file ("dwarf_output", out, 0);
    }

  // NOTE: dwarf_comparator ignore_refs = true
  dwarf_ref_tracker<dwarf_edit, dwarf_output> tracker;
  dwarf_comparator<dwarf_edit, dwarf_output, true> cmp (tracker);
  if (! cmp.equals (in, out))
    error (-1, 0, "fail test #%d '%s'", n, name);
}

int
main (int argc, char **argv)
{
  // Test number to run (or all if none given)
  int r = 0;
  if (argc > 1)
    r = atoi(argv[1]);

  // Whether to print input/output/both [in|out|inout]
  show_input = 0;
  show_output = 0;
  if (argc > 2)
    {
      if (strstr (argv[2], "in"))
	show_input = 1;
      if (strstr (argv[2], "out"))
	show_output = 1;
    }

  if (show_input | show_output)
    {
      // Abuse print_die_main initialization, but don't pass real
      // argc/argv since we use those ourselves.
      int dummy_argc = 0;
      char **dummy_argv = NULL;
      unsigned int d;
      print_die_main (dummy_argc, dummy_argv, d);
    }

#define RUNTEST(N) (r == 0 || r == N)

  dwarf_edit in1;
  if (RUNTEST (1))
    test_run (1, "empty_cu", empty_cu(in1));

  dwarf_edit in2;
  if (RUNTEST (2))
    test_run (2, "empty_cus", empty_cus(in2));

  dwarf_edit in3;
  if (RUNTEST (3))
    test_run (3, "two_same_dies", two_same_dies (in3));

  dwarf_edit in4;
  if (RUNTEST (4))
    test_run (4, "var_ref_type", var_ref_type (in4));

  dwarf_edit in5;
  if (RUNTEST (5))
    test_run (5, "var_ref_type_after", var_ref_type_after (in5));

  dwarf_edit in6;
  if (RUNTEST (6))
    test_run (6, "dup_same_type_vars", dup_same_type_vars (in6));

  dwarf_edit in7;
  if (RUNTEST (7))
    test_run (7, "circular_struct", circular_struct (in7));

  dwarf_edit in8;
  if (RUNTEST (8))
    test_run (8, "circular_struct2", circular_struct2 (in8));

  // XXX Won't merge CUs on main dwarf branch (does on dwarf-hacking)
  // How to check?
  dwarf_edit in9;
  if (RUNTEST (9))
    test_run (9, "two_circular_structs", two_circular_structs (in9));

  // Won't merge CUs since order of children different.
  dwarf_edit in10;
  if (RUNTEST (10))
    test_run (10, "two_circular_structs2", two_circular_structs2 (in9));

  return 0;
}