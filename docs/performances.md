# Débit RGB : mesures et explication

[Navigation](README.md) · [Résultat JSON du benchmark](../validation/performance.json)

Le contrôle couleur de l’AW3426DW fonctionne sans handshake. Son ancien débit faible venait principalement des pauses explicites et du rythme d’appel du plugin. Il ne faut donc pas attribuer automatiquement la lenteur à l’authentification historique.

## Réglage actuel : 100 ms, validation visuelle en attente

Après les essais courts ci-dessous, l’utilisateur a signalé des à-coups et un retard visible pendant l’utilisation. Le 19 septembre à 18:25, le réglage recommandé AW3426DW a donc été porté de **20 à 100 ms** entre commandes. SignalRGB conserve un ordonnanceur sans pause bloquante : il prélève la couleur récente uniquement lorsqu’une émission est permise, sans file de trames dans le plugin. OpenRGB livre également 100 ms pour ce profil. La [fiche du changement](../validation/conservative-timing.json) décrit le changement et ses limites sans publier les chemins de la machine.

Les tableaux suivants conservent les mesures historiques avec leurs réglages d’origine. L’appréciation initiale « plus fluide et stable » ne constitue pas une validation durable, puisqu’un problème est apparu ensuite. Un ACK USB ne prouve ni l’absence d’une file interne au moniteur, ni le délai d’apparition de la lumière. La validation visuelle du nouveau réglage à 100 ms reste **en attente**. Dans SignalRGB, le mode `Custom` peut remplacer la valeur recommandée ; le journal d’initialisation affiche l’intervalle réellement utilisé.

## Mesure de départ : pause de 200 ms après chaque écriture

Le plugin corrigé initial du 19 septembre 2026 conserve `device.pause(delayRGB)` après chaque paquet, avec `delayRGB=200`. Le mode global envoie un paquet au masque `09`. Le mode individuel envoie successivement logo `01` et bouton `08`, chacun suivi de sa pause.

Dans `signalrgb-colors.pcap`, les intervalles médians globaux sont proches de **249 ms**, soit **4 Hz**. Chaque zone du mode individuel est actualisée toutes les **452,5 ms**, soit environ **2,21 Hz par zone**. L’écart médian entre la commande logo et la commande bouton est **205,734 ms**. Les 269 commandes ont toutes un checksum valide et un ACK réussi : ces intervalles ne correspondent pas à des échecs de transport.

Un modèle explicatif simple est `durée du passage ≈ nombre de paquets × pause + temps des appels et de l’ordonnancement`. Il explique pourquoi deux zones indépendantes coûtent davantage qu’un masque commun. La part restante, environ 45–50 ms dans ces essais, n’est pas isolée par une mesure optique ou un profilage détaillé : elle ne doit pas être attribuée entièrement à la puce.

## Benchmark du 19 septembre 2026

Le test temporaire fait varier uniquement la pause de l’émetteur et utilise un dégradé identifiable par phase. Les **1 453 transferts enregistrés** sont valides. Les chiffres suivants sont des cadences de commandes USB de ce test, pas des FPS mesurés sur la lumière.

| Mode | Pause après écriture | Cadence globale ou par zone |
| --- | ---: | ---: |
| Global | 200 ms | 4,01 commandes/s |
| Global | 100 ms | 7,10 commandes/s |
| Global | 50 ms | 10,67 commandes/s |
| Global | 20 ms | 15,99 commandes/s |
| Global | 0 ms | 21,39 commandes/s |
| Individuel | 20 ms | 10,72 commandes/s **par zone** |
| Individuel | 0 ms | **Non mesuré** |

Le scénario prévoyait sept phases de dix secondes. Le plugin normal a été restauré environ 59 secondes après l’installation du test ; la phase individuelle à 0 ms n’a pas été capturée. Il serait incorrect de présenter cette expérience comme un essai complet de 70 secondes ou de lui attribuer un résultat pour cette phase.

L’utilisateur a décrit le dégradé comme **plus fluide et stable**. Cette observation qualitative complète les ACK, mais ne mesure ni la fréquence optique exacte, ni les pertes éventuelles invisibles, ni la stabilité sur plusieurs heures.

