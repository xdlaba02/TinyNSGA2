#pragma once

#include <vector>
#include <array>
#include <numeric>
#include <random>

namespace TinyNSGA2 {
  template <typename T, size_t N, typename IF, typename EF, typename CF, typename MF, typename RNG>
  class Evolver {
    using ET = std::array<float, N>;

    IF  &m_initF;
    EF  &m_evaluationF;
    CF  &m_crossF;
    MF  &m_mutationF;
    RNG &m_rng;

    size_t m_population_size;

    std::vector<T>      m_population;
    std::vector<ET>     m_evaluations;
    std::vector<float>  m_crowding_distances;
    std::vector<size_t> m_indices;

    static std::strong_ordering dominates(const ET &a, const ET &b) {
      bool adb = 0;
      bool bda = 0;

      for (size_t i = 0; i < N; i++) {
        adb |= a[i] < b[i];
        bda |= a[i] > b[i];
      }

      return adb <=> bda;
    }

    std::vector<size_t>::iterator nondominated_sort(std::vector<size_t>::iterator begin, std::vector<size_t>::iterator end) {
      auto front_end = begin;

      for (auto it1 = begin; it1 < end; it1++) {
        bool dominated = false;

        for (auto it2 = begin; it2 < front_end; it2++) {
          auto ordering = dominates(m_evaluations[*it1], m_evaluations[*it2]);

          if (ordering > 0) {
            std::swap(*it2, *--front_end);
          }
          else if (ordering < 0) {
            dominated = true;
            break;
          }
        }

        if (!dominated) {
          std::swap(*it1, *front_end++);
        }
      }

      return front_end;
    }

    void assign_crowding_distance(std::vector<size_t>::iterator begin, std::vector<size_t>::iterator end) {
      for (auto it = begin; it < end; it++) {
        m_crowding_distances[*it] = 0.f;
      }

      for (size_t i = 0; i < N; i++) {
        std::sort(begin, end, [&](const auto &a, const auto &b) {
          return m_evaluations[a][i] < m_evaluations[b][i];
        });

        m_crowding_distances[*begin]     = std::numeric_limits<float>::infinity();
        m_crowding_distances[*(end - 1)] = std::numeric_limits<float>::infinity();

        float evaluation_factor = 1.f / (m_evaluations[*(end - 1)][i] - m_evaluations[*begin][i]);

        for (auto it = begin + 1; it < end - 1; it++) {
          m_crowding_distances[*it] += (m_evaluations[*(it + 1)][i] - m_evaluations[*(it - 1)][i]) * evaluation_factor;
        }
      }
    }

    const T &tournament(size_t a, size_t b) const {
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

  public:
    Evolver(IF &initF, EF &evaluationF, CF &crossF, MF &mutationF, RNG &rng):
        m_initF(initF),
        m_evaluationF(evaluationF),
        m_crossF(crossF),
        m_mutationF(mutationF),
        m_rng(rng) {}

    void init(size_t population_size) {
      m_population_size = population_size;

      m_population.resize(population_size * 2);
      m_evaluations.resize(population_size * 2);
      m_crowding_distances.resize(population_size * 2);
      m_indices.resize(population_size * 2);

      std::iota(std::begin(m_indices), std::end(m_indices), 0);

      for (size_t i = 0; i < population_size; i++) {
        m_initF(m_population[m_indices[i]]);
        m_evaluationF(m_population[m_indices[i]], m_evaluations[m_indices[i]]);
      }

      auto it = std::begin(m_indices);
      while (it < std::begin(m_indices) + population_size) {
        auto front_end = nondominated_sort(it, std::begin(m_indices) + population_size);
        assign_crowding_distance(it, front_end);
        it = front_end;
      }
    }

    void evolve(size_t generations = 1) {
      for (size_t g = 0; g < generations; g++) { // iterates selected number of generations
        for (size_t p = 0; p < 2; p++) { // from every quadruple we select two parents and make two children, so we do it twice to generate new quadruple
          std::shuffle(std::begin(m_indices), std::begin(m_indices) + m_population_size, m_rng); // shuffle the current population for random tournaments

          for (size_t i = 0; i < m_population_size / 4; i++) { // iterate random quadruples in current population
            m_crossF(
              tournament(m_indices[i * 4 + 0], m_indices[i * 4 + 1]),
              tournament(m_indices[i * 4 + 2], m_indices[i * 4 + 3]),
              m_population[m_indices[m_population_size + i * 4 + p * 2 + 0]],
              m_population[m_indices[m_population_size + i * 4 + p * 2 + 1]]
            );
          }

          for (size_t i = m_population_size / 4 * 4; i < m_population_size; i++) { // those last outsiders who could not find a partner will clone themselves
            m_population[m_indices[m_population_size + i]] = m_population[m_indices[i]];
          }
        }

        for (size_t i = 0; i < m_population_size; i++) { // mutate and evaluate new children
          m_mutationF(m_population[m_indices[m_population_size + i]]);
          m_evaluationF(m_population[m_indices[m_population_size + i]], m_evaluations[m_indices[m_population_size + i]]);
        }

        auto it = std::begin(m_indices);
        while (it < std::begin(m_indices) + m_population_size) {
          auto front_end = nondominated_sort(it, std::end(m_indices));

          assign_crowding_distance(it, front_end);

          if (front_end > std::begin(m_indices) + m_population_size) {
            std::sort(it, front_end, [&](const auto &a, const auto &b) {
              return m_crowding_distances[a] > m_crowding_distances[b];
            });
          }

          it = front_end;
        }
      }
    }

    const T &individual(size_t i) {
      return m_population[m_indices[i]];
    }

    const ET &evaluation(size_t i) {
      return m_evaluations[m_indices[i]];
    }
  };

  template <typename T, size_t N, typename IF, typename EF, typename CF, typename MF, typename RNG>
  Evolver<T, N, IF, EF, CF, MF, RNG> create(IF &initF, EF &evaluationF, CF &crossF, MF &mutationF, RNG &rng) {
    return Evolver<T, N, IF, EF, CF, MF, RNG>(initF, evaluationF, crossF, mutationF, rng);
  }
}
