# Répertoire de ressources pour les tests

Ce répertoire contient les images nécessaires pour les tests automatisés.

Structure attendue pour chaque cas de test :

```
resources/
  ├── test_case_1/
  │    ├── cropped_sheet.jpg     # Image de la feuille à analyser
  │    ├── expected_impacts.jpg  # Masque attendu des impacts
  │    └── expected_visuals.jpg  # Masque attendu des cibles visuelles
  ├── test_case_2/
  │    ├── ...
  ...
```

Chaque dossier de test doit contenir les fichiers suivants :
- `cropped_sheet.jpg` : L'image d'entrée de la feuille de cible à analyser
- `expected_impacts.jpg` : Image en noir et blanc où les zones blanches représentent les impacts attendus
- `expected_visuals.jpg` : Image en noir et blanc où les zones blanches représentent les cibles visuelles attendues

Les dossiers contenant "WIP" dans leur nom ou le dossier "TODO" seront ignorés par les tests automatiques.
