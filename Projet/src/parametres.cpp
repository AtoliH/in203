#include <cstdlib>
#include <cassert>
#include <ctime>
#include <iostream>
#include <random>
#include <mpi.h>
#include "galaxie.hpp"
#include "parametres.hpp"

double nombre_aleatoire()
{
    static thread_local std::random_device rd;
    static thread_local std::mt19937 generateur(rd()); // Un seul générateur par thread car couteux
    std::uniform_real_distribution<double> distribution(0.0, 1.0); // Opération peu couteuse que l'on peut se permettre
    return distribution(generateur);
}

int nombre_aleatoire_entier(int min, int max)
{
    static thread_local std::random_device rd;
    static thread_local std::mt19937 generateur(rd());
    std::uniform_int_distribution<double> distribution(min, max);
    return distribution(generateur);
}

expansion calcul_expansion(const parametres& c)
{
    double val = nombre_aleatoire();
    if (val < 0.01*c.expansion)     // parmi c.expansion, on a 1% de chance d'expansion isotrope...
        return expansion_isotrope;
    if (val < c.expansion)          // ... et 99% de chance d'expansion dans 1 seule direction
        return expansion_unique;
    return pas_d_expansion;
}
//_ ______________________________________________________________________________________________ _
bool calcul_depeuplement(const parametres& c)
{
    double val = nombre_aleatoire();
    if (val < c.disparition)
        return true;
    return false;   
}
//_ ______________________________________________________________________________________________ _
bool calcul_inhabitable(const parametres& c)
{
    double val = nombre_aleatoire();
    if (val < c.inhabitable)
        return true;
    return false;
}
//_ ______________________________________________________________________________________________ _
bool apparition_technologie(const parametres& p)
{
    double val = nombre_aleatoire();
    if (val < p.apparition_civ)
        return true;
    return false;
}
//_ ______________________________________________________________________________________________ _
bool a_un_systeme_proche_colonisable(int i, int j, int width, int height, const char* galaxie)
{
    assert(i >= 0);
    assert(j >= 0);
    assert(i < height);
    assert(j < width);

    if ( (i>0) && (galaxie[(i-1)*width+j] == habitable)) return true;
    if ( (i<height-1) && (galaxie[(i+1)*width+j] == habitable)) return true;
    if ( (j>0) && (galaxie[i*width+j-1] == habitable)) return true;
    if ( (j<width-1) && (galaxie[i*width+j+1] == habitable)) return true;

    return false;
}
//_ ______________________________________________________________________________________________ _
    void 
