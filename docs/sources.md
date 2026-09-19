# Sources et périmètre public

[Navigation](README.md) · [Historique](historique.md) · [Attributions](../NOTICE.md)

## Dépôts et contributions publiques

| Source primaire | Repère | Utilisation |
| --- | --- | --- |
| [Dépôt original](https://github.com/Fefedu973/AW3423DWF-SignalRGB-Plugin) | Base conservée `ee524b53856991d3c31718981e42a079c910f4a9` | Plugins historiques et deux premières constantes OEM |
| [Premier commit](https://github.com/Fefedu973/AW3423DWF-SignalRGB-Plugin/commit/c49ddc8) | 26 août 2023 | Première publication retrouvée |
| [Version fonctionnelle](https://github.com/Fefedu973/AW3423DWF-SignalRGB-Plugin/commit/d793cc3) | 1 janvier 2024 | Progression du keygen et du plugin |
| [OpenRGB MR 2688](https://gitlab.com/CalcProgrammer1/OpenRGB/-/merge_requests/2688) | Ouverte 26 janvier 2025, fusionnée 3 mai 2025 | Contribution AW3423DWF de Fefedu973 |
| [OpenRGB MR 2683](https://gitlab.com/CalcProgrammer1/OpenRGB/-/merge_requests/2683) | 23 janvier 2025, fermée | Essai antérieur |
| [SignalRGB MR 1146](https://gitlab.com/signalrgb/signal-plugins/-/merge_requests/1146) | HarDBR, mai–juin 2025 | Regroupement Gen1/Gen2/Gen3 |
| [Sources SignalRGB Alienware](https://gitlab.com/signalrgb/signal-plugins/-/tree/Development/Plugins/Alienware) | Sources consultées 19 septembre 2026 | Familles, profils et initialisations historiques |
| [Base OpenRGB](https://gitlab.com/CalcProgrammer1/OpenRGB/-/commit/cd44e41d462629410a806119694a773e88449ce1) | 18 septembre 2026 | Base exacte du patch livré |
| [AlienFX Tools](https://github.com/T-Troll/alienfx-tools) | Copie consultée `52713b2` | Nomenclature historique V6/V9 |
| [Dell, FAQ moniteurs](https://www.dell.com/support/kbdoc/en-au/000131796/dell-gaming-monitor-frequently-asked-questions) | Version consultée 9 juillet 2026 | Catalogue, pas une preuve de protocole RGB |

Les manuels Dell propres aux modèles figurent dans `models.json` et [Support](support.md). Les dates de recherche sont fixes ; les branches et pages publiques peuvent évoluer.

## Métadonnées et constantes du pilote

AWCC/FxDisplayCommon 6.14.24.0 et les métadonnées FxMetaData ont fourni les identités, zones et branches logicielles `MicroChip5734`/`RTS5409S`. Le booléen d’authentification est distinct du choix de transport. Les documents résument ces résultats ; aucun binaire Dell ni fichier source décompilé n’est redistribué.

Le code conserve cinq constantes OEM génériques nécessaires au chemin d’authentification prévu. Les deux premières sont déjà présentes dans les plugins historiques publics DWF/DM. Les trois autres ont été relevées dans le pilote AWCC actuel. Il ne s’agit pas de secrets de compte, de tokens cloud ou de clés liées à un utilisateur. Le catalogue historique complet de 14 valeurs reste local. L’AW3426DW testé n’active pas cette authentification ; les autres profils issus du pilote restent à valider individuellement sur matériel.

## Preuves publiées

- `signalrgb/tests/fixtures.json` : 146 challenges/réponses et 16 rapports couleur AWCC, sans descripteurs ou identifiants d’utilisateur.
- `openrgb/files/tests/alienware-monitor/fixtures.json` : 44 rapports couleur et 101 paires challenge/réponse, avec numéros de trame et hashes de source.
- `validation/` : résumés de tests, mesures USB historiques et changement de cadence. Les timestamps et fréquences décrivent des essais, pas une mesure optique.
- `models.json` : sources, cellules des tableaux historiques et niveaux de preuve. Les chemins locaux sont remplacés par des noms de source `local-source:` ; cela n’implique pas que le fichier privé soit téléchargeable.
- `Alienware-RGB-Moniteurs.xlsx` : valeurs et mise en forme préservées ; 26 liens vers des fichiers privés remplacés par cette page de provenance.

## Archives locales exclues

Les conversations/DM Discord bruts, captures d’écran privées, URL de pièces jointes, bibliothèques propriétaires, décompilations et captures USB complètes ne font pas partie de ce dépôt. La chronologie ne publie qu’une synthèse technique ; ses notes privées restent distinguées des commits et MR accessibles.

Le clone local de préparation conserve l’intégralité de la suite originale dans `archives-local/alienware-rgb-suite-private/`, y compris son archive historique. `.gitignore` exclut ce dossier et les types de fichiers sensibles au périmètre de publication. Aucun historique Git existant n’a été réécrit.
