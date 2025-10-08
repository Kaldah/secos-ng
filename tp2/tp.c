/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>
void bp_handler() {
   // TODO
}

void bp_trigger() {
	// TODO
}

void tp() {
	// TODO print idtr
	idt_reg_t current_idtr;
	get_idtr(current_idtr);
	debug("Current IDTR:\n");
	debug("Base: 0x%x\n", current_idtr.addr);
	debug("Limit: 0x%x\n", current_idtr.limit);
	// TODO call bp_trigger
   //bp_trigger();

}
