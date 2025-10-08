# TP1 — Réponses

Ce document regroupe les réponses et observations pour les questions du TP1.

## Q1 — SGDT et macro associée

Rappel succinct sur l'instruction SGDT :

- Rôle : SGDT (Store Global Descriptor Table Register) copie le contenu du registre GDTR (limit + base de la GDT) en mémoire.
- Opérande : SGDT prend une adresse mémoire destination (opérande mémoire). Syntaxe (AT&T/GCC inline) : `sgdt m`.
- Format écrit en mémoire :
  - En mode protégé 32-bit : 6 octets — un `limit` de 16 bits suivi d'un `base` de 32 bits.
  - En mode long (x86-64) : 10 octets — un `limit` de 16 bits suivi d'un `base` de 64 bits.
- Effets : n'affecte pas de registres d'état. Écrit uniquement en mémoire la paire (limit, base).
- Usage typique : lire l'adresse et la taille de la GDT pour inspection/debug.

Macro et type dans le dépôt :

- Macro : `get_gdtr(aLocation)` dans `kernel/include/segmem.h`

```c
#define get_gdtr(aLocation)       \
   asm volatile ("sgdt %0"::"m"(aLocation):"memory")
```

- Type utile : `gdt_reg_t` (défini dans `kernel/include/segmem.h`) :

```c
typedef struct global_descriptor_table_register
{
   uint16_t            limit;   /* dt limit = size - 1 */
   union
   {
      offset_t         addr;    /* base address */
      seg_desc_t       *desc;
   };
} __attribute__((packed)) gdt_reg_t;
```

Exemple d'utilisation :

```c
gdt_reg_t gdtr;
get_gdtr(gdtr); /* stocke limit+base dans gdtr */
debug("GDT base 0x%lx limit 0x%x\n", gdtr.addr, gdtr.limit);
```

---

## Q2 — Afficher le contenu de la GDT courante

(voir `tp.c` : fonction `print_gdt_content`).

> Tâche : appeler `print_gdt_content()` après avoir récupéré `gdtr` via `get_gdtr`.

Exemple d'appel :

```c
gdt_reg_t gdtr;
get_gdtr(gdtr);
print_gdt_content(gdtr.addr, gdtr.limit+1); /* ou selon prototype fourni */
```

(Remarque : adapter l'appel selon la signature exacte de `print_gdt_content` déjà fournie dans `tp.c`.)

---

## Q3 — Valeurs des sélecteurs (CS, DS, SS, ES, FS, GS)

Procédure recommandée :

- Utiliser les macros `get_cs()`, `get_ds()`, `get_ss()`, etc. définies dans `kernel/include/segmem.h`.
- Pour chaque sélecteur lu, décomposer le champ index/ti/rpl ou utiliser `gdt_reg_t` + index*8 pour retrouver le descripteur correspondant et l'afficher.

Exemple succinct :

```c
uint16_t cs = get_cs();
debug("CS = 0x%x index=%u rpl=%u ti=%u\n", cs, cs>>3, cs&0x3, (cs>>2)&1);
```

---

## Q4 — Observations sur la configuration par défaut

Points attendus à observer et à rédiger :

- GRUB charge une GDT "flat" typique pour démarrer le noyau en mode protégé : descripteurs code/data couvrant tout l'espace (base=0, limite=4GB ou granularity=page) et RPL 0 pour le noyau.
- En conséquence, la séparation mémoire via segmentation est minimale (flat model) : protection par segmentation n'est pas utilisée par défaut, on repose sur la pagination pour l'isolation.

---

## Q5–Q11 — Reconfiguration de la GDT (flat + tests)

Structure proposée pour les réponses et actes :

- Q5 : montrer l'adresse choisie pour la nouvelle GDT (alignée 8 octets), montrer création des descripteurs (indice 0 NULL, 1=code RX, 2=data RW).
- Q6 : utiliser `set_gdtr()` (ou `lgdt`) et mettre à jour les sélecteurs (CS via far jump, SS/DS/ES/FS/GS via movw vers registres). Utiliser `gdt_krn_seg_sel(idx)` pour fabriquer les sélecteurs.
- Q7 : rappeler `print_gdt_content()` pour vérifier.
- Q8 : tentative d'assigner un descripteur code à DS ou un descripteur data à CS — expliquer les exceptions / comportements conformes à la doc Intel.
- Q9–Q11 : expliquer le test `es` avec base/limit restreints, pourquoi écrire vers NULL peut marcher pour une copie plus petite que la limite, et échouer lorsque la copie dépasse la limite (faute de segmentation).

---

## Q12–Q13 — Passage vers ring 3 (userland)

Plan et réponses à présenter :

- Q12 : ajouter deux descripteurs ring3 (code/data) et expliquer les champs (DPL=3).
- Q13 : expérimenter le chargement des sélecteurs et les conséquences : DS/ES/FS/GS acceptent des sélecteurs DPL=3, SS a des contraintes particulières (SS doit être de type data et RPL/CPL compatibles), et le changement de CS nécessite un far jump/jmp far ou iret ; on doit utiliser LJMP pour modifier CS.
- Expliquer aussi la méthode alternative : utiliser un mécanisme de retour utilisateur via interrupt/syscall ou créer un contexte d'exécution user avec iret et une stack d'utilisateur correctement préparée.

---

## QEMU / KVM

Rappel : pour ce TP il est conseillé d'utiliser KVM (modifier `utils/config.mk` si nécessaire).

---

Si vous voulez, je peux :

- Compléter les sections Q2…Q13 en produisant le code exact à ajouter dans `tp.c` et en testant la compilation.
- Exécuter la VM (QEMU/KVM) et capturer les logs pour que vous puissiez voir les sorties de debug.

Indiquez si je dois remplir toutes les réponses textuelles et/ou ajouter les modifications de code et tests.