Les détails et bornes de phase sont dans [performance.json](../validation/performance.json). Les résultats de l’ordonnanceur mesuré ensuite à 20 ms restent distingués de cette série. Une échéance minimale de 20 ms dans un ordonnanceur sans pause bloquante n’est pas équivalente à ajouter une pause de 20 ms après chaque écriture.

## Ce que change un ordonnanceur sans pause bloquante

Au lieu de bloquer l’exécution après chaque paquet, l’émetteur peut mémoriser la prochaine échéance et revenir rapidement tant qu’elle n’est pas atteinte. Il peut aussi regrouper les zones de couleur identique, alterner équitablement les zones différentes et éviter une accumulation de trames déjà périmées. Cela réduit la latence logicielle et libère les appels de rendu.

Cette conception ne garantit pas un débit de 50 Hz parce qu’un intervalle de 20 ms a été configuré. Le moteur de rendu, les appels HID, le contrôleur et le moniteur participent au résultat. Le tableau précédent caractérise l’émetteur temporaire avec pauses ; celui qui suit mesure l’ordonnanceur au seuil historique de 20 ms.

## Vérification historique de l’ordonnanceur à 20 ms

La capture animée, dont le [résumé anonymisé est publié](../validation/usb-summary.json), utilise l’ordonnanceur sans pause bloquante avec un seuil de 20 ms, une source de couleurs temporaire pendant dix secondes globales puis dix secondes individuelles. Cette source de test a ensuite été retirée. Les **439/439 commandes** de la capture ont des longueurs et checksums valides ainsi qu’un ACK USB réussi, sans handshake E1.

| Segment animé | Commandes | Intervalle médian | Cadence |
| --- | ---: | ---: | ---: |
| Global, masque `09` | 209 | 45,91 ms | **21,78 commandes/s** |
| Logo, masque `01` | 110 | 92,02 ms | **10,87 commandes/s** |
| Bouton, masque `08` | 109 | 92,56 ms | **10,80 commandes/s** |

Ces trois segments comptent 428 commandes animées ; le total 439 inclut les autres commandes de la capture, avant ou après les segments retenus. Les statistiques sont publiées dans [final-scheduler.json](../validation/final-scheduler.json), avec le SHA256 `4b94903ba17160f3c1e0339a9f01a1c6b71566bdc963cf884dfbeea45d1ac649` de la capture.

Le gain mesuré est d’environ **5,4 fois en mode global** par rapport au point de départ à 4,01 commandes/s, et environ **4,9 fois par zone** par rapport au mode individuel initial proche de 2,21 Hz. Cela caractérise ces essais courts sur cet exemplaire d’AW3426DW. L’observation utilisateur de stabilité et fluidité concerne le premier benchmark ; aucune mesure optique supplémentaire n’a été réalisée pour la seconde série.

Une première capture de contrôle de cette version, signalrgb-suite-final.pcap (résumé dans [usb-summary.json](../validation/usb-summary.json)), contenait également 29/29 transferts valides. Dans le plugin installé, les couleurs immobiles sont dédupliquées puis rappelées après deux secondes : une couleur statique n’est donc volontairement pas réémise à la cadence maximale. L’intervalle testé de 20 ms concernait seulement AW3426DW et a depuis été remplacé par la recommandation de 100 ms. Les autres profils conservent leurs réglages documentés dans le code.

## Pourquoi conserver des réglages prudents pour les anciens modèles

Dans les échanges du 4 mai 2025 (notes de recherche locales), l’AW3225QF/Microchip ne répondait qu’en partie à 2 ms ; 100 ms puis 50 ms fonctionnaient, 35 ms échouait. Le 26 mai, Fefedu973 indiquait que 50 ms n’était pas fiable sur son DWF. Le benchmark AW3426DW ne remplace pas ces essais sur d’autres périphériques.

Sur les modèles authentifiés, un passage peut aussi inclure demande de challenge, lecture et réponse. Sur Microchip, l’initialisation et les contraintes de traitement sont différentes. Retirer toutes les pauses de toute la gamme sur la seule base du PID 101D ne serait pas une conclusion fondée.

## Mesure future de la cadence réelle des LED

Pour une valeur de FPS lumineux, filmer une alternance ou un compteur de couleurs connus avec une cadence de caméra suffisante, en synchronisant l’horodatage des commandes avec l’observation. Compter les changements physiquement visibles et les comparer aux commandes. La présente recherche ne fournit pas cette mesure et n’assimile pas ACK USB et réponse optique.
