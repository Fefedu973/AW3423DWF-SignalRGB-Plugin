# Plugins SignalRGB Alienware

Trois fichiers autonomes remplacent les contrôleurs Gen1/Gen2/Gen3 correspondants. Ils se placent dans le dossier utilisateur `Documents\WhirlwindFX\Plugins`, éventuellement redirigé par Windows ou OneDrive. Voir la [documentation officielle](https://docs.signalrgb.com/troubleshooting/advanced-troubleshooting/replacing-plugin/).

| Plugin | Périphériques |
| --- | --- |
| `Alienware_Monitor_Gen1_Controller.js` | VID 0424 : trois anciens écrans classiques et deux Microchip |
| `Alienware_Monitor_Gen2_Controller.js` | VID 187C : dix Microchip |
| `Alienware_Monitor_Gen3_Controller.js` | VID 187C : huit Realtek, avec authentification selon le modèle |

`profiles.json` donne la liste exacte. Les modèles sans RGB ou sans protocole/identité établis ne sont pas revendiqués par les plugins.

## Changements

- Profils, zones et masques par modèle ; deux zones seulement sur AW3426DW.
- Checksum complet pour toutes les commandes DDC, Report ID nul explicite, buffer HID65 ou 193 selon le transport.
- Initialisation classique conservée ; requête d'initialisation Microchip reprise de Gen2.
- Authentification protégée contre les lectures tronquées, sélection bornée parmi cinq clés AWCC, état par instance et temporisation après erreur. Un succès du contrôle I/O n'est pas présenté comme une preuve cryptographique d'acceptation de la clé.
- Rafraîchissement sans pause systématique en fin de commande ; une seule commande actualisée par appel, rotation équitable entre zones.
- Regroupement des zones de même couleur et suppression des émissions identiques, avec rappel toutes les deux secondes.

## Réglages

`Lighting Mode` conserve Canvas et Forced. `Individual LED Control` expose les zones indépendantes.

`Refresh Timing = Recommended` utilise **100 ms** entre commandes pour AW3426DW, 200 ms pour les autres Realtek et 50 ms pour les anciens transports. `Custom` utilise `Custom Minimum Interval (ms)`, borné de 0 à 500 ms ; il peut donc remplacer la valeur recommandée. Ce champ reprend l'identifiant historique `delayRGB`, mais n'impose plus une pause finale après chaque trame. Le réglage Recommended ignore une ancienne valeur personnalisée mémorisée. Le journal d'initialisation indique l'intervalle effectivement sélectionné.

Le réglage AW3426DW était de 20 ms pendant les captures de performance. Après un retour ultérieur signalant du retard visible, il a été porté à 100 ms. Cette valeur prudente n'a pas encore reçu de nouvelle validation optique. Les sauvegardes originales restent locales ; les [mesures historiques](../docs/performances.md) restent distinguées du réglage courant.

En cas de couleurs sautées sur un autre modèle, augmenter l'intervalle personnalisé et enregistrer une capture. Les délais des autres écrans n'ont pas été optimisés sur leur matériel. Une couleur fixe produit volontairement peu de trafic ; cela ne signifie pas que l'animation est limitée à cette cadence.

## Installation et retour arrière

Pour une autre installation, copier les trois fichiers `.js` dans le dossier utilisateur `WhirlwindFX\Plugins` de SignalRGB, puis laisser le logiciel les recharger ou le redémarrer. Conserver une seule copie active de chaque contrôleur dans ce dossier.

Avant installation, sauvegarder les fichiers personnalisés portant les mêmes noms. Pour revenir à la version précédente, restaurer ces sauvegardes ; pour retrouver les plugins fournis par SignalRGB, retirer uniquement les copies personnalisées ajoutées. Les anciens plugins spécifiques au modèle restent conservés dans `../plugins/` comme historique, et ne doivent pas être installés simultanément avec les contrôleurs de cette suite.

## Tests reproductibles

Avec Node.js, depuis ce dossier :

```text
node tests/test-plugins.cjs
```

Ces tests n'ouvrent aucun périphérique. Ils vérifient les 23 profils, les erreurs de lecture/écriture, la sélection des clés, l'indépendance des instances, le contrôle des intervalles, l'équité des zones, les couleurs immobiles et l'arrêt. Les fixtures incluent 146 réponses historiques et les 16 rapports fixes AWCC de l'AW3426DW, comparés sur les 192 octets USB.

Le générateur reproductible est `../outils/build-signalrgb.cjs`, sa source commune `../outils/controller-template.js`, et sa matrice `../models.json`. Son exécution régénère les trois fichiers livrés ; elle ne les installe pas dans SignalRGB.

Les captures historiques à 20 ms confirment les échanges USB sur AW3426DW. Elles ne mesurent pas la latence lumineuse ni la stabilité prolongée du réglage actuel à 100 ms. Les autres profils sont documentés et testés hors ligne, sans prétendre à une validation physique sur chaque écran.
