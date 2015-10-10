/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Purpose: manage read-only LDT alias
 *
 * Author: Stas Sergeev
 */

#include "cpu.h"
#include "dpmi.h"
#include "vgaemu.h"
#include "dosemu_debug.h"
#include "msdos_ldt.h"

#define LDT_INIT_LIMIT 0xfff

unsigned char *ldt_alias;
static unsigned short dpmi_ldt_alias;

int msdos_ldt_setup(unsigned char *alias, unsigned short alias_sel)
{
    if (SetSelector(alias_sel, DOSADDR_REL(alias),
		    LDT_INIT_LIMIT, 0,
                  MODIFY_LDT_CONTENTS_DATA, 0, 0, 0, 0))
	return 0;
    ldt_alias = alias;
    dpmi_ldt_alias = alias_sel;
    return 1;
}

u_short DPMI_ldt_alias(void)
{
  return dpmi_ldt_alias;
}

int msdos_ldt_fault(struct sigcontext *scp)
{
    int pref_seg = -1, done = 0;
    unsigned char *csp;

    csp = (unsigned char *) SEL_ADR(_cs, _eip);
    do {
      switch (*(csp++)) {
         case 0x66:      /* operand prefix */  /*prefix66=1;*/ break;
         case 0x67:      /* address prefix */  /*prefix67=1;*/ break;
         case 0x2e:      /* CS */              pref_seg=_cs; break;
         case 0x3e:      /* DS */              pref_seg=_ds; break;
         case 0x26:      /* ES */              pref_seg=_es; break;
         case 0x36:      /* SS */              pref_seg=_ss; break;
         case 0x65:      /* GS */              pref_seg=_gs; break;
         case 0x64:      /* FS */              pref_seg=_fs; break;
         case 0xf2:      /* repnz */
         case 0xf3:      /* rep */             /*is_rep=1;*/ break;
         default: done=1;
      }
    } while (!done);

    if (pref_seg == -1)
	pref_seg = _ds;
    if (pref_seg == dpmi_ldt_alias) {
	unsigned limit = GetSegmentLimit(dpmi_ldt_alias);
	D_printf("DPMI: expanding LDT, old_lim=0x%x\n", limit);
	SetSegmentLimit(dpmi_ldt_alias, limit + DPMI_page_size);
	return 1;
    }

    return 0;
}

int msdos_ldt_pagefault(struct sigcontext *scp)
{
    if ((unsigned char *)_cr2 >= ldt_alias &&
	  (unsigned char *)_cr2 < ldt_alias + LDT_ENTRIES*LDT_ENTRY_SIZE) {
	instr_emu(scp, 1, 10);
	return 1;
    }
    return 0;
}
