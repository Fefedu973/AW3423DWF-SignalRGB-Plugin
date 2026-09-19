# Outils retrouvés et méthode reproductible

[Navigation](README.md) · [Protocoles](protocoles.md) · [Sources](sources.md)

## Fonds historiques conservés localement

| Emplacement d’origine | Rôle dans la recherche |
| --- | --- |
| `archive locale : AW3423DWF-SignalRGB-Plugin` | Plugins, scripts Python/JS, `converttext.py`, anciennes versions VS Code `.history` 2023–2024. |
| `archive locale : KeygenTester` | Sources Qt/C++ et exécutable **Alienware Keygen Tester**, février 2025. |
| `archive locale : awcrack` | `liste.xlsx`, `liste (version 1).xlsx`, dumps working/non working, DLL, scripts et bases IDA. |
| `archive locale : alienwarergb` | Projet C# AlienFX v 2.2 décompilé, DLL native et bases de désassemblage. |
| `archive locale : alien` | Autre copie de `USBtoI2C_Vid_187c_Pid_100e.dll` et base `.i64`. |
| `archive locale : test` et `old test` | Captures, exports JSON/CSV/TXT, anciens plugins et versions OpenRGB/AlienFX Tools. |
| `archive locale : OpenRGB-aw3423dwf` | Ancienne adaptation OpenRGB. |
| `archive locale : OpenRGB` | Autre clone avec contrôleur DWF. |

Les originaux n’ont pas été déplacés ou corrigés. Le manifeste de récupération locale associe les copies essentielles à leurs sources et hashes. Les deux Excel ont été extraits avec leurs adresses de cellules ; la nouvelle matrice ne transforme pas une cellule historique non vérifiée en compatibilité matérielle attestée.

## Outils d’époque : fonctions et limites

| Outil/fichier | Ce qu’il fait réellement | Limite retrouvée |
| --- | --- | --- |
| Wireshark + USBPcap | Enregistre et inspecte les échanges USB sous Windows | Une capture filtrée sans descripteur ne prouve pas l’identité du périphérique. |
| dotPeek / dnSpy | Décompile les bibliothèques .NET et permet de suivre les appels | Les DLL natives nécessitent une autre analyse ; noms obfusqués et code décompilé ne sont pas un projet source original. |
| IDA et bases `.i64` | Analyse native de `USBtoI2C`, routines de handshake | Adresses et symboles dépendent du hash/version de la DLL. |
| Débogueur Visual Studio | Logs en temps réel de `AWCC.background.server` | Les libellés de logs peuvent changer ; ne pas confondre logs de calcul et acceptation matérielle. |
| `test.py` | Squelette de parcours des clés | Fonctions d’I/O et d’authentification avec `pass` : ne communique pas réellement. |
| `test2.py` | Expériences de réduction et génération, clé réduite explicitement écrite | Mélange de tentatives et de valeurs de test ; ne suffit pas comme référence finale. |
| `test3.py` | Comparateur hors ligne avec tables DWF/DM | Formatage hexadécimal sans remplissage zéro pouvant fausser des comparaisons textuelles. |
| `function gen_key(in_buffer, oem_key_idx).js` | Contient des versions du keygen | Une ancienne branche conserve le défaut de priorité `% 2`, contrairement à `gen_key_new`. |
| `KeygenTester/mainwindow.cpp` | GUI de comparaison challenge/réponse avec clés OEM | `extractInputKey` enlève les zéros finaux ; `extractResultKey` ignore les octets zéro. Des données légitimes peuvent donc être déformées. |
| `KeygenTester/usbparser.cpp` | Prototype d’un parseur | Identité fictive `187C:052C`, extraction naïve et chaîne factice si rien n’est trouvé : aucune valeur comme preuve USB. |

Le projet Qt contient un CMake ciblant C++17 et Qt 5/Qt 6 Widgets, ainsi qu’un fichier `.pro`. L’exécutable retrouvé est dans `build\Desktop_Qt_5_15_2_MSVC2019_64bit-Debug\KeygenTester.exe`. Il a été archivé, sans exécution pour la validation 2026. Son cœur de calcul est utile à l’histoire ; l’extraction des données doit être corrigée avant toute confiance dans ses résultats.

## Refaire une capture exploitable

