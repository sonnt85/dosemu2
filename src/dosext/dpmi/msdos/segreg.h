#ifndef SEGREG_H
#define SEGREG_H

int msdos_fault(struct sigcontext *scp);

typedef struct x86_ins {
  int _32bit:1;	/* 16/32 bit code */
  unsigned address_size; /* in bytes so either 4 or 2 */
  unsigned operand_size;
} x86_ins;

int x86_handle_prefixes(struct sigcontext *scp, unsigned cs_base,
	x86_ins *x86);
int x86_instr_len(unsigned char *p, int is_32);

#endif
