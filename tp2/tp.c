/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>
void bp_handler() {
   // TODO
   debug("Breakpoint exception handler invoked.\n");
	// Q5 : objdump --disassemble=bp_handler tp.o
	//   16:   90                      nop
	//   17:   c9                      leave
	//   18:   c3                      ret
	// Retour avec un RET alors que le contexte d'interruption attend un IRET
	// Cela dépile la pile sous forme de "frame" de fonction
    // Alors que la fonction a été appelée à l'arrivée d'une int
    // ce qui n'est pas cohérent...

	uint32_t val;
   	asm volatile ("mov 4(%%ebp), %0":"=r"(val));
	debug("Return address from stack frame (EIP): 0x%x\n", val);
	// cette valeur est l'adresse dans bp_trigger
    // qui suit l'instruction int3.

	// On utilise l'instruction PUSHA pour empiler les GPRs
	// afin de simuler le contexte d'interruption attendu par IRET

	asm volatile ("pusha");
	debug("#BP handling\n");
	// On récupère l'EIP à partir de la pile
	uint32_t eip;
	// L'EIP est à l'offset 4 dans la frame de fonction
	asm volatile ("mov 4(%%ebp), %0":"=r"(eip));
	debug("EIP = %x\n", (unsigned int) eip);
	asm volatile ("popa");
	asm volatile ("leave; iret");
	// Q11 : Dev en C rajoute les frames de fonction non désirées...

}

void bp_trigger() {
	// TODO
	debug("Triggering breakpoint exception...\n");
	asm volatile ("int $3");
	debug("Breakpoint exception returned.\n");
}

void tp() {
	// TODO print idtr
	idt_reg_t current_idtr;
	get_idtr(current_idtr);
	debug("IDTR base: %ld, IDTR limit: %d\n", current_idtr.addr, current_idtr.limit);
	
	// TODO Modify IDT to set breakpoint handler bp_handler
	int_desc_t* idt_entries = (int_desc_t*)current_idtr.addr;
	idt_entries[3].offset_1 = (uint32_t)bp_handler & 0xFFFF;
	idt_entries[3].offset_2 = (uint32_t)bp_handler >> 16;

	/*
	int_desc_t *bp_dsc = &current_idtr.desc[3];
	bp_dsc->offset_1 = (uint16_t)((uint32_t)bp_handler);
	bp_dsc->offset_2 = (uint16_t)(((uint32_t)bp_handler)>>16);
	*/
	// TODO call bp_trigger
   bp_trigger();

}
