/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <string.h>

#define tss_dsc(_dSc_,_tSs_)                                            \
   ({                                                                   \
      raw32_t addr    = {.raw = _tSs_};                                 \
      (_dSc_)->raw    = sizeof(tss_t);                                  \
      (_dSc_)->base_1 = addr.wlow;                                      \
      (_dSc_)->base_2 = addr._whigh.blow;                               \
      (_dSc_)->base_3 = addr._whigh.bhigh;                              \
      (_dSc_)->type   = SEG_DESC_SYS_TSS_AVL_32;                        \
      (_dSc_)->p      = 1;                                              \
   })


void userland() {
   asm volatile ("mov %eax, %cr0");
}

void print_gdt_content(gdt_reg_t gdtr_ptr) {
    seg_desc_t* gdt_ptr;
    gdt_ptr = (seg_desc_t*)(gdtr_ptr.addr);
    int i=0;
    while ((uint32_t)gdt_ptr < ((gdtr_ptr.addr) + gdtr_ptr.limit)) {
        uint32_t start = gdt_ptr->base_3<<24 | gdt_ptr->base_2<<16 | gdt_ptr->base_1;
        uint32_t end;
        if (gdt_ptr->g) {
            end = start + ( (gdt_ptr->limit_2<<16 | gdt_ptr->limit_1) <<12) + 4095;
        } else {
            end = start + (gdt_ptr->limit_2<<16 | gdt_ptr->limit_1);
        }
        debug("%d ", i);
        debug("[0x%x ", start);
        debug("- 0x%x] ", end);
        debug("seg_t: 0x%x ", gdt_ptr->type);
        debug("desc_t: %d ", gdt_ptr->s);
        debug("priv: %d ", gdt_ptr->dpl);
        debug("present: %d ", gdt_ptr->p);
        debug("avl: %d ", gdt_ptr->avl);
        debug("longmode: %d ", gdt_ptr->l);
        debug("default: %d ", gdt_ptr->d);
        debug("gran: %d ", gdt_ptr->g);
        debug("\n");
        gdt_ptr++;
        i++;
    }
}

/* Helpers to extract base and limit from a seg_desc_t */
static inline uint32_t seg_desc_get_base(const seg_desc_t *d)
{
    return (d->base_1) | ((uint32_t)d->base_2 << 16) | ((uint32_t)d->base_3 << 24);
}

static inline uint32_t seg_desc_get_limit(const seg_desc_t *d)
{
    uint32_t lim = ((uint32_t)d->limit_2 << 16) | (uint32_t)d->limit_1;
    if (d->g)
        return (lim << 12) | 0xFFF;
    return lim;
}

void q2() {
    gdt_reg_t gdtr;
    get_gdtr(gdtr); /* la macro écrit dans la variable gdtr */
    print_gdt_content(gdtr);
}

void q3() {
    // Valeurs des sélecteurs (CS, DS, SS, ES, FS, GS)
    debug("CS: 0x%x\n", get_cs());
    debug("DS: 0x%x\n", get_ds());
    debug("SS: 0x%x\n", get_ss());
    debug("ES: 0x%x\n", get_es());
    debug("FS: 0x%x\n", get_fs());
    debug("GS: 0x%x\n", get_gs());
}

void q4() {
   // Read and write outside the kernel memory
}

void q5(seg_desc_t *gdt, gdt_reg_t *gdtr) {
    /* Construire une nouvelle GDT (3 entrées) :
     *  - index 0 : NULL
     *  - index 1 : segment code 32-bit, exécutable+lecture, DPL=0
     *  - index 2 : segment données 32-bit, lecture+écriture, DPL=0
     *
     * La table doit être alignée sur 8 octets. Le paramètre 'gdt' désigne
     * l'espace où écrire les descripteurs et 'gdtr' recevra la base et la
     * limite (limit = taille - 1).
     */
    /* Remplir 'gdtr' (base + limit) */
    seg_desc_t* new_gdt = gdt;
    gdtr->addr = (offset_t)gdt;
    gdtr->limit = 3 * sizeof(seg_desc_t) - 1; // 3 descripteurs (NULL, code, data)
    // Descripteur NULL
    new_gdt[0] = (seg_desc_t){0};
    // Descripteur Code, 32 bits RX, flat, indice 1
    new_gdt[1] = (seg_desc_t){
        .limit_1 = 0xFFFF,
        .base_1 = 0x0000,
        .base_2 = 0x00,
        .type = 0xA, // Code segment, RX
        .s = 1, // Code/Data segment
        .dpl = 0, // Ring 0
        .p = 1, // Present
        .limit_2 = 0xF,
        .avl = 0,
        .l = 0, // 32 bits
        .d = 1, // Default operation size (0=16 bits, 1=32 bits)
        .g = 1, // Granularity (0=byte, 1=4KB)
        .base_3 = 0x00
    };
    // Descripteur Données, 32 bits RW, flat, indice 2
    new_gdt[2] = (seg_desc_t){
        .limit_1 = 0xFFFF,
        .base_1 = 0x0000,
        .base_2 = 0x00,
        .type = 0x2, // Data segment, RW
        .s = 1, // Code/Data segment
        .dpl = 0, // Ring 0
        .p = 1, // Present
        .limit_2 = 0xF,
        .avl = 0,
        .l = 0, // 32 bits
        .d = 1, // Default operation size (0=16 bits, 1=32 bits)
        .g = 1, // Granularity (0=byte, 1=4KB)
        .base_3 = 0x00
    };
}

