#pragma once

#include <list>
#include <vector>
#include <numeric>

template <typename T>
std::strong_ordering dominates(const T &a, const T &b) {
  bool adb = 0;
  bool bda = 0;

  for (size_t i = 0; i < T().size(); i++) {
    adb |= a[i] < b[i];
    bda |= a[i] > b[i];
  }

  return adb <=> bda;
}


template <typename T>
std::vector<size_t> get_front(const std::vector<T> &evaluations, std::list<size_t> &orig) {
  std::list<size_t> front {};

  for (auto it1 = std::begin(orig); it1 != std::end(orig); it1++) {
    bool dominated = false;

    for (auto it2 = std::begin(front); it2 != std::end(front); it2++) {
      auto ordering = dominates(evaluations[*it1], evaluations[*it2]);

      if (ordering > 0) {
        orig.push_front(*it2);
        it2 = front.erase(it2);
        it2--;
      }
      else if (ordering < 0) {
        dominated = true;
        break;
      }
    }

    if (!dominated) {
      front.push_front(*it1);
      it1 = orig.erase(it1);
      it1--;
    }
  }

  return { std::begin(front), std::end(front) };
}

template <typename T>
void assign_crowding_distance(const std::vector<T> &evaluations, std::vector<float> &crowding_distances, std::vector<size_t> idxs /* working copy */) {
  for (size_t j = 0; j < idxs.size(); j++) {
    crowding_distances[idxs[j]] = 0.0;
  }

  for (size_t i = 0; i < T().size(); i++) {
    std::sort(std::begin(idxs), std::end(idxs), [&](const auto &a, const auto &b) {
      return evaluations[a][i] < evaluations[b][i];
    });

    crowding_distances[idxs[0]]               = std::numeric_limits<float>::infinity();
    crowding_distances[idxs[idxs.size() - 1]] = std::numeric_limits<float>::infinity();

    float evaluation_factor = 1.f / (evaluations[idxs[idxs.size() - 1]][i] - evaluations[idxs[0]][i]);

    for (size_t j = 1; j < idxs.size() - 1; j++) {
      crowding_distances[idxs[j]] += (evaluations[idxs[j + 1]][i] - evaluations[idxs[j - 1]][i]) * evaluation_factor;
    }
  }
}


template <typename T>
void assign_crowding_distances(const std::vector<T> &evaluations, std::vector<float> &crowding_distances) {
  std::list<size_t> orig(evaluations.size());
  std::iota(std::begin(orig), std::end(orig), 0);

  while (!orig.empty()) {
    assign_crowding_distance(evaluations, crowding_distances, get_front(evaluations, orig));
  }
}

template <typename T, typename F>
void get_better_half(const std::vector<T> &mixed_evaluations, std::vector<float> &mixed_crowding_distances, F &&callback) {
  std::list<size_t> orig(mixed_evaluations.size());
  std::iota(std::begin(orig), std::end(orig), 0);

  const size_t half_size = mixed_evaluations.size() / 2;

  size_t newpop_idx = 0;
  while (newpop_idx < mixed_evaluations.size() / 2) {
    std::vector<size_t> front = get_front(mixed_evaluations, orig);

    assign_crowding_distance(mixed_evaluations, mixed_crowding_distances, front);

    if (newpop_idx + front.size() > half_size) {
      std::sort(std::begin(front), std::end(front), [&](const auto &a, const auto &b) {
        return mixed_crowding_distances[a] > mixed_crowding_distances[b];
      });
    }

    for (auto it = std::begin(front); it != std::end(front) && newpop_idx < half_size; it++) {
      callback(*it);
      newpop_idx++;
    }
  }
}
