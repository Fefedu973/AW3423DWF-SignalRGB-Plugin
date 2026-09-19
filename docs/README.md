# Les recherches Alienware RGB de Fefedu973

Cette documentation reconstruit les recherches retrouvées entre août 2023 et septembre 2026 : premiers paquets AW3423DWF, compréhension du challenge/réponse, outils de validation, extension à d’autres modèles, contributions SignalRGB/OpenRGB, puis diagnostic de l’AW3426DW. Elle associe chaque conclusion à une source ou à une expérience et conserve les pistes qui se sont révélées fausses.

Le résultat majeur de 2026 est précis : **AW3426DW, VID `187C`, PID `101D`, utilise l’enveloppe Realtek mais son pilote AWCC ne demande pas d’authentification**. Son crash initial SignalRGB venait d’un profil absent. Les captures et la confirmation de l’utilisateur valident désormais le contrôle du logo et du bouton d’alimentation. La clé dite universelle reste pertinente pour documenter les anciens modèles protégés ; elle n’explique pas cette panne.

| Lire pour… | Document |
| --- | --- |
| Comprendre la progression et les contributions de chacun | [Historique 2023–2026](historique.md) |
| Reconstruire les rapports, les masques, les checksums et le keygen | [Protocoles et authentification](protocoles.md) |
| Refaire une capture et une analyse, retrouver les outils d’époque | [Outils et méthode](outils-et-methode.md) |
| Comprendre les débits RGB et les limites des mesures | [Performances](performances.md) |
| Lire correctement les statuts par modèle | [Support et niveaux de preuve](support.md) |
| Retrouver les liens, archives, empreintes et limites de couverture | [Sources et preuves](sources.md) |

La [matrice JSON](../models.json), le [CSV](../models.csv) et le [classeur](../Alienware-RGB-Moniteurs.xlsx) recensent **42 modèles**, avec les VID/PID, protocoles et statuts de test lorsqu’ils sont connus. Ils couvrent les 40 références de la FAQ Dell consultée, plus AW3226Q et l’annonce AW2527HX. Ce catalogue dépasse les **23 profils USB** implémentés dans la suite : un modèle présent dans le tableau n’est pas automatiquement pris en charge.

Les noms « V2 », « V2,5 », « V9 » et « Gen3 » viennent de classements historiques différents ; ils ne sont pas interchangeables. Les archives complètes de recherche, conservées localement et exclues de Git conservent les fichiers d’époque ; les petites preuves utilisées ici sont indexées dans [Sources](sources.md).

## Ce que signifient les preuves

Une identité dans un descripteur USB, une branche explicite du pilote, une capture réussie et une observation physique répondent à des questions différentes. Une réponse de keygen identique à celle d’une capture prouve un calcul, pas l’acceptation de cette clé par tous les modèles. Un ACK USB prouve la réussite du transfert, pas le moment où la LED change. Une mention « fully working » dans Discord documente un témoignage d’époque, pas une revalidation sur le matériel actuel.

Les originaux de recherche sont conservés. Les documents de la suite n’affirment aucune intention du constructeur à partir de la présence d’un handshake. Les futures contributions devront conserver ces distinctions et compléter les preuves par modèle.
