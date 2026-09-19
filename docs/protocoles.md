# Protocoles, rapports et authentification

[Navigation](README.md) · [Sources](sources.md) · [Méthode reproductible](outils-et-methode.md)

## Quatre informations à conserver séparément

Un profil de moniteur doit préciser **son identité USB**, **sa famille de transport**, **son besoin d’authentification** et **sa carte des zones**. Une année de sortie ou un nom commercial voisin ne remplace aucun de ces champs.

| Terme des archives | Sens observé | Limite |
| --- | --- | --- |
| V1, V2, V2,5 dans les Excel | Classement de travail de Fefedu973 ; V2,5 indique le handshake OEM | Ce n’est pas une norme ni la version du firmware. |
| V6 / V9 chez T-Troll | Familles de commandes de son SDK ; V9 décrit les rapports Realtek longs | Le support V9 reste commenté dans la copie AlienFX Tools consultée. |
| Gen1 / Gen2 / Gen3 dans SignalRGB | Regroupement de plugins | Gen3 ne signifie pas automatiquement authentification active. |
| `MicroChip5734` / `RTS5409S` dans AWCC | Deux implémentations HID de la bibliothèque commune | Un PID dans la factory est une preuve de chemin logiciel, pas une capture matérielle. |

AW3423DWF (`100e`) et AW2724DM (`1010`) suivent le chemin Realtek avec authentification. AW3426DW (`101d`) suit Realtek sans authentification dans AWCC 6.14.24.0. La factory emploie également Realtek sans handshake pour d’autres PID ; les noms et niveaux de validation sont consignés dans la matrice, sans en déduire un support matériel automatique.

## Convention des octets et des longueurs

Tous les offsets ci-dessous commencent à zéro dans le **payload USB**, après retrait de l’en-tête USBPcap et du setup de contrôle éventuel. Ils ne désignent pas les positions dans un hexdump complet de la trame.

