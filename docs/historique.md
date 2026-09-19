# Historique 2023–2026

[Navigation](README.md) · [Sources détaillées](sources.md)

## Août 2023 : des couleurs fonctionnent, mais pas durablement

Le premier commit du dépôt, [`c49ddc8`](https://github.com/Fefedu973/AW3423DWF-SignalRGB-Plugin/commit/c49ddc8), date du **26 août 2023 à 11:29 +02**. Un plugin AW3423DWF commence à reproduire les commandes RGB observées. La difficulté n’est pas seulement l’emplacement des couleurs : un bloc de huit octets, repris d’une capture, fonctionne temporairement puis devient invalide.

Le **27 août**, début du échanges techniques avec T-Troll, Fefedu973 partage la DLL AlienFX, son script et la fonction `FillInChecksum`. T-Troll rapproche le bloc de couleur du protocole de ses anciens moniteurs. Le **28 août**, il documente les longueurs, commandes d’effets et offsets de ce qu’il nomme V9, puis fournit plusieurs versions de test d’AlienFX CLI. La détection commence à fonctionner, mais les couleurs ne suivent pas : reconnaître le périphérique et lui envoyer un bloc plausible ne suffisent pas.

Les **28–29 août**, trois opérations se distinguent : `40 e1 01`, lecture `GET_REPORT`, puis `40 e1 02`. Le challenge retourné et la réponse envoyée changent. La capture `test6.pcapng`, demandée avec trois zones rouge/vert/bleu, sert de référence. La première hypothèse de T-Troll — commandes de configuration facultatives — est progressivement abandonnée pour les modèles protégés.

À ce stade, les mots « checksum » et « clé » se recouvrent dans la conversation. Les recherches suivantes séparent deux mécanismes : un checksum XOR sur le bloc DDC et un challenge/réponse avant son utilisation. Cette séparation évite de chercher une clé dans le dernier octet d’une commande couleur.

## Septembre–novembre 2023 : de la capture aux bibliothèques Dell

Le **9 septembre**, T-Troll demande de capturer le démarrage d’`AWCCService` depuis le gestionnaire de services. L’objectif est de conserver l’initialisation, absente d’une capture démarrée après AWCC. Le **14 octobre**, l’ordre `e101 → GET_REPORT → e102 → couleur` est explicité. L’idée d’une initialisation unique, éventuellement avec une réponse nulle, est discutée ; elle reste une piste expérimentale, pas une règle validée pour tous les moniteurs.

Les **3–5 novembre**, l’analyse suit la chaîne .NET de détection et d’ouverture : `UsbDetectionManager`, gestionnaire AW3423DWF, `HidToUsbApi.OpenDevice`, sélection `keyIndex`, puis appels à la bibliothèque native. Le DLL historique se trouve alors dans `archive locale : Alienware Gaming Monitor AlienFX Driver`. Les méthodes de lecture, d’écriture et `FillInChecksum` sont partagées dans les notes de recherche.

Les **11–14 novembre**, les échanges portent sur le code natif : `HID_handshakeauth2`, `HID_handshakeauthforce`, `sub_180469620` et la zone `byte_180F36EB0`. Le découpage devient : décoder la table OEM en une clé synthétique de huit octets, transformer les seize octets reçus, puis XORer les deux. Les adresses sont celles d’un binaire historique particulier ; elles ne sont pas des identifiants stables de fonctions pour une DLL actuelle.

Une autre découverte pratique complète la décompilation : démarrer `AWCC.background.server` dans le débogueur Visual Studio expose les logs de calcul. Le libellé `RevertDellSynKey SYNKEY2A` sert de contrôle croisé. Les premiers scripts de novembre contiennent encore des interprétations incomplètes ; leur conservation sert à comprendre le parcours, pas à en faire des implémentations de référence.

## Décembre 2023–janvier 2024 : le calcul devient reproductible

Le **28 décembre 2023**, Fefedu973 partage une version Python du keygen, utilisant les tables OEM, la parité et l’index dans le challenge. Le fil évoque sa vérification sur un autre exemplaire du même modèle. Le **1 er janvier 2024**, un plugin fonctionnel est partagé dans Discord, puis le commit [`d793cc3`](https://github.com/Fefedu973/AW3423DWF-SignalRGB-Plugin/commit/d793cc3) est intitulé « The plugin now works ! ».

Cette étape résout l’obstacle pratique du challenge/réponse. Elle ne signifie pas que chaque détail du code est final : les archives contiennent encore un calcul de parité JavaScript sans les parenthèses nécessaires et des checksums couleur fixes. Les captures de 2026 permettent de distinguer le résultat fonctionnel observé à l’époque des défauts de généralisation encore présents dans certaines versions.

## Septembre–octobre 2024 : une recherche collective par modèle

Le serveur **Alienware Monitor RGB Crack HUB** ouvre le **28 septembre 2024**. Le salon DLL (notes de recherche locales) centralise les bibliothèques de modèles et les DLL USB/I²C. Le tutoriel (notes de recherche locales) organise les captures : démarrer avant AWCC, changer les couleurs globalement puis par zone, identifier le moniteur et exporter les paquets affichés.

Pour l’AW2724DM (notes de recherche locales), un contributeur fournit une capture le **29 septembre**, un autre contributeur en fournit une autre le **4 octobre**. Ces deux références ont été récupérées. Le **9 octobre**, Fefedu973 explique que Wireshark documente le comportement, mais que le travail principal de cette phase consiste à retrouver la logique dans les DLL et leur désassemblage.

Les statuts du salon de suivi sont des déclarations datées. La dernière édition visible, **9 février 2025**, marque notamment DWF, DW, AW2723DF et AW3225QF fonctionnels, AW2724DM décodé avec intégration en développement, et d’autres modèles encore en attente. Les appellations et erreurs de saisie originales doivent rester distinguées des identités USB vérifiées.

## Janvier–février 2025 : OpenRGB, testeur et bibliothèque commune

Le **23 janvier 2025**, les notes de recherche confirme l’intégration OpenRGB après correction du type de lecture HID. Le [commit partagé](https://gitlab.com/Fefedu973/OpenRGB/-/commit/38780c9d796fbc334a213feb4ab4a444610a991d) et la MR !2683 précèdent la [MR !2688](https://gitlab.com/CalcProgrammer1/OpenRGB/-/merge_requests/2688), ouverte le **26 janvier** pour l’AW3423DWF seul.

Le **2 février**, Fefedu973 montre dans le salon AW2724DM son **Alienware Keygen Tester** : deux dumps hexadécimaux, une table OEM, un index et un mode d’essai de toutes les clés. Il prépare une version OpenRGB adaptée. Les sources Qt/C++ ont été retrouvées localement. Leur calcul central est exploitable, tandis que leur extraction des zéros et un parseur USB de démonstration demandent des corrections ; [les limites sont documentées](outils-et-methode.md).

Le **9 février**, les plugins AW2723DF, AW3225QF, DWF et DM arrivent dans le dépôt. Les fichiers DWF/DM publiés sur Discord sont identiques par SHA256 aux copies GitHub. Les deux tableaux Excel datent du **8 et du 15 février** selon les métadonnées retrouvées et classent les protocoles « V1 », « V2 » et « V2,5 ».

Le même **9 février à 17:55**, `FxDisplayCommon.dll` est partagé avec T-Troll. La bibliothèque commune expose deux familles : `MicroChip5734` et `RTS5409S`. À **18:22**, le testeur affiche un résultat positif pour la clé OEM courte `f5 3f c1 39 44 3a 31 79 0d b1 82 76`. Le screenshot a été récupéré. C’est la preuve historique de ce vecteur précis, pas une preuve que chaque modèle accepte cette clé.

Le **5 mars**, un contributeur rapporte que deux AW2724DM présents dès le démarrage ne sont pas initialisés correctement, alors qu’un branchement du second après le premier fonctionne. Le signalement est une limite distincte du keygen, restée sans diagnostic définitif dans le salon lu.

## Mai–juin 2025 : intégrations et limites de débit

La MR personnelle [OpenRGB !2688](https://gitlab.com/CalcProgrammer1/OpenRGB/-/merge_requests/2688) est fusionnée le **3 mai 2025 à 22:59 UTC**. Le **4 mai**, CalcProgrammer 1 travaille dans le général à un contrôleur pour l’ancienne famille de moniteurs. Il compare les paquets et les temporisations AW3225QF : après correction du remplissage `ff`, les envois trop rapprochés restent problématiques. Ses essais trouvent 100 ms puis 50 ms fonctionnels, 35 ms non fonctionnels.

Le **9 mai**, il indique avoir remplacé le réglage explicite de couleur globale par un regroupement automatique lorsque toutes les zones ont le même RGB. Le **26 mai**, Fefedu973 précise que 50 ms n’est pas fiable pour son DWF. Les seuils ne se généralisent donc pas d’un modèle à l’autre.

La [MR SignalRGB !1146](https://gitlab.com/signalrgb/signal-plugins/-/merge_requests/1146), **HarDBR**, est créée le **25 mai** puis fusionnée le **5 juin** dans `Development`. Elle regroupe les contrôleurs Gen1/Gen2/Gen3. Cette contribution est distincte de la MR OpenRGB de Fefedu973 et s’appuie sur le travail disponible sur plusieurs modèles.

## Septembre 2026 : AW3426DW et consolidation des preuves

Le **19 septembre**, les logs de l’AW3426DW montrent `187C:101D` et un crash `Cannot read property 'name' of undefined`. La table des modèles du plugin Gen3 ne contient pas ce PID, bien qu’il ait été ajouté à sa liste de détection. Le crash survient avant tout handshake.

Le pilote AWCC **6.14.24.0** choisit pour ce PID un `RTS5409S` avec `authHandshhake=false`. Une capture complète confirme 18 commandes DDC, dont 16 couleurs fixes et 2 Pulse, aucun échange `40 e1`. Le modèle n’a que deux zones : logo `01`, bouton `08`, masque commun `09`.

Le correctif ajoute le profil, le bon checksum et la convention de rapport HID de 193 octets pour 192 octets USB. La première capture SignalRGB contient 45 commandes correctes ; une seconde 269, avec RGB global et deux paires de couleurs individuelles. Les ACK et les octets concordent avec AWCC. L’utilisateur confirme que les deux zones suivent SignalRGB.

La relecture des archives vérifie indépendamment **153 réponses sur 153** et **150 checksums DDC sur 150**. Parmi ces réponses, 50 utilisent la clé dite universelle et les captures DM en utilisent une autre. La consolidation de la suite conserve les tableaux, les sources, les traces et leurs hashes.

Le travail de fin de session rassemble **42 modèles documentés et 23 profils USB implémentés**. SignalRGB est réparti en trois plugins et OpenRGB reçoit un patch avec sources et tests. Un ordonnanceur sans pause bloquante remplace l’attente systématique du plugin. Dans l’essai historique avec un seuil de 20 ms, 439/439 commandes sont valides ; la cadence USB atteint environ 21,78 commandes/s globales ou 10,8 par zone. Après une première appréciation positive, l’utilisateur signale du retard pendant l’utilisation. Le 19 septembre à 18:25, la recommandation AW3426DW passe à **100 ms**, également reportée dans OpenRGB. La validation visuelle de ce nouveau réglage reste en attente. Ces essais et leurs limites sont détaillés dans [Performances](performances.md). Les autres moniteurs de la matrice ne sont pas déclarés testés physiquement à partir du seul résultat AW3426DW.

## Hypothèses remplacées par des preuves

| Hypothèse ou raccourci historique | État établi ensuite |
| --- | --- |
| `e101/e102` ne sont que de la configuration facultative | Handshake identifié pour certains Realtek ; option explicitement désactivée pour AW3426DW. |
| Rejouer une ancienne réponse suffit durablement | La réponse dépend du challenge. Reproduction déterministe par keygen nécessaire lorsqu’un handshake est demandé. |
| Le dernier octet couleur représente une clé | C’est le checksum DDC ; la réponse d’authentification est un autre bloc de huit octets. |
| Le changement de modèle implique une nouvelle clé | AW3426DW réutilise l’enveloppe mais n’authentifie pas ; les zones et le profil changent. |
| La famille de puce permet de déduire l’authentification | Les deux sont des axes distincts dans la factory AWCC actuelle. |
| « Universelle » signifie prouvée sur toute la gamme | Le corpus prouve des vecteurs et des modèles précis ; d’autres clés sont effectivement observées. |
| Une couleur ou un ACK réussi prouve un débit d’animation | Il faut mesurer les intervalles, les zones et la réaction physique sur une séquence contrôlée. |
