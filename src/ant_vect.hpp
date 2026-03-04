// ant.hpp
#ifndef _ANT_VECT_HPP_
# define _ANT_VECT_HPP_
#include <vector>
# include <utility>
# include "pheronome.hpp"
# include "fractal_land.hpp"
# include "basic_types.hpp"

class ants_vect
{
public:
    /**
     * Une fourmi peut être dans deux états possibles : chargée ( elle porte de la nourriture ) ou non chargée
     */
    enum state { unloaded = 0, loaded = 1 };
    ants_vect() = default;
    ~ants_vect() = default;

    void add_ant(const position_t& pos, std::size_t seed){
        m_states.push_back(unloaded);
        m_positions.push_back(pos);
        m_seeds.push_back(seed);
    }
    std::size_t size() const { return m_positions.size(); }
    bool is_loaded(std::size_t i) const { return m_states[i] == loaded; }
    const position_t& get_position(std::size_t i) const { return m_positions[i]; }
    static void set_exploration_coef (double eps) { m_eps = eps; }

    void advance( pheronome& phen, const fractal_land& land,
                  const position_t& pos_food, const position_t& pos_nest, std::size_t& cpteur_food );

private:
    static double m_eps; // Coefficient d'exploration commun à toutes les fourmis.
    std::vector<std::size_t> m_seeds;
    std::vector<state> m_states;
    std::vector<position_t> m_positions;
};

#endif