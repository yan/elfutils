/* Test program for elfutils::dwarf basics.
   Copyright (C) 2009 Red Hat, Inc.
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

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <clocale>
#include <cstdio>
#include <libintl.h>
#include <ostream>
#include <iomanip>

#include "c++/dwarf"

using namespace elfutils;
using namespace std;

static Dwarf *
open_file (const char *fname)
{
  int fd = open (fname, O_RDONLY);
  if (unlikely (fd == -1))
    error (2, errno, gettext ("cannot open '%s'"), fname);
  Dwarf *dw = dwarf_begin (fd, DWARF_C_READ);
  if (dw == NULL)
    {
      error (2, 0,
	     gettext ("cannot create DWARF descriptor for '%s': %s"),
	     fname, dwarf_errmsg (-1));
    }
  return dw;
}

static void
print_die (const dwarf::debug_info_entry &die,
	   unsigned int indent, unsigned int limit)
{
  string prefix (indent, ' ');
  const string tag = dwarf::tags::name (die.tag ());

  cout << prefix << "<" << tag << " offset=[" << die.offset () << "]";

  for (dwarf::debug_info_entry::attributes_type::const_iterator i
	 = die.attributes ().begin (); i != die.attributes ().end (); ++i)
    cout << " " << (*i).to_string ();

  if (die.has_children ())
    {
      if (limit != 0 && indent >= limit)
	{
	  cout << ">...\n";
	  return;
	}

      cout << ">\n";

      for (dwarf::debug_info_entry::children_type::const_iterator i
	     = die.children ().begin (); i != die.children ().end (); ++i)
	print_die (*i, indent + 1, limit);

      cout << prefix << "</" << tag << ">\n";
    }
  else
    cout << "/>\n";
}

static void
process_file (const char *file, unsigned int limit)
{
  dwarf dw (open_file (file));

  cout << file << ":\n";

  for (dwarf::compile_units::const_iterator i = dw.compile_units ().begin ();
       i != dw.compile_units ().end ();
       ++i)
    print_die (*i, 1, limit);
}

int
main (int argc, char *argv[])
{
  /* Set locale.  */
  (void) setlocale (LC_ALL, "");

  /* Make sure the message catalog can be found.  */
  (void) bindtextdomain (PACKAGE_TARNAME, LOCALEDIR);

  /* Initialize the message catalog.  */
  (void) textdomain (PACKAGE_TARNAME);

  cout << hex << setiosflags (ios::showbase);

  unsigned int depth = 0;
  if (argc > 1 && sscanf (argv[1], "--depth=%u", &depth) == 1)
    {
      --argc;
      ++argv;
    }

  for (int i = 1; i < argc; ++i)
    process_file (argv[i], depth);

  return 0;
}