La démarche prolonge le tutoriel Discord du 29 septembre 2024 (notes de recherche locales) et la [documentation USBPcap](https://desowin.org/usbpcap/tour.html).

1. Identifier précisément le modèle, le firmware, la version d’AWCC et le contrôleur USB auquel le câble upstream est relié. Le câble vidéo seul ne renseigne pas l’interface HID.
2. Démarrer USBPcap **avant** AWCC et avant sa phase d’ouverture du moniteur. Pour analyser une authentification, conserver aussi le démarrage du service concerné et les réponses IN.
3. Relever l’identité avec le `GET_DESCRIPTOR` de type DEVICE. Bus et adresse USB changent après reconnexion ; ne pas recopier l’adresse d’une capture ancienne.
4. Appliquer une séquence notée avec horaires : global rouge, vert, bleu, puis chaque zone séparément ; ajouter des couleurs non primaires, du noir et une luminosité différente. Noter aussi les effets matériels si c’est l’objet du test.
5. Arrêter proprement la capture. USBPcapCMD peut verrouiller le fichier tant qu’il enregistre ; un fichier partiel n’est pas une preuve d’absence de paquets.
6. Conserver le fichier complet original puis une copie filtrée au périphérique. Par exemple `usb.bus_id == 1 && usb.device_address == 22` est valable uniquement si le descripteur de cette capture identifie bien l’appareil à cette adresse.
7. Conserver SHA256, journal d’actions, version du script émetteur et résultats de l’analyse. Pour vérifier plusieurs moniteurs identiques, les distinguer par chemin USB/numéro de série réellement disponible plutôt que par leur seul PID.

Le filtrage historique `usb.src == "1.27.0" or usb.dst == "1.27.0"` est un exemple daté. Exclure les réponses `GET_REPORT` empêche de recalculer un challenge/réponse ; exclure les descripteurs empêche de prouver VID/PID.

## Décompiler utilement

Les répertoires AWCC retrouvés sont `archive locale : Dell.Gaming.Display` et son sous-répertoire `ExternalLibrary`. L’ancien pilote était dans `archive locale : Alienware Gaming Monitor AlienFX Driver`.

Copier les DLL d’une version identifiée et relever leurs hashes avant analyse. Dans .NET, suivre les métadonnées du modèle vers la factory HID, la classe de communication, le paramètre d’authentification, la table des zones et la construction des commandes. L’absence d’un symbole attendu ne suffit pas à conclure à un changement de protocole : la logique s’est déplacée des DLL par modèle vers `FxDisplayCommon.dll` entre les versions historiques consultées.

Pour le natif, partir des exports réellement invoqués (`HID_handshakeauth2`, écritures I²C) et relier les buffers aux offsets de la capture. Les noms `sub_180469620` et `byte_180F36EB0` sont des repères du travail 2023, pas des adresses à appliquer à un autre DLL. La décompilation aide à formuler un test ; la capture et l’observation du matériel ferment la boucle.

## Analyse hors ligne de la suite

`outils/analyze_alienware_pcap.py` n’ouvre aucun périphérique et ne charge pas de DLL. Il lit PCAP/PCAPNG de type USBPcap 249, reconstitue les chaînes de contrôle, extrait VID/PID des descripteurs disponibles, recalcule les réponses OEM et les checksums DDC. Son usage est :

```powershell
# Depuis la racine de la suite, avec Python 3.11 ou supérieur
python .\outils\analyze_alienware_pcap.py capture.pcapng --output analyse.json
```

Pour une capture externe :

```powershell
python .\outils\analyze_alienware_pcap.py capture.pcapng --output .\validation\resultat.json
```

Le paramètre optionnel `--keys` accepte un JSON de forme `[{"label":"identifiant","hex":"f5 3f ..."}]` et ajoute ces candidats à ceux de l’analyseur. Un candidat doit garder sa provenance. Le code public fournit seulement les cinq constantes utilisées par les plugins, et non le catalogue historique complet de 14 valeurs. Une capture ancienne utilisant un autre candidat peut donc avoir moins de correspondances avec cette configuration ; les résultats historiques mentionnent le corpus et le catalogue réellement utilisés à l'époque. Les captures brutes restent locales : fournir son propre fichier à l'analyseur.

Pour contrôler les résultats, examiner les identités, `authentication_chains`, `matching_oem_keys`, `ddc_commands`, les longueurs, `ack_status` et `warnings`. Un compteur d’authentification égal à zéro est normal pour la capture AW3426DW de référence. Une identité `null` signale qu’aucun descripteur exploitable ne la prouve. Un linktype différent de 249 est compté mais non décodé.

## Les jeux de référence

| Jeu | Usage | Résultat vérifié |
| --- | --- | --- |
| `monitor aw3423dwf.pcapng`, complet, archives `random\test` | Handshake et DDC du DWF | 57 réponses/57 ; 54 checksums/54, dont 38 commandes couleur. |
| `AW2724DM.pcapng` | Autre table OEM, autre modèle | 51 réponses/51 et 51 checksums/51. |
| `alienfx_AWFX_random_collors.pcapng` | Couleurs variées DM | 18 réponses/18 et 18 checksums/18. |
| `ALIENWARE_USBCAP_brightness0.pcapng` | Luminosité nulle et octets zéro | 27 réponses/27 et 27 checksums/27. |
| `aw3426dw-awcc.pcap` | Référence constructeur actuelle sans handshake | 18/18 DDC et ACK ; 16 fixes, 2 Pulse. |
| `signalrgb-initial.pcap` | Transport du premier correctif réel | 45/45 commandes valides et acquittées. |
| `signalrgb-colors.pcap` | Couleurs et adressage indépendant | 269/269 commandes valides et acquittées. |

Les nouveaux résultats de performance sont séparés des premiers tests fonctionnels : voir [Performances](performances.md). Les captures historiques de Microchip sont conservées, mais le parseur Realtek n’est pas une validation de toutes les commandes de cette autre famille.

## Ajouter une preuve sans perdre l’historique

Une nouvelle expérience doit fournir le modèle exact, VID/PID et origine de cette identité, firmware, versions logicielles, endpoint, scénario, résultat visuel, fichier capturé et SHA256. Conserver le journal des échecs et les hypothèses abandonnées. Pour déclarer une extension complète, tester aussi reconnexion, veille/reprise, extinction, démarrage avec plusieurs moniteurs et utilisation prolongée. Ces cas ne sont pas implicitement couverts par une minute de couleurs réussies.
