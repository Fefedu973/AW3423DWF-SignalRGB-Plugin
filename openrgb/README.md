# Mise à jour OpenRGB — écrans Alienware

Le patch ajoute **23 profils USB distincts** et regroupe les pilotes AW3423DWF et AW3225QF dans un contrôleur commun. Il corrige le checksum des couleurs Realtek, conserve l’authentification sur les modèles concernés et ajoute l’AW3426DW (`187C:101D`, logo `01`, bouton `08`, ensemble `09`).

Base : [OpenRGB officiel, commit cd 44 e 41 d](https://gitlab.com/CalcProgrammer1/OpenRGB/-/commit/cd44e41d462629410a806119694a773e88449ce1), daté du 18 septembre 2026. Les sources et leur historique amont sont conservés ; cette livraison est un patch à appliquer au commit indiqué.

## Contenu et portée

| Famille | Profils | Transport | Authentification OEM |
| --- | --- | --- | --- |
| Classique | AW2518H, AW3418DW, AW3418HW | HID 65 octets, commande `92 48` ; 64 octets USB | Non |
| Microchip | AW2521H/HF/HFL, AW2720HF, AW2721D, AW2723DF, AW2725DF, AW3225QF, AW3420DW, AW3423DW, AW3821DW, AW5520QF | HID 65 octets, commande `92 37`, DDC ; 64 octets USB | Non |
| Realtek avec authentification | AW3423DWF, AW2724DM, AW2725Q, AW3425DW | HID 193 octets, commandes `40 E1` puis `40 C6` ; 192 octets USB | Conservée |
| Realtek sans authentification | AW2524H, AW3426DW, AW3226Q, AW3926QW | HID 193 octets, commande `40 C6` ; 192 octets USB | Désactivée uniquement selon le profil AWCC |

Les modèles sans zones RGB déclarées, sans VID/PID certain ou sans protocole identifié restent dans la matrice générale, sans être enregistrés dans ce pilote. Les variantes HFA/HFLA ne reçoivent pas de PID inventé.

Les profils et les masques proviennent des métadonnées AWCC et de `HIDDeviceFactory`/`AnimationBase` de `FxDisplayCommon` 6.14.24.0. Le transport classique et certaines initialisations Microchip proviennent des plugins SignalRGB historiques. L’initialisation déjà présente dans OpenRGB pour l’AW3225QF est conservée exactement. Les nouveaux profils Microchip utilisent la requête F4 du plugin Gen2 ; cette requête n’est pas présentée comme une activation de mode logiciel démontrée.

Le checksum DDC XOR commence à `6E` et couvre tous les octets du corps, y compris la zone, les trois couleurs et la luminosité. Les écritures HID comprennent un Report ID zéro distinct du contenu USB. Les tailles, zones et masques collectifs sont propres au modèle. L’ancien détecteur AW3423DWF est supprimé pour éviter les doublons.

Pour les modèles authentifiés, cinq clés OEM AWCC sont conservées. L’AW2724DM essaie d’abord sa clé démontrée dans les captures ; les autres commencent par la première clé AWCC. Une négociation bornée reproduit le challenge/réponse et la requête de contrôle AWCC, avec vérification des retours HID, état par écran, verrouillage des échanges, nouvelle authentification avant chaque couleur et nouvelle sélection après une erreur. **Un transfert USB réussi ne prouve pas à lui seul qu’une clé ou une couleur a été acceptée par le moniteur.**

Intervalle minimal entre couleurs : 100 ms pour l’AW3426DW, 200 ms pour les autres Realtek, 50 ms pour les transports classiques/Microchip. Les modèles authentifiés conservent aussi une pause de 100 ms avant l’envoi de couleur. L’ancien réglage de 20 ms est historique : malgré des transferts USB réussis, l’utilisateur a ensuite signalé du retard. Le nouveau défaut de 100 ms est un choix prudent, pas encore validé optiquement. Aucune cadence OpenRGB n’a été mesurée sur le matériel.

## Installation du code

Le livrable est un **patch source**, sans exécutable OpenRGB compilé. `files/` contient les fichiers ajoutés/modifiés pour lecture ; `deleted-files.txt` énumère les anciens fichiers retirés. Utiliser le patch pour appliquer aussi les suppressions :

Depuis la racine de ce dépôt :

```powershell
git clone https://gitlab.com/CalcProgrammer1/OpenRGB.git OpenRGB
git -C OpenRGB checkout cd44e41d462629410a806119694a773e88449ce1
git -C OpenRGB apply --check '../openrgb/alienware-monitor-suite.patch'
git -C OpenRGB apply '../openrgb/alienware-monitor-suite.patch'
python OpenRGB\tests\alienware-monitor\run.py
```

Compiler ensuite OpenRGB avec sa chaîne Qt habituelle, selon les [instructions officielles de compilation](https://gitlab.com/CalcProgrammer1/OpenRGB/-/blob/master/Documentation/Compiling.md). Avant un futur test matériel OpenRGB, fermer SignalRGB et Alienware Command Center pour qu’un seul logiciel commande l’éclairage. Aucun OpenRGB n’a été démarré sur la machine pendant cette intervention.

Pour retirer uniquement ce patch, dans un checkout qui n’a pas reçu d’autres modifications sur les mêmes lignes :

```powershell
git -C OpenRGB apply --reverse --check '../openrgb/alienware-monitor-suite.patch'
git -C OpenRGB apply --reverse '../openrgb/alienware-monitor-suite.patch'
```

## Vérifications et limites

Les résultats de la préparation du dépôt sont dans `../validation/publication-tests.json`.

- Compilation C++17 du véritable contrôleur et de ses profils, avec substitutions HID qui ne peuvent ouvrir aucun périphérique.
- Comparaison intégrale de 44 rapports couleur issus des captures et validation de 101 paires challenge/réponse DWF/DM, avec les deux branches de parité.
- Cas de lecture tronquée/erreur, écriture partielle, échec de contrôle, négociation bornée, indépendance des clés par écran, initialisations, tailles et limites des profils.
- Compilation syntaxique du détecteur et de l’adaptateur RGB avec les véritables en-têtes OpenRGB ; contrôle du diff et application du patch vérifiés contre le commit de base.

La compilation complète de l’application Qt n’a pas été réalisée : la chaîne Qt/qmake n’est pas configurée dans l’environnement utilisé. Aucun test matériel OpenRGB ni test d’exécution Linux/macOS n’est revendiqué. L’AW3426DW a été confirmé visuellement et par capture avec SignalRGB ; DWF/DM disposent de captures historiques. Les autres profils restent à valider sur leurs appareils respectifs.
