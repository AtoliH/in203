#include <cstdlib>
#include <string>
#include <iostream>
#include <SDL2/SDL.h>        
#include <SDL2/SDL_image.h>
#include <fstream>
#include <ctime>
#include <iomanip>      // std::setw
#include <chrono>
#include <thread>
#include <memory>
#include <mpi.h>

#include "parametres.hpp"
#include "galaxie.hpp"
 
int main(int argc, char ** argv)
{
    char commentaire[4096];
    int width, height;
    SDL_Event event;
    SDL_Window   * window;

    parametres param;


    std::ifstream fich("parametre.txt");
    fich >> width;
    fich.getline(commentaire, 4096);
    fich >> height;
    fich.getline(commentaire, 4096);
    fich >> param.apparition_civ;
    fich.getline(commentaire, 4096);
    fich >> param.disparition;
    fich.getline(commentaire, 4096);
    fich >> param.expansion;
    fich.getline(commentaire, 4096);
    fich >> param.inhabitable;
    fich.getline(commentaire, 4096);
    fich.close();

    std::cout << "Resume des parametres (proba par pas de temps): " << std::endl;
    std::cout << "\t Chance apparition civilisation techno : " << param.apparition_civ << std::endl;
    std::cout << "\t Chance disparition civilisation techno: " << param.disparition << std::endl;
    std::cout << "\t Chance expansion : " << param.expansion << std::endl;
    std::cout << "\t Chance inhabitable : " << param.inhabitable << std::endl;
    std::cout << "Proba minimale prise en compte : " << 1./RAND_MAX << std::endl;

    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);

    window = SDL_CreateWindow("Galaxie", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              width, height, SDL_WINDOW_SHOWN);

    galaxie g(width, height, param.apparition_civ);
    galaxie g_next(width, height);
    galaxie_renderer gr(window);

    int deltaT = (20*52840)/width;
    std::cout << "Pas de temps : " << deltaT << " années" << std::endl;

    std::cout << std::endl;

    gr.render(g);
    unsigned long long temps = 0;

    int nbp, rank, provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &nbp);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int pas = height / nbp;
    int ligne_debut = pas * rank;
    int ligne_fin = (rank == nbp - 1)? height : ligne_debut + pas;

    std::chrono::time_point<std::chrono::system_clock> start, end1, end2;
    while (1) {
        // Recouvrement calcul / affichage en mémoire partagée 
        start = std::chrono::system_clock::now();
        std::thread update([&end1, &param, &width, &height, &g, &g_next, &ligne_debut, &ligne_fin, &rank]() {
            //mise_a_jour(param, width, height, g.data(), g_next.data());
            mise_a_jour_partielle(param, width, height, g.data(), g_next.data(), ligne_debut, ligne_fin, rank);
            end1 = std::chrono::system_clock::now();
        });

        // Seul 1 processus s'occupe de l'affichage
        if (rank == 0) {
            std::thread render([&end2, &gr, &g]() {
                gr.render(g);
                end2 = std::chrono::system_clock::now();
            });

            render.join();
        }

        update.join();
        
        int datasize = (ligne_fin - ligne_debut) * width;
        MPI_Gather(g_next.data() + ligne_debut * width, datasize, MPI_CHAR, g_next.data(), datasize, MPI_CHAR, 0, MPI_COMM_WORLD); 

        g_next.swap(g);
        
        if (rank == 0) {
            std::chrono::duration<double> elaps1 = end1 - start;
            std::chrono::duration<double> elaps2 = end2 - start;
            
            temps += deltaT;
            std::cout << "Temps passe : "
                      << std::setw(10) << temps << " années"
                      << std::fixed << std::setprecision(3)
                      << "  " << "|  CPU(ms) : calcul " << elaps1.count()*1000
                      << "  " << "affichage " << elaps2.count()*1000
                      << "\r" << std::flush;
            //_sleep(1000);
            if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
              std::cout << std::endl << "The end" << std::endl;
              break;
            }
        }
    }

    MPI_Finalize();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
