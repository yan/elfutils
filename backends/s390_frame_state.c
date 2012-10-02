/* Fetch live process Dwarf_Frame_State from PID.
   Copyright (C) 2012 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include "../libdw/cfi.h"
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#define BACKEND s390_
#include "libebl_CPU.h"

#define BUILD_BUG_ON_ZERO(x) (sizeof (char [(x) ? -1 : 1]) - 1)

#include "core-get-pc.c"

Dwarf_Frame_State *
s390_frame_state (Ebl *ebl, pid_t pid, bool pid_attach, Elf *core __attribute__ ((unused)))
{
  /* gcc/config/ #define DWARF_FRAME_REGISTERS.  */
  const size_t nregs = 34;
#ifdef __s390__
  struct user user_regs;
#endif /* __s390__ */
  /* Needless initialization for old GCCs.  */
  Dwarf_Addr core_pc = 0;
  bool core_pc_set;

  if (pid_attach)
    {
#ifndef __s390__
      abort ();
#else /* __s390__ */
      if (ptrace (PTRACE_ATTACH, pid, NULL, NULL) != 0)
	return NULL;
      for (;;)
	{
	  int status;
	  if (waitpid (pid, &status, 0) != pid || !WIFSTOPPED (status))
	    {
	      ptrace (PTRACE_DETACH, pid, NULL, NULL);
	      return NULL;
	    }
	  if (WSTOPSIG (status) == SIGSTOP)
	    break;
	  if (ptrace (PTRACE_CONT, pid, NULL, (void *) (uintptr_t) WSTOPSIG (status)) != 0)
	    {
	      ptrace (PTRACE_DETACH, pid, NULL, NULL);
	      return NULL;
	    }
	}
#endif /* __s390__ */
    }
  if (pid)
    {
#ifndef __s390__
      abort ();
#else /* __s390__ */
      ptrace_area parea;
      parea.process_addr = (uintptr_t) &user_regs;
      parea.kernel_addr = 0;
      parea.len = sizeof (user_regs);
      if (ptrace (PTRACE_PEEKUSR_AREA, child, &parea, NULL) != 0)
	{
	  if (pid_attach)
	    ptrace (PTRACE_DETACH, pid, NULL, NULL);
	  return NULL;
	}
#endif /* __s390__ */
    }
  if (core)
    {
      /* Fetch PSWA.  */
      core_pc_set = core_get_pc (core, &core_pc, ebl->class == ELFCLASS32 ? 0x4c : 0x50);
      if (! core_pc_set)
	return NULL;
    }

  Dwarf_Frame_State_Base *base = malloc (sizeof (*base));
  if (base == NULL)
    return NULL;
  base->ebl = ebl;
  base->nregs = nregs;
  base->regs_bits = ebl->class == ELFCLASS64 ? 64 : 32;
  Dwarf_Frame_State *state = malloc (sizeof (*state) + sizeof (*state->regs) * nregs);
  if (state == NULL)
    {
      free (base);
      return NULL;
    }
  base->unwound = state;
  state->base = base;
  state->unwound = NULL;
  state->pc_state = DWARF_FRAME_STATE_ERROR;

  memset (state->regs_set, 0, sizeof (state->regs_set));
  if (pid)
    {
#ifndef __s390__
      abort ();
#else /* __s390__ */
      dwarf_frame_state_reg_set (state, 0, user_regs.rax);
      dwarf_frame_state_reg_set (state, 1, user_regs.rdx);
      dwarf_frame_state_reg_set (state, 2, user_regs.rcx);
      dwarf_frame_state_reg_set (state, 3, user_regs.rbx);
      dwarf_frame_state_reg_set (state, 4, user_regs.rsi);
      dwarf_frame_state_reg_set (state, 5, user_regs.rdi);
      dwarf_frame_state_reg_set (state, 6, user_regs.rbp);
      dwarf_frame_state_reg_set (state, 7, user_regs.rsp);
      dwarf_frame_state_reg_set (state, 8, user_regs.r8);
      dwarf_frame_state_reg_set (state, 9, user_regs.r9);
      dwarf_frame_state_reg_set (state, 10, user_regs.r10);
      dwarf_frame_state_reg_set (state, 11, user_regs.r11);
      dwarf_frame_state_reg_set (state, 12, user_regs.r12);
      dwarf_frame_state_reg_set (state, 13, user_regs.r13);
      dwarf_frame_state_reg_set (state, 14, user_regs.r14);
      dwarf_frame_state_reg_set (state, 15, user_regs.r15);
      dwarf_frame_state_reg_set (state, 16, user_regs.rip);


      for (unsigned u = 0; u < 16; u++)
	dwarf_frame_state_reg_set (state, 0 + u, user_regs.regs.gprs[u]);
      /* Avoid a conversion double -> integer.  */
      BUILD_BUG_ON_ZERO (sizeof (*user_regs.regs.fp_regs.fprs) - sizeof (*state->regs));
      for (unsigned u = 0; u < 16; u++)
	dwarf_frame_state_reg_set (state, 16 + u, *(const __typeof (*state->regs) *) &user_regs.regs.fp_regs.fprs[u]);
      dwarf_frame_state_reg_set (state, 65, user_regs.regs.psw.addr);
#endif /* __s390__ */
    }
  if (core)
    {
      state->pc = core_pc;
      state->pc_state = DWARF_FRAME_STATE_PC_SET;
    }

  return state;
}

__typeof (s390_frame_state)
     s390x_frame_state
     __attribute__ ((alias ("s390_frame_state")));

void
s390_frame_detach (Ebl *ebl __attribute__ ((unused)), pid_t pid)
{
  ptrace (PTRACE_DETACH, pid, NULL, NULL);
}

__typeof (s390_frame_detach)
     s390x_frame_detach
     __attribute__ ((alias ("s390_frame_detach")));

void
s390_normalize_pc (Ebl *ebl __attribute__ ((unused)), Dwarf_Addr *pc)
{
  /* Clear S390 bit 31.  */
  *pc &= (1U << 31) - 1;
}