mise_a_jour(const parametres& params, int width, int height, const char* galaxie_previous, char* galaxie_next)
{
    int i;

    memcpy(galaxie_next, galaxie_previous, width*height*sizeof(char));

#pragma omp parallel for
    for ( i = 0; i < height; ++i )
    {
        for ( int j = 0; j < width; ++j )
        {
            if (galaxie_previous[i*width+j] == habitee)
            {
                if ( a_un_systeme_proche_colonisable(i, j, width, height, galaxie_previous) )
                {
                    expansion e = calcul_expansion(params);
                    if (e == expansion_isotrope)
                    {
                        if ( (i > 0) && (galaxie_previous[(i-1)*width+j] != inhabitable) )
                        {
#                           pragma omp atomic write
                            galaxie_next[(i-1)*width+j] = habitee;
                        }
                        if ( (i < height-1) && (galaxie_previous[(i+1)*width+j] != inhabitable) )
                        {
#                           pragma omp atomic write
                            galaxie_next[(i+1)*width+j] = habitee;
                        }
                        if ( (j > 0) && (galaxie_previous[i*width+j-1] != inhabitable) )
                        {
#                           pragma omp atomic write
                            galaxie_next[i*width+j-1] = habitee;
                        }
                        if ( (j < width-1) && (galaxie_previous[i*width+j+1] != inhabitable) )
                        {
#                           pragma omp atomic write
                            galaxie_next[i*width+j+1] = habitee;
                        }
                    }
                    else if (e == expansion_unique)
                    {
                        // Calcul de la direction de l'expansion :
                        int ok = 0;
                        do
                        {
                            int dir = nombre_aleatoire_entier(0,3);
                            if ( (i>0) && (0 == dir) && (galaxie_previous[(i-1)*width+j] != inhabitable) )
                            {
#                               pragma omp atomic write
                                galaxie_next[(i-1)*width+j] = habitee;
                                ok = 1;
                            }
                            if ( (i<height-1) && (1 == dir) && (galaxie_previous[(i+1)*width+j] != inhabitable) )
                            {
#                               pragma omp atomic write
                                galaxie_next[(i+1)*width+j] = habitee;
                                ok = 1;
                            }
                            if ( (j>0) && (2 == dir) && (galaxie_previous[i*width+j-1] != inhabitable) )
                            {
#                               pragma omp atomic write
                                galaxie_next[i*width+j-1] = habitee;
                                ok = 1;
                            }
                            if ( (j<width-1) && (3 == dir) && (galaxie_previous[i*width+j+1] != inhabitable) )
                            {
#                               pragma omp atomic write
                                galaxie_next[i*width+j+1] = habitee;
                                ok = 1;
                            }
                        } while (ok == 0);
                    }// End if (e == expansion_unique)
                }// Fin si il y a encore un monde non habite et habitable
                if (calcul_depeuplement(params))
                {
#                   pragma omp atomic write
                    galaxie_next[i*width+j] = habitable;
                }
                if (calcul_inhabitable(params))
                {
#                   pragma omp atomic write
                    galaxie_next[i*width+j] = inhabitable;
                }
            }  // Fin si habitee
            else if (galaxie_previous[i*width+j] == habitable)
            {
                if (apparition_technologie(params))
                {
#                   pragma omp atomic write
                    galaxie_next[i*width+j] = habitee;
                }
            }
            else { // inhabitable
                // nothing to do : le systeme a explose
            }
            // if (galaxie_previous...)
        }// for (j)
    }// for (i)

}
//_ ______________________________________________________________________________________________ _
    void 
