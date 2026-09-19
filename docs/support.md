# Support des modèles et niveaux de preuve

[Navigation](README.md) · [Matrice JSON](../models.json) · [Matrice CSV](../models.csv) · [Classeur](../Alienware-RGB-Moniteurs.xlsx)

La matrice est un inventaire de **42 modèles**, et non une liste de moniteurs tous testés. Elle réunit les deux tableaux historiques, les métadonnées d’AWCC installé, les contrôleurs publics et les vérifications Dell. Les champs inconnus restent vides ou `null`. Les sources et les lignes d’origine sont conservées dans chaque fiche JSON. La suite implémente **23 profils USB uniques** : trois anciens `92 48`, douze Microchip et huit Realtek, dont quatre avec authentification.

## Lire un statut

| Statut de la matrice | Ce qu’il établit | Ce qu’il ne prouve pas |
| --- | --- | --- |
| `signalrgb_hardware_tested` | Commandes SignalRGB capturées et comportement physique confirmé pendant cette session | Fonctionnement prolongé, veille/reprise et tous les firmwares |
| `historical_capture_validated` | Un corpus historique identifie le périphérique et reproduit ses calculs | Revalidation visuelle actuelle ou test du nouveau plugin sur cet appareil |
| `awcc_declared_not_hardware_tested` | Métadonnées de zones et chemin logiciel trouvés dans AWCC | Compatibilité matérielle complète de SignalRGB/OpenRGB |
| `upstream_declared_not_hardware_tested` | Profil présent dans une implémentation publique | Vérification USB et visuelle dans cette session |
| `rgb_confirmed_protocol_unknown` | Éclairage explicitement documenté, mais protocole/identité insuffisants | Possibilité d’utiliser un PID emprunté à un modèle voisin |
| `not_rgb_supported` | Pas de prise en charge d’éclairage dans les preuves indiquées | Absence absolue de LED physiques ; vérifier `rgb_status` et sa source |
| Autre statut inconnu ou catalogue | Existence du modèle documentée | Présence de RGB, identité USB ou compatibilité logicielle |

`auth=true` signifie qu’AWCC active le challenge/réponse OEM pour ce profil ; `false` signifie qu’il ne l’active pas, ou que la famille documentée n’utilise pas ce mécanisme. Ce n’est pas une affirmation sur la sécurité ou les intentions du constructeur. `controller` est d’abord le nom d’une classe logicielle ; aucune puce n’a été identifiée physiquement par démontage pendant cette session.

## Familles à ne pas fusionner

| Famille | Identités attestées dans les sources utilisées | Authentification E1 | Niveau général |
| --- | --- | --- | --- |
| Ancien format `92 48` | `0424:274A` AW3418DW, `274B` AW3418HW, `274C` AW2518H | Non dans le code public | Profils SignalRGB publics, sans test matériel actuel |
| Microchip HID/I²C/DDC | Plusieurs PID `187C` et `0424`, détails dans la matrice | Pas de handshake Realtek dans ce chemin | Métadonnées AWCC et sources publiques ; quelques témoignages et captures historiques |
| Realtek avec handshake | `187C:100E` DWF, `1010` DM ; `1019` AW2725Q et `101A` AW3425DW dans AWCC | Oui | DWF/DM vérifiés hors ligne ; deux autres déclarés par le pilote |
| Realtek sans handshake | `187C:100F` AW2524H, `101D` AW3426DW, `1020` AW3226Q, `1021` AW3926QW | Non dans AWCC actuel | AW3426DW testé matériellement ; autres profils déclarés |

Le nom AW3423DW et le nom AW3423DWF ne désignent pas le même transport. AW2725D, AW2725DF et AW2725Q sont également des modèles distincts. Une suffixe proche, le même nombre de zones ou la présence d’AWCC ne suffisent pas à copier un PID.

## Périmètre exact du test AW3426DW

