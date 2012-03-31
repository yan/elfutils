/* Find line information for relocatable address.
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

   In addition, as a special exception, Red Hat, Inc. gives You the
   additional right to link the code of Red Hat elfutils with code licensed
   under any Open Source Initiative certified open source license
   (http://www.opensource.org/licenses/index.php) which requires the
   distribution of source code with any binary distribution and to
   distribute linked combinations of the two.  Non-GPL Code permitted under
   this exception must only link to the code of Red Hat elfutils through
   those well defined interfaces identified in the file named EXCEPTION
   found in the source code files (the "Approved Interfaces").  The files
   of Non-GPL Code may instantiate templates or use macros or inline
   functions from the Approved Interfaces without causing the resulting
   work to be covered by the GNU General Public License.  Only Red Hat,
   Inc. may make changes or additions to the list of Approved Interfaces.
   Red Hat's grant of this exception is conditioned upon your not adding
   any new exceptions.  If you wish to add a new Approved Interface or
   exception, please contact Red Hat.  You must obey the GNU General Public
   License in all respects for all of the Red Hat elfutils code and other
   code used in conjunction with Red Hat elfutils except the Non-GPL Code
   covered by this exception.  If you modify this file, you may extend this
   exception to your version of the file, but you are not obligated to do
   so.  If you do not wish to provide this exception without modification,
   you must delete this exception statement from your version and license
   this file solely under the GPL without exception.

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

#include "relocate.h"
#include <assert.h>


Dwarf_Line *
dwarf_getsrc_relocatable (Dwarf_Die *cudie, Dwarf_Relocatable *reloc)
{
  Dwarf_Lines *lines;
  size_t nlines;

  if (INTUSE(dwarf_getsrclines) (cudie, &lines, &nlines) != 0)
    return NULL;

  /* First we can partially resolve the relocatable address to
     a symndx and a section-relative offset.  */
  GElf_Sym sym;
  GElf_Word shndx;
  if (reloc->valp != NULL)
    {
      int result = INTUSE(dwarf_relocatable_info) (reloc,
						   NULL, NULL, NULL, NULL);
      if (unlikely (result < 0))
	return NULL;
      assert (reloc->valp == NULL);
      shndx = result;
    }
  else if (reloc->symndx == STN_UNDEF)
    shndx = 0;
  else
    {
      /* The relocation is already resolved to a symndx and
	 section-relative address, but we have to recover that shndx.  */
      struct dwarf_section_reloc *const r
	= reloc->cu->dbg->relocate->sectionrel[reloc->sec];
      if (unlikely (gelf_getsymshndx (r->symdata, r->symxndxdata,
				      reloc->symndx, &sym, &shndx) == NULL))
	{
	  __libdw_seterrno (DWARF_E_RELBADSYM);
	  return NULL;
	}
      if (sym.st_shndx == SHN_ABS)
	shndx = 0;
      else if (likely (sym.st_shndx < SHN_LORESERVE)
	       && likely (sym.st_shndx != SHN_UNDEF))
	shndx = sym.st_shndx;
      else if (sym.st_shndx != SHN_XINDEX)
	{
	  __libdw_seterrno (DWARF_E_RELUNDEF);
	  return NULL;
	}
    }

  if (lines->reloc == NULL)
    {
      if (shndx == 0)
	return INTUSE(dwarf_getsrc_die) (cudie, reloc->adjust);
      else
	goto nomatch;
    }

  /* The lines are sorted by address, so we can use binary search.  */
  const Dwarf_Addr addr = reloc->adjust;
  size_t l = 0, u = nlines;
  while (l < u)
    {
      size_t idx = (l + u) / 2;
      const GElf_Word this_shndx = lines->reloc[idx * 2 + 1];
      if (shndx < this_shndx)
	u = idx;
      else if (shndx > this_shndx)
	l = idx + 1;
      else if (addr < lines->info[idx].addr)
	u = idx;
      else if (addr > lines->info[idx].addr || lines->info[idx].end_sequence)
	l = idx + 1;
      else
	return &lines->info[idx];
    }

  if (nlines > 0)
    assert (lines->info[nlines - 1].end_sequence);

  /* If none were equal, the closest one below is what we want.  We
     never want the last one, because it's the end-sequence marker
     with an address at the high bound of the CU's code.  If the debug
     information is faulty and no end-sequence marker is present, we
     still ignore it.  */
  if (u > 0 && u < nlines && addr > lines->info[u - 1].addr)
    {
      while (lines->info[u - 1].end_sequence && u > 0)
	--u;
      if (u > 0)
	return &lines->info[u - 1];
    }

 nomatch:
  __libdw_seterrno (DWARF_E_ADDR_OUTOFRANGE);
  return NULL;
}