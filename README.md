# Alienware Monitor RGB

Recherche, plugins **SignalRGB** et patch **OpenRGB** pour l’éclairage des moniteurs Alienware. Ce dépôt prolonge le plugin AW3423DWF publié par Fefedu973 depuis 2023, sans supprimer ses anciennes versions ni son historique Git.

## Prise en charge

- **AW3426DW : VID 187C, PID 101D**, logo et bouton d’alimentation, contrôle SignalRGB testé par capture USB et observation physique le 19 septembre 2026. Ce modèle utilise les commandes Realtek sans handshake OEM.
- **23 profils USB implémentés** : 3 anciens `92 48`, 12 Microchip `92 37`, 8 Realtek `40 C6`, dont 4 authentifiés. Les profils autres que l’AW3426DW ne sont pas tous testés physiquement avec cette version. DWF et DM disposent de fixtures historiques.
- **42 modèles documentés** dans la matrice. Un modèle inventorié sans identité USB/protocole établi n’est pas déclaré pilotable.
- **OpenRGB est livré sous forme de patch source** et de tests C++ ciblés. Aucun exécutable GUI complet ni test matériel OpenRGB n’est fourni.

Le délai recommandé AW3426DW est **100 ms**. Les captures de 20 ms sont historiques : malgré des transferts réussis, un retour ultérieur signalait du retard. La validation optique du nouveau réglage 100 ms reste en attente. Un ACK USB ne mesure pas la cadence visible des LED.

## Utiliser le code

| Entrée | Contenu |
| --- | --- |
| [SignalRGB](signalrgb/README.md) | Trois plugins autonomes, installation, paramètres, 34 tests |
| [OpenRGB](openrgb/README.md) | Patch sur le commit amont indiqué, sources, compilation ciblée et tests |
| [Classeur](Alienware-RGB-Moniteurs.xlsx) | Matrice 42 modèles et deux tableaux historiques préservés |
| [JSON](models.json) / [CSV](models.csv) | Identités, zones, sources, niveaux de preuve et inconnues |
| [Documentation](docs/README.md) | Chronologie 2023–2026, protocoles, méthodes et performances |
| [Sources et publication](docs/sources.md) | Provenance, constantes de protocole et exclusions |
| [Attributions](NOTICE.md) | Origines du code et licences conservées |

Les fichiers `aw3423dwf.js` et `plugins/` sont les versions historiques, conservées telles quelles. Pour une nouvelle installation, utiliser les fichiers de `signalrgb/` et éviter les contrôleurs concurrents pour le même périphérique.

## Développement et vérification

```text
node outils/build-signalrgb.cjs
node signalrgb/tests/test-plugins.cjs
python outils/analyze_alienware_pcap.py capture.pcapng --output analysis.json
```

Le générateur ne modifie pas l’installation SignalRGB. Les tests substituent les E/S et n’ouvrent aucun matériel. Les instructions OpenRGB indiquent le checkout nécessaire et les en-têtes amont à utiliser.

Les cinq constantes OEM du code sont des constantes génériques de protocole provenant du pilote et des travaux historiques, pas des identifiants de compte. L’AW3426DW ne les utilise pas. Leur présence ne prouve pas qu’elles soient acceptées par tous les modèles.

Les archives privées, DLL, décompilations, captures USB brutes et conversations Discord restent hors du dépôt public. Dans le clone de préparation, elles sont conservées sous `archives-local/`, explicitement ignoré par Git. Les fixtures publiées ne contiennent que les paquets de protocole nécessaires aux tests.
