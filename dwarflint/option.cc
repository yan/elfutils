/* Pedantic checking of DWARF files
   Copyright (C) 2009,2010 Red Hat, Inc.
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

#include "option.hh"
#include <cassert>
#include <cstring>
#include <iostream>

option_common *
options::opt (int key) const
{
  const_iterator it = find (key);
  if (it == end ())
    return NULL;
  return it->second;
}

option_common const *
options::getopt (int key) const
{
  return opt (key);
}

error_t
options::parse_opt (int key, char *arg,
		    __attribute__ ((unused)) argp_state *state)
{
  option_common *o = registered ().opt (key);
  if (o == NULL)
    return ARGP_ERR_UNKNOWN;
  return o->parse_opt (arg, state);
}

struct last_option
  : public argp_option
{
  last_option () {
    std::memset (this, 0, sizeof (*this));
  }
};

/* Bug report address.  */
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

argp
options::build_argp ()
{
  _m_opts.clear ();
  for (const_iterator it = begin (); it != end (); ++it)
    _m_opts.push_back (it->second->build_option ());
  _m_opts.push_back (last_option ());
  argp a = {
    &_m_opts.front (),
    &options::parse_opt,
    "FILE...",
    "\
Pedantic checking of DWARF stored in ELF files.",
    NULL, NULL, NULL
  };
  return a;
}


int option_common::_m_last_opt = 300;

options &
options::registered ()
{
  static options opts;
  return opts;
}

int
option_common::get_short_option (char opt_short)
{
  if (opt_short)
    return opt_short;
  return _m_last_opt++;
}

namespace
{
  argp_option argp_option_ctor (char const *name, int key,
				char const *arg, int flags,
				char const *doc, int group)
  {
    assert (name != NULL);
    assert (doc != NULL);
    argp_option opt = {
      name, key, arg, flags, doc, group
    };
    return opt;
  }
}

option_common::option_common (char const *description,
			      char const *arg_description,
			      char const *opt_long, char opt_short,
			      int flags)
  : _m_opt (argp_option_ctor (opt_long, get_short_option (opt_short),
			      arg_description, flags,
			      description, 0))
  , _m_seen (false)
{
  assert (options::registered ().getopt (_m_opt.key) == NULL);
  options::registered ()[_m_opt.key] = this;
}