mise_a_jour_partielle(const parametres& params, int width, int height, const char* galaxie_previous, char* galaxie_next,
        int start, int end, int rank)
{
    int i;

    memcpy(galaxie_next, galaxie_previous, width*height*sizeof(char));

    int fantome_start = std::max(start - 1, 0);
    int fantome_end = std::min(end + 1, height);

#pragma omp parallel for
    for ( i = fantome_start; i < fantome_end; ++i )
    {
        for ( int j = 0; j < width; ++j )
        {
            if (galaxie_previous[i*width+j] == habitee)
            {
                if ( a_un_systeme_proche_colonisable(i, j, width, height, galaxie_previous) )
                {
                    expansion e = calcul_expansion(params);
                    if (e == expansion_isotrope)
                    {
                        if ( (i > 0) && (galaxie_previous[(i-1)*width+j] != inhabitable) )
                        {
#                           pragma omp atomic write
                            galaxie_next[(i-1)*width+j] = habitee;
                        }
                        if ( (i < height-1) && (galaxie_previous[(i+1)*width+j] != inhabitable) )
                        {
#                           pragma omp atomic write
                            galaxie_next[(i+1)*width+j] = habitee;
                        }
                        if ( (j > 0) && (galaxie_previous[i*width+j-1] != inhabitable) )
                        {
#                           pragma omp atomic write
                            galaxie_next[i*width+j-1] = habitee;
                        }
                        if ( (j < width-1) && (galaxie_previous[i*width+j+1] != inhabitable) )
                        {
#                           pragma omp atomic write
                            galaxie_next[i*width+j+1] = habitee;
                        }
                    }
                    else if (e == expansion_unique)
                    {
                        // Calcul de la direction de l'expansion :
                        int ok = 0;
                        do
                        {
                            int dir = nombre_aleatoire_entier(0,3);
                            if ( (i>0) && (0 == dir) && (galaxie_previous[(i-1)*width+j] != inhabitable) )
                            {
#                               pragma omp atomic write
                                galaxie_next[(i-1)*width+j] = habitee;
                                ok = 1;
                            }
                            if ( (i<height-1) && (1 == dir) && (galaxie_previous[(i+1)*width+j] != inhabitable) )
                            {
#                               pragma omp atomic write
                                galaxie_next[(i+1)*width+j] = habitee;
                                ok = 1;
                            }
                            if ( (j>0) && (2 == dir) && (galaxie_previous[i*width+j-1] != inhabitable) )
                            {
#                               pragma omp atomic write
                                galaxie_next[i*width+j-1] = habitee;
                                ok = 1;
                            }
                            if ( (j<width-1) && (3 == dir) && (galaxie_previous[i*width+j+1] != inhabitable) )
                            {
#                               pragma omp atomic write
                                galaxie_next[i*width+j+1] = habitee;
                                ok = 1;
                            }
                        } while (ok == 0);
                    }// End if (e == expansion_unique)
                }// Fin si il y a encore un monde non habite et habitable
                if (calcul_depeuplement(params))
                {
#                   pragma omp atomic write
                    galaxie_next[i*width+j] = habitable;
                }
                if (calcul_inhabitable(params))
                {
#                   pragma omp atomic write
                    galaxie_next[i*width+j] = inhabitable;
                }
            }  // Fin si habitee
            else if (galaxie_previous[i*width+j] == habitable)
            {
                if (apparition_technologie(params))
                {
#                   pragma omp atomic write
                    galaxie_next[i*width+j] = habitee;
                }
            }
            else { // inhabitable
                // nothing to do : le systeme a explose
            }
            // if (galaxie_previous...)
        }// for (j)
    }// for (i)

    // Percolation
    char cellules_fantomes[width*2];
    if (rank % 2 != 0) {
        if (fantome_start != start) {
            MPI_Send(galaxie_next + fantome_start * width, width * 2, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD);
            MPI_Recv(galaxie_next + fantome_start * width, width * 2, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, NULL);
        }
        if (fantome_end != end) {
            MPI_Send(galaxie_next + end * width, width * 2, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD);
            MPI_Recv(galaxie_next + end * width, width * 2, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD, NULL);
        }
    } else {
        if (fantome_end != end) {
            MPI_Recv(cellules_fantomes, width * 2, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD, NULL);
            for (int j = 0; j < width * 2; ++j) {
                char c1 = cellules_fantomes[j];
                char c2 = galaxie_next[end*width + j];

                if (c1 == inhabitable || c2 == inhabitable)
                    galaxie_next[end*width + j] = inhabitable;
                else if (c1 == habitable || c2 == habitable)
                    galaxie_next[end*width + j] = habitable;
                else
                    galaxie_next[end * width + j] = habitee;
            }
            MPI_Send(galaxie_next + end * width, width * 2, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD);
        }
        if (fantome_start != start) {
            MPI_Recv(cellules_fantomes, width * 2, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, NULL);
            for (int j = 0; j < width * 2; ++j) {
                char c1 = cellules_fantomes[j];
                char c2 = galaxie_next[fantome_start*width + j];

                if (c1 == inhabitable || c2 == inhabitable)
                    galaxie_next[fantome_start*width + j] = inhabitable;
                else if (c1 == habitable || c2 == habitable)
                    galaxie_next[fantome_start*width + j] = habitable;
                else
                    galaxie_next[fantome_start * width + j] = habitee;
            }
            MPI_Send(galaxie_next + fantome_start * width, width * 2, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD);
        }
    }
}
//_ ______________________________________________________________________________________________ _
