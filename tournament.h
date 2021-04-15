#pragma once

#include <random>

#include "nondominated_sort.h"

template <typename T, typename T2, typename RNG>
class Tournament {
  const std::vector<T> &m_population;
  const std::vector<T2> &m_evaluations;
  const std::vector<float> &m_crowding_distances;

  RNG &m_rng;

public:
  Tournament(const std::vector<T> &population, const std::vector<T2> &evaluations, const std::vector<float> &crowding_distances, RNG &rng):
      m_population(population),
      m_evaluations(evaluations),
      m_crowding_distances(crowding_distances),
      m_rng(rng) {}

  const T &fight(size_t a, size_t b) {
    auto ordering = dominates(m_evaluations[a], m_evaluations[b]);

    if (ordering != 0) {
      return ordering > 0 ? m_population[a] : m_population[b];
    }

    if (m_crowding_distances[a] != m_crowding_distances[b]) {
      return m_crowding_distances[a] > m_crowding_distances[b] ? m_population[a] : m_population[b];
    }

    std::uniform_real_distribution<float> perc(0.0, 1.0);
    return perc(m_rng) < 0.5f ? m_population[a] : m_population[b];
  }
};
