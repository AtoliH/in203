# Mémoire partagée
On utilise OpenMP pour paralléliser le code en mémoire partagée. On favorise l'utilisation de la bibliothèque random de c++ plutôt que std::rand. Un seul générateur est créé par thread car la création de la seed avec random_device et la création du générateur coutent chers. On utilise le mot clé thread_local et on déclare la variable statiquement afin de faire en sorte qu'elles ne soient déclarées qu'une fois par thread.

De potentiels data races peuvent survenir dans le cas où deux threads voudraient écrire sur une même adresse de galaxie_next. On s'assure donc de protéger ces écritures en utilisant pragma omp atomic write. Les threads n'écrivant que sur galaxie_next, d'autres data races ne sont pas possibles.

Les gains de performances sont malheureusement difficiles à observer en raison du nombre restreient de coeur de la machine sur laquelle ont été effectués les tests (seulement 2).
On notera donc une décevante baisse de performance suite à l'implémentation de la parallélisation en mémoire partagée. On passe de 12ms de temps de calcul à 14ms. Cela est probablement dû à l'utilisation des générateurs de nombre aléatoire de c++ dont le coût de création est assez élevé. Ce coût n'aura pas pu être rattrappé par le peu de coeurs disponibles.

# Mémoire distribuée
On utilise ensuite Open MPI pour paralléliser le code en mémoire distribuée. La méthode choisie ici et d'assigner à chaque processus une bande horizontale. Toutes ces bandes seront de taille à peu près égale. Une fois que chaque processus a terminé les calculs sur sa bande, tout est envoyé a un processus principal qui s'occupe de recoller les morceaux et d'afficher le tout.

Aucun conflit de mémoire n'a lieux étant donné que chaque processus travaille sur une partie différente de la galaxie. Il faudra tout de même faire attention au niveau des raccords, où l'on percolera de façon efficace. Seuls deux MPI_Send/MPI_Recv sont ici nécessaires. Dans un premier temps les processus pairs échangent leurs données avec leur voisin suivant et dans un second temps les processus impairs échangent leurs données avec les voisins suivants. On évite ainsi tout problème de verrouillage mortel. 

N'ayant que deux coeurs sur ma machine, j'ai fait en sorte que le processus 0 fasse une partie des calculs en plus de faire le rendu graphique, afin d'avoir des résultats plus intéressants. Le coup de l'affichage étant négligeable par rapport au calcul, cela a peu d'impact sur le résultat. On obtient ainsi une nette amélioration du temps de calcul avec des threads effectuant le calcul en 9ms, donc qelques milisecondes de gagnées.