void q6(gdt_reg_t *gdtr) {
   /* Load the new GDT and reload segment selectors */
   set_gdtr(*gdtr);
   /* reload selectors using the helper macros */
   set_cs(gdt_krn_seg_sel(1)); /* code @ index 1 */
   set_ds(gdt_krn_seg_sel(2)); /* data @ index 2 */
   set_ss(gdt_krn_seg_sel(2));
   set_es(gdt_krn_seg_sel(2));
   set_fs(gdt_krn_seg_sel(2));
   set_gs(gdt_krn_seg_sel(2));
}

void q7() {
    q2();
}

void q8() {
    /* 
    Essayer de charger un descripteur de segment de code dans le sélecteur DS.
    Que se passe-t-il ? Est-ce conforme avec ce que décrit la documentation Intel à ce sujet ?
    Faire de même avec un descripteur de segment de données pour le sélecteur CS.
    */
    set_cs(gdt_krn_seg_sel(2)); // Charger un descripteur de segment de données dans CS
    set_ds(gdt_krn_seg_sel(1)); // Charger un descripteur de segment de code dans DS
}

void q9() {
   // Write and read outside the kernel memory
   /*
    Dans la GDT précédente, définir une nouvelle entrée contenant un descripteur
    ayant les caractéristiques suivantes :

    data, ring 0
    32 bits RW
    base 0x600000
    limite 32 octets
   */
    // Ajouter le descripteur dans la GDT actuelle
    seg_desc_t* gdt;
    gdt_reg_t gdtr;
    get_gdtr(gdtr);
    gdt = (seg_desc_t*)(gdtr.addr);
    /* Descripteur Données, 32 bits RW, base 0x600000, limite 32 octets, indice 3 */
    uint32_t base3 = 0x600000;
    gdt[3] = (seg_desc_t){
        .limit_1 = 31 & 0xFFFF, /* Limite 32 octets */
        .base_1 = (uint16_t)(base3 & 0xFFFF), /* base low */
        .base_2 = (uint8_t)((base3 >> 16) & 0xFF),
        .type = 0x2, // Data segment, RW
        .s = 1, // Code/Data segment
        .dpl = 0, // Ring 0
        .p = 1, // Present
        .limit_2 = (31 >> 16) & 0xF,
        .avl = 0,
        .l = 0, // 32 bits
        .d = 1, // Default operation size (0=16 bits, 1=32 bits)
        .g = 0, // Granularity (0=byte, 1=4KB)
        .base_3 = (uint8_t)((base3 >> 24) & 0xFF)
    };
    // Recharger la GDT
    gdtr.limit += sizeof(seg_desc_t); // Augmenter la limite de la GDT
    set_gdtr(gdtr);
}

void q10() {
   /* : Charger le sélecteur de segment "es" de manière à adresser ce nouveau descripteur de données
   puis ré-exécuter la copie _memcpy8(dst, src, 32);. Que se passe-t-il ? 
   Pourquoi n'y a-t-il pas de faute mémoire alors que le pointeur dst est NULL ?
   */
    seg_desc_t *gdtp;
    gdt_reg_t gdtr;
    get_gdtr(gdtr);
    gdtp = (seg_desc_t*)(gdtr.addr);
    uint32_t base_addr = seg_desc_get_base(&gdtp[3]);
    set_es(gdt_krn_seg_sel(3)); /* Charger le sélecteur ES pour le nouveau descripteur */
    _memcpy8(NULL, (void*)base_addr, 32);
}

void q11() {
    /*: De même, effectuer à présent une copie de 64 octets.
     * Cette opération dépasse la limite du descripteur que nous avons
     * défini à l'indice 3 (base=0x600000, limite=32 octets). En chargeant
     * ES avec ce descripteur, les écritures effectuées via ES sont
     * vérifiées par le processeur par rapport à base+limit. Copier 64
     * octets écrit au-delà de cette limite et provoque une exception
     * processeur (par ex. General Protection Fault). Le noyau, n'ayant
     * pas d'handler installé pour cette exception dans ce TP, cela se
     * traduit par une erreur fatale (crash / arrêt de la VM).
     *
     * Remarque : la copie de 32 octets ne déclenche pas d'exception
     * car elle reste dans la limite (0..31) définie pour ce segment.
     */
    seg_desc_t *gdtp2;
    gdt_reg_t gdtr;
    get_gdtr(gdtr);
    gdtp2 = (seg_desc_t*)(gdtr.addr);
    uint32_t base_addr2 = seg_desc_get_base(&gdtp2[3]);
    set_es(gdt_krn_seg_sel(3)); /* Charger le sélecteur ES pour le nouveau descripteur */
    _memcpy8(NULL, (void*)base_addr2, 64);
}

