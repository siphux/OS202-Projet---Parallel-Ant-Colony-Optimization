#include "pheromone_mpi.hpp"
#include <vector>
#include <iostream>
#include <random>
#include "fractal_land.hpp"
#include "ant_vect.hpp"

# include "renderer_vect.hpp"
# include "window.hpp"
# include "rand_generator.hpp"
#include <chrono>
#if defined(_OPENMP)
#include <omp.h>
#endif
#include <mpi.h>


struct time_measure {
    double total = 0.0;
    double compute = 0.0;
    double render = 0.0;
    double ants_move = 0.0;
    double evaporation = 0.0;
    double pheromones_update = 0.0;
    double mpi_comms = 0.0;
} T;



void advance_time( const fractal_land& land, pheronome& phen, 
                   const position_t& pos_nest, const position_t& pos_food,
                   ants_vect& ants, std::size_t& cpteur, int rank, int size)
{
    auto t_start_ants = std::chrono::high_resolution_clock::now();
    ants.advance(phen, land, pos_food, pos_nest, cpteur);
    auto t_end_ants = std::chrono::high_resolution_clock::now();

    auto t_start_sync1 = std::chrono::high_resolution_clock::now();
    MPI_Allreduce(MPI_IN_PLACE, phen.data_buffer(), phen.data_size(), MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    auto t_end_sync1 = std::chrono::high_resolution_clock::now();

    int row_per_proc = phen.get_dim() / size;
    int rem = phen.get_dim() % size;
    int start_row = 1 + rank * row_per_proc + std::min(rank, rem);
    int local_rows = row_per_proc + (rank < rem ? 1 : 0);
    phen.do_evaporation_mpi(start_row, start_row + local_rows);
    auto t_end_evap = std::chrono::high_resolution_clock::now();

    auto t_start_sync2 = std::chrono::high_resolution_clock::now();
    phen.sync_evaporation_mpi(start_row, local_rows, size, rank);
    auto t_end_sync2 = std::chrono::high_resolution_clock::now();
    phen.update();
    auto t_end_update = std::chrono::high_resolution_clock::now();
    T.ants_move += std::chrono::duration<double>(t_end_ants - t_start_ants).count();
    T.mpi_comms += std::chrono::duration<double>(t_end_sync1 - t_start_sync1).count();
    T.evaporation += std::chrono::duration<double>(t_end_evap - t_end_sync1).count();
    T.mpi_comms += std::chrono::duration<double>(t_end_sync2 - t_start_sync2).count();
    T.pheromones_update += std::chrono::duration<double>(t_end_update - t_end_sync2).count();
}

int main(int nargs, char* argv[])
{   

    MPI_Init(&nargs, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (rank == 0){
        SDL_Init( SDL_INIT_VIDEO );
    }

    std::size_t seed = 2026; // Graine pour la génération aléatoire ( reproductible )
    const int nb_ants = 500000; // Nombre de fourmis
    const double eps = 0.8;  // Coefficient d'exploration
    const double alpha=0.7; // Coefficient de chaos
    //const double beta=0.9999; // Coefficient d'évaporation
    const double beta=0.999; // Coefficient d'évaporation
    // Location du nid
    position_t pos_nest{256,256};
    // Location de la nourriture
    position_t pos_food{500,500};
    //const int i_food = 500, j_food = 500;    
    // Génération du territoire 512 x 512 ( 2*(2^8) par direction )
    fractal_land land(8,2,1.,1024);
    double max_val = 0.0;
    double min_val = 0.0;
    for ( fractal_land::dim_t i = 0; i < land.dimensions(); ++i )
        for ( fractal_land::dim_t j = 0; j < land.dimensions(); ++j ) {
            max_val = std::max(max_val, land(i,j));
            min_val = std::min(min_val, land(i,j));
        }
    double delta = max_val - min_val;
    /* On redimensionne les valeurs de fractal_land de sorte que les valeurs
    soient comprises entre zéro et un */
    for ( fractal_land::dim_t i = 0; i < land.dimensions(); ++i )
        for ( fractal_land::dim_t j = 0; j < land.dimensions(); ++j )  {
            land(i,j) = (land(i,j)-min_val)/delta;
        }
    // Définition du coefficient d'exploration de toutes les fourmis.
    ants_vect::set_exploration_coef(eps);
    // On va créer des fourmis un peu partout sur la carte :
    //pour avoir le même comportement que la version séquentielle, on utilise la même seed sur tous les processus et on ne garde que les fourmis dans la nbp ème plage de valeur.
    std::size_t local_ants_start = rank * (nb_ants / size) + std::min(rank, nb_ants % size);
    std::size_t local_ants_count = nb_ants / size + (rank < nb_ants % size ? 1 : 0);
    std::size_t local_ants_end = local_ants_start + local_ants_count;
    ants_vect ants;
    auto gen_ant_pos = [&land, &seed] () { return rand_int32(0, land.dimensions()-1, seed); };
    for ( size_t i = 0; i < nb_ants; ++i ) {
        position_t pos{gen_ant_pos(), gen_ant_pos()};
        if (i >= local_ants_start && i < local_ants_end) {
            ants.add_ant(pos, seed);
        }
    }
    // On crée toutes les fourmis dans la fourmilière.
    pheronome phen(land.dimensions(), pos_food, pos_nest, alpha, beta);

    Window* win = nullptr;
    Renderer* renderer = nullptr;
    if (rank == 0){
        win = new Window("Ant Simulation (MPI version)", 2*land.dimensions()+10, land.dimensions()+266);
        renderer = new Renderer( land, phen, pos_nest, pos_food, ants );
    }
    // Compteur de la quantité de nourriture apportée au nid par les fourmis
    size_t local_food_quantity = 0;
    size_t total_food_quantity = 0;
    
    
    SDL_Event event;
    bool cont_loop = true;
    bool not_food_in_nest = true;
    std::size_t it = 0;

    T = {0., 0., 0., 0., 0., 0., 0.};

    while (cont_loop) {
        ++it;
        if (rank == 0){
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT)
                    cont_loop = false;
            }
        }

        int cont = cont_loop ? 1 : 0;
        MPI_Bcast(&cont, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (!cont) break;

        auto t0 = std::chrono::high_resolution_clock::now();

        double ants_move_before = T.ants_move;
        double evap_before = T.evaporation;
        double update_before = T.pheromones_update;
        double comms_before = T.mpi_comms;


        advance_time( land, phen, pos_nest, pos_food, ants, local_food_quantity, rank, size);
        auto t_reduce = std::chrono::high_resolution_clock::now();
        MPI_Reduce(&local_food_quantity, &total_food_quantity, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
        auto t1 = std::chrono::high_resolution_clock::now();

        if (rank == 0){
            renderer->display( *win, total_food_quantity );
            win->blit();
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        
        T.compute += std::chrono::duration<double>(t1-t0).count();  //double donc renvoie des secondes
        T.render += std::chrono::duration<double>(t2-t1).count();
        T.total += std::chrono::duration<double>(t2-t0).count();
        T.mpi_comms += std::chrono::duration<double>(t1 - t_reduce).count();

        if (rank == 0){
            double current_ants_move = T.ants_move - ants_move_before;  
            double current_evap = T.evaporation - evap_before; 
            double current_update = T.pheromones_update - update_before;
            double current_comms = T.mpi_comms - comms_before;
            double current_compute = std::chrono::duration<double>(t1-t0).count();
            double current_render = std::chrono::duration<double>(t2-t1).count();



        if (it % 100 == 0) {
                std::cout << "\n=== Iteration " << it << " ===" << std::endl;
                std::cout << "[Temps de l'iteration courante]" << std::endl;
                std::cout << "  Déplacement fourmis : " << current_ants_move * 1000 << " ms" << std::endl;
                std::cout << "  Évaporation MPI     : " << current_evap * 1000 << " ms" << std::endl;
                std::cout << "  Mise à jour phéro   : " << current_update * 1000 << " ms" << std::endl;
                std::cout << "  Comms MPI (Sync)    : " << current_comms * 1000 << " ms" << std::endl;
                std::cout << "  Temps Calcul        : " << current_compute * 1000 << " ms" << std::endl;
                std::cout << "  Temps rendu         : " << current_render * 1000 << " ms" << std::endl;
                std::cout << "  Temps total         : " << std::chrono::duration<double>(t2-t0).count() * 1000 << " ms" << std::endl;

                std::cout << "\n[Temps moyen]" << std::endl;
                std::cout << "  Déplacement fourmis : " << (T.ants_move / it) * 1000 << " ms" << std::endl;
                std::cout << "  Évaporation MPI     : " << (T.evaporation / it) * 1000 << " ms" << std::endl;
                std::cout << "  Mise à jour phéro   : " << (T.pheromones_update / it) * 1000 << " ms" << std::endl;
                std::cout << "  Comms MPI (Sync)    : " << (T.mpi_comms / it) * 1000 << " ms" << std::endl;
                std::cout << "  Temps Calcul        : " << (T.compute / it) * 1000 << " ms" << std::endl;
                std::cout << "  Temps rendu         : " << (T.render / it) * 1000 << " ms" << std::endl;
                std::cout << "  Temps total         : " << (T.total / it) * 1000 << " ms" << std::endl;
                std::cout << "========================\n" << std::endl;
            }


            if ( not_food_in_nest && total_food_quantity > 0 ) {
                std::cout << "La première nourriture est arrivée au nid a l'iteration " << it << " après " << T.total << " secondes." << std::endl;
                not_food_in_nest = false;
            }
            //SDL_Delay(10);
        }
    }
    SDL_Quit();
    return 0;
}