Sur l’AW3426DW capturé, un rapport USB commence par `40 c6` et mesure **192 octets**. Le tableau utilisé avec l’API HID commence par `00 40 c6` et mesure **193 octets** : le premier zéro occupe l’emplacement du Report ID nul. Son retrait pendant l’envoi a été constaté dans une capture SignalRGB réelle. La convention du premier octet est décrite par [SignalRGB](https://docs.signalrgb.com/developer/plugins/writes-and-reads/) et [HIDAPI](https://github.com/libusb/hidapi/blob/master/hidapi/hidapi.h).

Le transport capturé est `SET_REPORT` Output : `bmRequestType=21`, `bRequest=09`, `wValue=0200`, interface 0, longueur `00c0`. Un Feature report (`0300`) n’est pas interchangeable avec cet Output report. L’endpoint SignalRGB observé est interface 0, usage `00da`, usage page `ffda`, collection 0.

## Enveloppe Realtek et couleur fixe

| Offset USB | Index dans le tableau HID | Valeur ou rôle |
| ---: | ---: | --- |
| 0–1 | 1–2 | `40 c6` |
| 6–7 | 7–8 | Longueur du bloc DDC, little endian ; `0a 00` pour la couleur fixe |
| 8 | 9 | `6e` |
| 10 | 11 | `82` |
| 64–67 | 65–68 | `51 87 d0 04` |
| 68 | 69 | Masque des zones |
| 69–71 | 70–72 | Rouge, vert, bleu |
| 72 | 73 | Luminosité ; `64` hexadécimal = 100 |
| 73 | 74 | Checksum DDC |
| Autres | Autres | Zéros dans les commandes fixes de référence AW3426DW |

Le masque est un OR de bits de zones. Dans le DWF historique : logo `01`, numéro `02`, bouton `08`, ensemble `0b`. Dans l’AW3426DW : logo `01`, bouton `08`, ensemble `09`. Un envoi commun est possible lorsque les zones doivent recevoir la même couleur ; deux couleurs différentes requièrent deux commandes sur ce chemin.

```text
checksum = 6e XOR 51 XOR 87 XOR d0 XOR 04 XOR masque XOR R XOR G XOR B XOR luminosité
```

Une façon équivalente de valider un bloc complet est de XORer tous ses octets, checksum inclus, avec `6e` : le résultat doit être zéro. La longueur doit aussi être cohérente avec le champ de longueur DDC. L’analyseur ne remplace pas une erreur de longueur par un simple checksum apparemment valide.

Exemples capturés :

```text
AW3426DW, rouge global : 51 87 d0 04 09 ff 00 00 64 fe
AW3426DW, vert logo   : 51 87 d0 04 01 00 ff 00 64 f6
AW3426DW, bleu bouton : 51 87 d0 04 08 00 00 ff 64 ff
```

Rouge, vert, bleu purs et blanc ont tous `R XOR G XOR B = ff`. Ils peuvent donc masquer l’erreur d’un checksum constant. Une vérification sérieuse doit inclure des couleurs non primaires et une luminosité différente. Les formules anciennes omettant `6e` ou le XOR du préfixe `51 87 d0 04` ne sont pas équivalentes à la formule Dell.

Les archives décrivent aussi Pulse, morphing et temporisation. La capture AWCC actuelle contient deux Pulse de mode `02`, distincts des 16 couleurs fixes de mode `04`. Ils ont été décodés, mais ne constituent pas un test de tous les effets matériels. Un plugin Canvas peut produire une animation avec des couleurs fixes successives sans programmer un effet autonome du moniteur.

## Challenge/réponse des modèles protégés

Séquence constatée dans les captures anciennes :

1. Rapport commençant par `40 e1 01` : demande du challenge.
2. `GET_REPORT` Input : `a1 01`, valeur `0100`, interface 0. Les **16 premiers octets du payload retourné** constituent le challenge.
3. Rapport `40 e1 02`, réponse calculée de **8 octets aux offsets USB 64–71**.
4. Rapport `40 c6` contenant le bloc DDC à envoyer.

La réponse n’est pas le checksum de couleur. Elle est calculée à partir du challenge et d’une clé OEM. La couleur et le masque ne figurent pas dans cette fonction. Les captures et les implémentations historiques répètent parfois ce handshake avant chaque écriture ; cela ne démontre pas qu’une authentification par session serait suffisante. Le seuil de réauthentification doit être établi sur le modèle testé.

### Réduire une clé OEM

L’algorithme transforme la table OEM de longueur variable en huit octets. `p` parcourt la clé, `sortie` commence vide :

```text
tant que p < taille(OEM) et taille(sortie) < 8 :
    n = (OEM[p] & 1) + ((OEM[p] & 0x10) >> 4)
    répéter n fois, tant que sortie contient moins de 8 octets :
        exiger que p + 1 < taille(OEM)
        ajouter OEM[p] XOR OEM[p + 1] à sortie
        p = p + 1
    p = p + 1
exiger exactement 8 octets en sortie
```

Les marqueurs et les données se chevauchent selon l’avancement de `p`. Il faut reproduire cette boucle et ses limites, sans supprimer les octets zéro.

### Construire la réponse

Pour un challenge `C` de 16 octets et la clé réduite `S` :

```text
v = (C[0] << 8) | C[15]
si nombre_de_bits_à_1(v) est impair :
    M = copie de C[0:8]
    i = C[14] & 7
    M[i] = M[i] XOR C[i + 8]
sinon :
    M = copie de C[8:16]
    i = C[6] & 7
    M[i] = M[i] XOR C[i]
réponse[j] = S[j] XOR M[j], pour j de 0 à 7
```

Il faut tester la parité du nombre de bits, pas celle d’un des octets. Une erreur de priorité dans une expression telle que `length - 1 % 2` change le calcul. Les deux branches sont représentées dans les captures historiques vérifiées.

### Vecteur historique et portée des clés

```text
OEM       : f5 3f c1 39 44 3a 31 79 0d b1 82 76
Réduite   : ca fe 7d 7e 48 74 33 f4
Challenge : 2c 20 76 84 f1 55 90 91 67 e4 80 65 9d 52 36 ef
Réponse   : 81 1a fd 1b d5 26 05 1b
```

Les notes de recherche du **9 février 2025** consignent ce vecteur positif ; la capture d’écran privée reste dans les archives locales. La clé porte l’étiquette historique « universelle » ; son index est 7 dans l’ancienne table DWF mais change dans d’autres tables. Il faut référencer sa valeur ou un identifiant stable, pas supposer que l’index 7 a toujours le même sens.

Les captures AW2724DM analysées utilisent une autre clé, étiquetée `dm-8` dans l’analyseur :

```text
OEM     : 7e 7f 18 00 2b 33 fb fd e3 64 d0 c0 04 b0 da c7 b5
Réduite : 01 18 18 06 1e 10 6a 72
```

Bilan sur quatre captures historiques identifiées par descripteur : **153/153 réponses reproduites**, dont **50** avec la clé commune. Les **150/150 checksums DDC** sont valides. Les sept autres réponses du corpus DWF et les captures DM ne doivent pas être attribuées arbitrairement à la clé commune. Les fixtures publiées isolent les paquets de protocole utiles ; le corpus complet et sa table de clés restent locaux.

Ces valeurs sont des constantes de protocole retrouvées dans les outils et bibliothèques, pas des identifiants de compte ou des clés Discord. Leur présence dans une bibliothèque actuelle ne démontre pas leur acceptation par tous les appareils. Sur l’AW3426DW testé, le chemin normal ne les emploie pas.

### Choix d’une clé dans les implémentations consolidées

Le catalogue historique complet reste dans les archives locales. Le code public conserve seulement les **cinq constantes de protocole du chemin AWCC actuel** : la clé commune, la clé `dm-8` et les trois clés étiquetées `awcc-current-2/3/4`. L’AW2724DM commence par la clé prouvée dans ses captures ; les autres modèles authentifiés commencent par la clé commune. L’index retenu appartient à l’instance du périphérique.

La sélection est bornée et emploie après authentification un petit bloc DDC de contrôle `51 82 01 c8 74`, encapsulé avec la valeur de vitesse 0. Un succès d’I/O de cette requête ne constitue **pas** un reçu cryptographique d’acceptation de la clé. Les erreurs de lecture, rapports tronqués ou écritures partielles sont vérifiés ; une erreur invalide le cache et déclenche une attente avant nouvel essai. Ces précautions améliorent la robustesse sans transformer les profils encore non testés en compatibilités prouvées.

Pour un modèle protégé, le challenge/réponse est refait avant les couleurs. La suite ne remplace pas toutes les authentifications par un commutateur global. Les détails de temporisation diffèrent entre les adaptateurs SignalRGB et OpenRGB et sont visibles dans leurs sources et tests.

## Microchip : enveloppe `92 37`

Les sources Gen2 et le chemin Microchip encapsulent le même bloc DDC dans un rapport plus court. Le tableau API HID contient 65 octets, dont le premier est le Report ID nul ; le payload de transport en contient 64. Le format fixe est :

```text
Offset USB 0 : 92 37 0a 00
Offset USB 4 : 51 87 d0 04 MASK R G B BRIGHTNESS CHECKSUM
Après le bloc : remplissage ff dans les commandes couleur de référence
```

Le checksum porte sur le bloc DDC, avec la même graine `6e`. La requête d’initialisation du plugin Gen2 est `92 37 05 00 51 82 d0 f4 99`. Son commentaire historique la nomme « Direct Mode », mais l’effet précis de cette requête n’a pas été démontré ici comme un changement de mode. La luminosité varie dans les versions historiques : le plugin public consulté utilise `64` pour AW3225QF et `23` pour d’autres profils. Les profils consolidés et leurs validations sont décrits avec le code de la suite ; ces différences historiques ne doivent pas être effacées lors d’une comparaison d’octets.

OpenRGB conserve pour l’AW3225QF sa propre initialisation déjà présente en amont ; pour les profils ajoutés, il reprend la requête Gen2 ci-dessus. SignalRGB conserve son initialisation historique Gen2. Ces chemins n’ont pas été validés physiquement sur chaque moniteur Microchip pendant la session.

## Anciens moniteurs : format `92 48`

Le plugin Gen1 public associe `0424:274A` à AW3418DW, `274B` à AW3418HW et `274C` à AW2518H. Il initialise avec `95 00 00 00` et un remplissage `ff`, puis écrit :

```text
Offset USB 0 : 92 48 05 00 04 MASK R G B
Après la couleur : remplissage ff
```

Ce format ne contient pas le préfixe DDC `51 87 d0` et le code public n’y ajoute aucun checksum. Il ne faut donc pas lui appliquer mécaniquement la formule DDC. La suite conserve une entrée API de 65 octets avec Report ID nul ; la version publique historique remplissait aussi le premier emplacement avec `ff`. Cette observation de code ne constitue pas une capture matérielle de Gen1 pendant la session.

Les notes de recherche du 9 février 2025 discute l’initialisation `95` de l’ancienne famille et recommande de ne l’envoyer qu’au démarrage. Le vocabulaire historique « Microchip » était parfois plus large que cette séparation de formats. Pour une implémentation reproductible, distinguer explicitement `92 48`, `92 37` et `40 c6`.

Il ne faut pas transposer l’offset 64 des rapports Realtek, un handshake `40 e1` ou le remplissage zéro de l’AW3426DW à ces appareils. Les observations de délai de l’AW3225QF sont utiles comme avertissement technique, mais ne fixent pas une fréquence maximale universelle.

## Décider quoi implémenter pour un modèle supplémentaire

Partir d’une identité attestée par descripteur ou métadonnées constructeur, conserver le profil d’endpoint, identifier la classe et le booléen d’authentification du pilote, puis les vérifier par capture. Comparer des couleurs asymétriques, toutes les zones et la luminosité. Déterminer ensuite le délai fiable et le comportement au redémarrage. Un modèle ne devient validé matériellement qu’après cette chaîne de preuves et un contrôle physique.