void q12() {
   /*
   Ajouter à la GDT précédente deux nouveaux descripteurs aux index de votre choix, avec les propriétés suivantes :

    Code, 32 bits RX, ring 3, flat
    Data, 32 bits RW, ring 3, flat
    */
    seg_desc_t* gdt;
    gdt_reg_t gdtr;
    get_gdtr(gdtr);
    gdt = (seg_desc_t*)(gdtr.addr);
    // Descripteur Code, 32 bits RX, flat, ring 3, indice 4
    gdt[4] = (seg_desc_t){
        .limit_1 = 0xFFFF,
        .base_1 = 0x0000,
        .base_2 = 0x00,
        .type = 0xA, // Code segment, RX
        .s = 1, // Code/Data segment
        .dpl = 3, // Ring 3
        .p = 1, // Present
        .limit_2 = 0xF,
        .avl = 0,
        .l = 0, // 32 bits
        .d = 1, // Default operation size (0=16 bits, 1=32 bits)
        .g = 1, // Granularity (0=byte, 1=4KB)
        .base_3 = 0x00
    };
    // Descripteur Données, 32 bits RW, flat, ring 3, indice 5
    gdt[5] = (seg_desc_t){
        .limit_1 = 0xFFFF,
        .base_1 = 0x0000,
        .base_2 = 0x00,
        .type = 0x2, // Data segment, RW
        .s = 1, // Code/Data segment
        .dpl = 3, // Ring 3
        .p = 1, // Present
        .limit_2 = 0xF,
        .avl = 0,
        .l = 0, // 32 bits
        .d = 1, // Default operation size (0=16 bits, 1=32 bits)
        .g = 1, // Granularity (0=byte, 1=4KB)
        .base_3 = 0x00
    };
    // Recharger la GDT
    gdtr.limit += 2 * sizeof(seg_desc_t); // Augmenter la limite de la GDT
    set_gdtr(gdtr);
}

void q13(gdt_reg_t *gdtr) {
   /*
   Charger progressivement les registres de segments avec des sélecteurs qui pointent vers les
   descripteurs ring 3 :

    Que se passe-t-il lors du chargement de DS/ES/FS/GS ?
    Que se passe-t-il lors du chargement de SS ?
    Concernant la mise à jour du sélecteur CS, effectuer un "far jump" (cf. fonction farjump()
    définie dans kernel/include/segmem.h) vers la fonction userland(), avec en paramètre
    (cf. type fptr32_t défini dans kernel/include/types.h) avec l'adresse de la fonction userland()
    comme nouvel offset, et l'index permettant de sélectionner le descripteur de code ring 3 défini en
    question Q12 et un RPL à 3 comme nouveau sélecteur.
    Que se passe-t-il ? Comment un noyau pourrait-il faire autrement pour démarrer une tâche en ring 3 ?
   */
    set_ds(gdt_krn_seg_sel(5)); // Charger un descripteur de segment de données ring 3 dans DS
    set_es(gdt_krn_seg_sel(5)); // Charger un descripteur de segment de données ring 3 dans ES
    set_fs(gdt_krn_seg_sel(5)); // Charger un descripteur de segment de données ring 3 dans FS
    set_gs(gdt_krn_seg_sel(5)); // Charger un descripteur de segment de données ring 3 dans GS

    // Charger un descripteur de segment de données ring 3 dans SS
    //set_ss(gdt_krn_seg_sel(5)); // Fatal exception !
    printf("Trying to load SS with kernel data segment descriptor\n");
     // Charger un descripteur de segment de données ring 3 dans SS
    // Charger un descripteur de segment de code ring 3 dans CS et sauter vers userland
    tss_t TSS;
    TSS.s0.esp = get_ebp();
    TSS.s0.ss  = gdt_krn_seg_sel(2);
    tss_dsc((seg_desc_t*)((gdtr->addr) + 6 * sizeof(seg_desc_t)), (offset_t)&TSS);
    set_tr(gdt_krn_seg_sel(6));

    fptr32_t fp;
    fp.offset  = (uint32_t)userland;
    fp.segment = (uint16_t)(gdt_krn_seg_sel(4) | 0x3); /* selector index=4, RPL=3 */
    printf("Far jumping to userland (CS=0x%x)\n", fp.segment);
    farjump(fp);
}

void tp() {
    /* Storage for the current GDT so q5 builds it and q6 can load it */
    static seg_desc_t current_gdt[7] __attribute__((aligned(8)));
    static gdt_reg_t current_gdtr;

	// TODO
    q2();
    printf("----\n");
    q3();
    printf("----\n");
    q5(current_gdt, &current_gdtr);
    printf("----\n");
    q6(&current_gdtr); 
    printf("----\n");
    q7();
    printf("----\n");
    //q8(); // fatal exception !
    q9();
    printf("----\n");
    q10();
    printf("----\n");
    //q11(); // fatal exception !
    printf("Q12----\n");
    q12();
    printf("Q13----\n");
    q13(&current_gdtr);
}
