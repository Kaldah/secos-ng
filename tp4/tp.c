/* GPLv2 (c) Airbus */
#include <debug.h>
#include <cr.h>
#include <pagemem.h>


void tp() {
	// q1: afficher CR3 courant
	uint32_t cr3_before = get_cr3();
	debug("cr3: 0x%x\n", cr3_before);

	/*
	 * q2/q4: préparer PGD et PTB en mémoire physique avant d'activer la pagination.
	 * On suppose ici que 0x600000 et 0x601000 sont des adresses physiques
	 * accessibles en mode linéaire (identity-mapped) avant l'activation de PG.
	 */
	pde32_t* pgd = (pde32_t*)0x600000;
	pte32_t* ptb = (pte32_t*)0x601000;
	debug("PGD phys: 0x%x, PTB phys: 0x%x\n", (uint32_t)pgd, (uint32_t)ptb);

	/* Zeroer complètement les tables pour être sûr */
	__clear_page((void*)pgd);
	__clear_page((void*)ptb);

	/* Remplir la PTB: mapper en identity la plage basse (4 MiB) pour couvrir le
	 * noyau et la pile. Un PT contient 1024 entrées de 4K => 4MiB.
	 */
	/* Note: use an unsigned type to match PTE32_PER_PT (unsigned long) and
	 * avoid signed/unsigned comparison warnings treated as errors. */
	for (unsigned long i = 0; i < PTE32_PER_PT; i++) {
		/* Utilisation de la macro fournie pour initialiser l'entrée */
		pg_set_entry(&ptb[i], PG_RW|PG_USR, (uint32_t)i);
		/* limiter les logs pour ne pas spammer */
		if (i < 4)
			debug("PTB[%lu] configurée: addr=0x%x, p=%d, rw=%d, lvl=%d\n",
				  i, ptb[i].addr << PG_4K_SHIFT, ptb[i].p, ptb[i].rw, ptb[i].lvl);
	}

	/* Configurer la première entrée de la PGD pour pointer vers la PTB */
    /* Utilisation de la macro fournie pour initialiser l'entrée du PGD */
    pg_set_entry(&pgd[0], PG_RW|PG_USR, ((uint32_t)ptb) >> PG_4K_SHIFT);
    debug("PGD[0] configurée: addr=0x%x, p=%d, rw=%d, lvl=%d\n",
	    pgd[0].addr << PG_4K_SHIFT, pgd[0].p, pgd[0].rw, pgd[0].lvl);

	/* Recharger CR3 avec l'adresse physique du PGD (vide le TLB) */
	set_cr3((uint32_t)pgd);
	debug("Nouveau cr3: 0x%x\n", get_cr3());

	/* q3/q6: activer la pagination (CR0.PG)
	 * ------------------------------------------------------------------
	 * NOTE: pour l'exécution finale dans QEMU nous laissons l'activation
	 * commentée afin d'éviter un redémarrage en boucle si l'environnement
	 * n'a pas exactement les mêmes allocations physiques ou si d'autres
	 * mappings nécessaires (pile, code) ne sont pas couverts.
	 *
	 * Réponse Q3 (commentaire): si on active CR0.PG sans préparer les
	 * tables (PGD/PTB) correctement pour les zones mémoire utilisées par
	 * le noyau (code, pile, data), le CPU rencontrera des fautes de page
	 * critiques et le système tombera en erreur / redémarrera.
	 * Ici on montre comment l'activer mais on garde la ligne `set_cr0(cr0)`
	 * commentée — décommentez-la pour effectuer l'expérience une fois
	 * que vous avez vérifié vos mappings.
	 */
	uint32_t cr0 = get_cr0();
	cr0 |= CR0_PG;
	// Activer la pagination (Q3)
	// Note: si vous préférez utiliser les macros fournies pour remplir les
	// entrées, `pg_set_entry(_e_, _attr_, _pfn_)` et `pg_set_zero(_e_)`
	// existent dans `kernel/include/pagemem.h`.
	set_cr0(cr0);
	debug("CR0 après activation de la pagination: 0x%x\n", get_cr0());

	/* Vérification rapide d'une PTE */
	debug("Vérif PTB[0]: addr=0x%x, p=%d, rw=%d, lvl=%d\n",
		  ptb[0].addr << PG_4K_SHIFT, ptb[0].p, ptb[0].rw, ptb[0].lvl);

}
