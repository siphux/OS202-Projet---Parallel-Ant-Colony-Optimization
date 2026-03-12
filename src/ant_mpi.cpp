#include "ant_mpi.hpp"
#include <iostream>
#include "rand_generator.hpp"

double ants_mpi::m_eps = 0.;

std::size_t ants_mpi::advance_local( pheronome& phen, const fractal_land& land, const position_t& pos_food, const position_t& pos_nest) 
{   
    std::size_t local_food = 0;
    #pragma omp parallel for
    for (std::size_t i = 0; i < m_positions.size(); ++i) {
        auto ant_choice = [this, i]() mutable { return rand_double( 0., 1., this->m_seeds[i] ); };
        auto dir_choice = [this, i]() mutable { return rand_int32( 1, 4, this->m_seeds[i] ); };
        double                                   consumed_time = 0.;
        // Tant que la fourmi peut encore bouger dans le pas de temps imparti
        while ( consumed_time < 1. ) {
            // Si la fourmi est chargée, elle suit les phéromones de deuxième type, sinon ceux du premier.
            int        ind_pher    = ( m_states[i] == loaded ? 1 : 0 );
            double     choix       = ant_choice( );
            position_t old_pos_ant = m_positions[i];
            position_t new_pos_ant = old_pos_ant;
            double max_phen    = std::max( {phen( new_pos_ant.x - 1, new_pos_ant.y )[ind_pher],
                                        phen( new_pos_ant.x + 1, new_pos_ant.y )[ind_pher],
                                        phen( new_pos_ant.x, new_pos_ant.y - 1 )[ind_pher],
                                        phen( new_pos_ant.x, new_pos_ant.y + 1 )[ind_pher]} );
            if ( ( choix > m_eps ) || ( max_phen <= 0. ) ) {
                do {
                    new_pos_ant = old_pos_ant;
                    int d = dir_choice();
                    if ( d==1 ) new_pos_ant.x  -= 1;
                    if ( d==2 ) new_pos_ant.y -= 1;
                    if ( d==3 ) new_pos_ant.x  += 1;
                    if ( d==4 ) new_pos_ant.y += 1;

                } while ( phen[new_pos_ant][ind_pher] == -1 );
            } else {
                // On choisit la case où le phéromone est le plus fort.
                if ( phen( new_pos_ant.x - 1, new_pos_ant.y )[ind_pher] == max_phen )
                    new_pos_ant.x -= 1;
                else if ( phen( new_pos_ant.x + 1, new_pos_ant.y )[ind_pher] == max_phen )
                    new_pos_ant.x += 1;
                else if ( phen( new_pos_ant.x, new_pos_ant.y - 1 )[ind_pher] == max_phen )
                    new_pos_ant.y -= 1;
                else  // if (phen(new_pos_ant.first,new_pos_ant.second+1)[ind_pher] == max_phen)
                    new_pos_ant.y += 1;
            }
            consumed_time += land( new_pos_ant.x, new_pos_ant.y);
            phen.mark_pheronome( new_pos_ant );
            m_positions[i] = new_pos_ant;
            if ( m_positions[i] == pos_nest ) {
                if ( m_states[i] == loaded ) {
                    local_food += 1;
                }
                m_states[i] = unloaded;
            }
            if ( m_positions[i] == pos_food ) {
                m_states[i] = loaded;
            }
        }
    }
    return local_food;
}