Le test du 19 septembre 2026 identifie `187C:101D`, interface 0, usage `00DA`, usage page `FFDA`. Le profil du pilote expose deux zones, logo et alimentation, masques `01` et `08`. AWCC et SignalRGB émettent les mêmes blocs DDC de couleur fixe, avec le masque commun `09` lorsque les couleurs sont identiques. Les couleurs séparées ont aussi été capturées et l’utilisateur a confirmé les deux zones.

Le crash d’origine était un accès à `.name` sur le profil absent de la table interne. Ajouter le PID à la seule liste de détection n’ajoutait ni nom, ni zones, ni choix d’authentification. Cette distinction explique pourquoi une détection USB réussie pouvait être suivie d’une initialisation impossible.

La suite adapte [SignalRGB](../signalrgb/README.md) et fournit le travail d’intégration [OpenRGB](../openrgb/README.md). Les tests SignalRGB comprennent 34 vérifications, avec 146 fixtures d’authentification et 16 paquets AWCC ; le test physique concerne AW3426DW. Les tests du patch OpenRGB vérifient 44 rapports complets et 101 réponses de challenge, plus des écritures/lectures partielles et l’isolation entre périphériques simulés. Le contrôleur, le wrapper RGB et le détecteur ont été compilés contre les en-têtes amont ; **ni l’application Qt complète ni un test matériel OpenRGB n’ont été exécutés** pendant cette session. Le détail et les commandes reproductibles se trouvent dans ces dossiers.

L’existence de code pour un modèle ne lui donne pas automatiquement le statut « testé matériellement ». Les sources AWCC sont un point de départ vérifiable pour les profils restants.

## Complément de catalogue vérifié en septembre 2026

Les manuels officiels [AW2525HM](https://dl.dell.com/content/manual25876081-alienware-25-320hz-gaming-monitor-aw2525hm-user-s-guide.pdf?language=en-us), [AW2725DM](https://dl.dell.com/content/manual52095525-alienware-27-gaming-monitor-aw2725dm-user-s-guide.pdf?language=en-us), [AW3225DM](https://dl.dell.com/content/manual55643468-alienware-32-gaming-monitor-aw3225dm-user-s-guide.pdf?language=en-us) et [AW3425DWM](https://dl.dell.com/content/manual56823731-alienware-34-gaming-monitor-aw3425dwm-user-s-guide.pdf?language=en-us) ont une section AlienFX/AWCC. Celle-ci décrit notamment GAME, Vision et Settings. Les passages consultés ne fournissent pas une carte de zones RGB exploitable : ces noms restent dans l’inventaire avec le degré d’incertitude approprié. La phrase générique sur les effets d’éclairage ne suffit pas à confirmer une fonction propre à chaque modèle.

La [FAQ officielle Dell, version du 9 juillet 2026](https://www.dell.com/support/kbdoc/en-au/000131796/dell-gaming-monitor-frequently-asked-questions), confirme aussi les références AW2526HL, AW2726DL, AW2726DM, AW3226DM, AW3426DWM et AW3926QW. Elle confirme des noms de produits, pas leurs PID ni leurs zones. Les informations AWCC plus précises, lorsqu’elles existent, restent prioritaires dans la matrice.

L’[annonce Dell du 2 septembre 2026](https://www.dell.com/ja-jp/blog/alienware-4k-560hz/) décrit AW3226Q et AW2527HX, ce dernier annoncé pour début 2027. L’annonce seule ne documente pas de protocole de LED. La mention « RGB stripe » d’une dalle OLED décrit les sous-pixels de l’image et ne doit surtout pas être interprétée comme un éclairage décoratif AlienFX.

## Comment promouvoir un profil vers « testé »

Ajouter une capture identifiée, le hash du plugin testé, la version du firmware, les zones effectivement observées et le résultat visuel. Conserver les échecs, en particulier le démarrage avec plusieurs appareils, la reconnexion et la veille. Pour un modèle authentifié, tester les deux branches du keygen à partir des challenges observés. Pour les performances, publier les cadences USB séparément des observations lumineuses.
