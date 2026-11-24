# Clôture définitive du développement — 24 novembre 2026

**Décision du mainteneur :** « Je m'avoue vaincu face à Microsoft et
Rockstar Games. » Je cesse les travaux expérimentaux sur la compatibilité
des jeux commerciaux qui avaient motivé Scarlett Fusion Lab.

Cette phrase représente ma propre décision et ne signifie pas que Microsoft,
Rockstar Games ou leurs partenaires ont pris part au projet ou ont formulé
une réclamation.

## Bilan technique et limites

- Les tests GPU et mémoire ont servi à mieux comprendre Direct3D 12,
  les budgets vidéo et les différences entre architectures graphiques.
- Certains tests synthétiques ont fonctionné, mais ces résultats ne
  démontrent pas le fonctionnement d'un titre commercial.
- **Ni monde jouable, ni commandes du personnage n'ont été validés.**
- L'expérience s'arrête volontairement à cette étape.

## Retrait préventif de contenus

La version publique a été réduite aux diagnostics matériels originaux.
Les scripts d'exécution et d'adaptation des jeux/runtimes,
les guides ciblant un titre commercial et les traces détaillées
ont été retirés de l'arborescence active.

Les fichiers d'origine présents uniquement sur une machine personnelle
ne sont pas inclus dans le dépôt et **ne sont pas touchés** par ce nettoyage.
Les binaires produits par compilation sont exclus par `.gitignore`.

Ce contrôle porte sur les fichiers et références GitHub accessibles à
la date de la clôture. Les anciennes révisions, clones, caches, issues
et copies externes peuvent subsister et nécessiter une intervention
distincte si du contenu doit en être retiré.

## Suite

Mon travail personnel reprend sur **Tactical Contested Extraction (TCE)**.
Ce dépôt reste un témoin minimal de ce qui a été appris.

**Aucun développement ou support supplémentaire n'est planifié ici.**
