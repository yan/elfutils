/* Pedantic checking of DWARF files
   Copyright (C) 2011 Red Hat, Inc.
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

#include "messages.hh"
#include "highlevel_check.hh"
#include "check_die_tree.hh"

using namespace elfutils;

namespace
{
  class die_check_registrar
    : public check_registrar_T<die_check_item>
  {
  public:
    friend class dwarflint;
    void run (checkstack &stack, dwarflint &lint);

    static die_check_registrar *
    inst ()
    {
      static die_check_registrar inst;
      return &inst;
    }
  };

  reg<check_die_tree> reg;
}

void
check_die_tree::register_check (die_check_item *check)
{
  die_check_registrar::inst ()->push_back (check);
}

class die_check_context
  : protected std::vector<die_check *>
{
  typedef std::vector<die_check *> _super_t;
  checkdescriptor const *_m_cd;

public:
  die_check_context (highlevel_check_i *check,
		     checkdescriptor const *cd,
		     dwarflint &lint,
		     die_check_registrar const &registrar)
    : _m_cd (cd)
  {
    // For per-DIE runs, we are only interested in limited context:
    // the main iteration check, and the per-DIE check.  This should
    // be enough to decide whether to run the per-DIE check or not.
    // We cannot use the original stack as a criterion, because the
    // original check that tricked us into running is here, and the
    // logic in should_check would then assume that we need to run
    // everything.
    checkstack stack;
    stack.push_back (cd);

    for (die_check_registrar::const_iterator it = registrar.begin ();
	 it != registrar.end (); ++it)
      {
	stack.push_back ((*it)->descriptor ());
	popper p (stack);
	if (lint.rules ().should_check (stack))
	  push_back ((*it)->create (check, stack, lint));
      }
  }

  void
  error (all_dies_iterator<dwarf> const &a_d_it,
	 char const *reason = NULL)
  {
    std::string r;
    if (reason)
      {
	r += ": ";
	r += reason;
      }

    wr_error (to_where (*a_d_it))
      << "A check failed: " << (_m_cd->name () ?: "(nil)")
      << r << std::endl;
  }

  void
  die (all_dies_iterator<dwarf> const &a_d_it)
  {
    for (iterator it = begin (); it != end (); ++it)
    again:
      try
	{
	  (*it)->die (a_d_it);
	}
      catch (check_base::unscheduled &e)
	{
	  // Turn the check off.
	  size_t pos = it - begin ();
	  delete *it;
	  erase (it);
	  it = begin () + pos;
	  if (it == end ())
	    break;
	  goto again;
	}
      catch (check_base::failed &e)
	{
	  // The check was supposed to emit an error message.
	}
      catch (std::exception &e)
	{
	  error (a_d_it, e.what ());
	}
      catch (...)
	{
	  error (a_d_it);
	}
  }

  ~die_check_context ()
  {
    for (iterator it = begin (); it != end (); ++it)
      delete *it;
  }
};

check_die_tree::check_die_tree (checkstack &stack, dwarflint &lint)
  : highlevel_check<check_die_tree> (stack, lint)
{
  die_check_context ctx (this, descriptor (), lint,
			 *die_check_registrar::inst ());

  for (all_dies_iterator<dwarf> it = all_dies_iterator<dwarf> (dw);
       it != all_dies_iterator<dwarf> (); ++it)
    ctx.die (it);
}