#pragma once

#include <vector>
#include <cassert>
#include <numeric>

#include "nondominated_sort.h"
#include "tournament.h"

template <typename T, typename ET, typename CT, typename MT, typename RNG>
class TinyNSGA2 {
  using IndividualType = T;
  using EvaluationFunctionType = ET;
  using CrossFunctionType = CT;
  using MutationFunctionType = MT;

  using EvaluationType = typename std::result_of<ET(const T &)>::type;
  using CrowdingDistanceType = float;

  std::vector<IndividualType> &m_population;
  std::vector<EvaluationType> m_evaluations;
  std::vector<CrowdingDistanceType> m_crowding_distances;

  std::vector<IndividualType> m_mixed_population;
  std::vector<EvaluationType> m_mixed_evaluations;
  std::vector<CrowdingDistanceType> m_mixed_crowding_distances;

  std::vector<size_t> m_shuffle1;
  std::vector<size_t> m_shuffle2;

  Tournament<IndividualType, EvaluationType, RNG> m_tournament;

  EvaluationFunctionType &m_evaluationF;
  CrossFunctionType      &m_crossF;
  MutationFunctionType   &m_mutationF;

  RNG &m_rng;

public:
  TinyNSGA2(std::vector<T> &population, ET &&evaluationF, CT &&crossF, MT &&mutationF, RNG &rng):
      m_population(population),
      m_evaluations(population.size()),
      m_crowding_distances(population.size()),
      m_mixed_population(population.size() * 2),
      m_mixed_evaluations(population.size() * 2),
      m_mixed_crowding_distances(population.size() * 2),
      m_shuffle1(population.size()),
      m_shuffle2(population.size()),
      m_tournament(m_population, m_evaluations, m_crowding_distances, rng),
      m_evaluationF(evaluationF),
      m_crossF(crossF),
      m_mutationF(mutationF),
      m_rng(rng) {

    assert(population.size() % 4 == 0);

    for (size_t i = 0; i < std::size(m_population); i++) {
      m_evaluations[i] = m_evaluationF(m_population[i]);
    }

    assign_crowding_distances(m_evaluations, m_crowding_distances);

    std::iota(std::begin(m_shuffle1), std::end(m_shuffle1), 0);
    std::iota(std::begin(m_shuffle2), std::end(m_shuffle2), 0);
  }

  const EvaluationType &evaluation(size_t i) {
    return m_evaluations[i];
  }

  std::vector<size_t> get_front_indices() {
    std::list<size_t> orig(m_population.size());
    std::iota(std::begin(orig), std::end(orig), 0);
    return get_front(m_evaluations, orig);
  }

  void evolve(size_t generations = 1) {
    for (size_t g = 0; g < generations; g++) {
      std::shuffle(std::begin(m_shuffle1), std::end(m_shuffle1), m_rng);
      std::shuffle(std::begin(m_shuffle2), std::end(m_shuffle2), m_rng);

      for (size_t i = 0; i < std::size(m_population); i += 4) {
        {
          const T &parent1 = m_tournament.fight(m_shuffle1[i + 0], m_shuffle1[i + 1]);
          const T &parent2 = m_tournament.fight(m_shuffle1[i + 2], m_shuffle1[i + 3]);
          m_crossF(parent1, parent2, m_mixed_population[i * 2 + 0], m_mixed_population[i * 2 + 1]);
        }

        {
          const T &parent1 = m_tournament.fight(m_shuffle2[i + 0], m_shuffle2[i + 1]);
          const T &parent2 = m_tournament.fight(m_shuffle2[i + 2], m_shuffle2[i + 3]);
          m_crossF(parent1, parent2, m_mixed_population[i * 2 + 2], m_mixed_population[i * 2 + 3]);
        }

        for (size_t j = 0; j < 4; j++) {
          m_mutationF(m_mixed_population[i * 2 + j]);
          m_mixed_evaluations[i * 2 + j] = m_evaluationF(m_mixed_population[i * 2 + j]);

          m_mixed_population[i * 2 + 4 + j]  = m_population[i + j];
          m_mixed_evaluations[i * 2 + 4 + j] = m_evaluations[i + j];
        }
      }

      size_t i = 0;
      get_better_half(m_mixed_evaluations, m_mixed_crowding_distances, [&](size_t idx) {
        m_population[i]         = m_mixed_population[idx];
        m_evaluations[i]        = m_mixed_evaluations[idx];
        m_crowding_distances[i] = m_mixed_crowding_distances[idx];
        i++;
      });
    }
  }
};
