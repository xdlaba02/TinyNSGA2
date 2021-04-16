# TinyNSGA2
TinyNSGA2 is a header-only C++11 implementation of NSGA2 multi-objective genetic algorithm [1]. The implementation is focused on simple and universal API, minimal memory usage with no dynamic allocations after initialization, and, last but not least, code readability. The implementation is based on the STL library. If this is not optimal for your project, feel free to modify it for your needs, but please leave copyright there. Template metaprogramming is used, so be aware.

## Usage
Include the `tinynsga2.h` file in your project.

Create a type that represents your individual. It can be whatever you want to evolve. I will use `IndiviualType` for the example.
Determine the number of criteria on which you will optimize. I will use `N` for the example.

You will need to define four evolution operator functions. The name of those functions doesn't matter, nor the _type_. It can be classic function, lambda, capture, or even functor. Due to the template metaprogramming implementation, the only thing that matters is the interface.

### Individual initialization
```
void init(IndiviualType &individual);
```
Do whatever you want to do with your individual here. You can initialize it randomly, you can create it as capture and increment some outside counter to know where you are. This function will be called at initialization, exactly once per individual.

### Individual evaluation
```
void eval(const IndiviualType &individual, std::array<float, N> &evaluation);
```
Evaluate your individual here and put the values to the array in the second parameter. The algorithm will perform minimization, so try to use a metric where less is better. Or just flip the sign. It's your call.

### Breeding
```
void cross(const IndiviualType &parent1, const IndiviualType &parent2, IndiviualType &child1, IndiviualType &child2);
```
You will need to combine two individuals selected by the algorithm for breeding into two, possibly distinct, children. You can do whatever filthy thing you want with the individuals, I won't tell. Or you can just copy the parents. Again, It's your call.

### Mutation
```
void mutate(IndiviualType &individual);
```
This function will perform mutation. Imagine there was a nuclear power plant accident. Add an eye or two to your individual. Use your imagination. I believe in you.

Besides the operators, you will need a random number generator with `operator()` defined so that it plops out some number. C++ provides some of them in the STL library with the `<random>` header. I will call it `rng` for this example.

With those things settled, you can create an `Evolver`, the class that manages the population and evolves it using your operators:
```
auto nsga2Evolver = TinyNSGA2::create<IndiviualType, N>(init, eval, cross, mutate, rng);
```
You can notice the weird usage. Why we can't call some constructor? Well, it's because it's just too ugly with 7 template parameters:
```
TinyNSGA2::Evolver<IndiviualType, N, decltype(init), decltype(eval), decltype(cross), decltype(mutate), decltype(rng)> nsga2Evolver(init, eval, cross, mutate, rng);
```
Sadly, C++11 does provide CTAD, so I had to solve it somehow.

If you have the evolver created, you can initialize the population:
```
nsga2Evolver.init(population_size);
```
This will create a population with `population_size` individuals and call the `init` function for each of them.
It is safe to init already initialized evolver, for example for different population sizes.

You can then perform evolution iteration(s) with:
```
nsga2Evolver.evolve(iterations);
```
One iteration will be performed when the `iterations` parameter is not specified. This way, you can evolve the population by a fixed number of iterations, but you can also loop the evolution manually.

All of this would be useless if you wouldn't be able to access the individuals outside the operator functions. You can do this with
```
const IndiviualType &individual = nsga2Evolver.individual(idx);`
const std::array<float, N> &evaluation = nsga2Evolver.evaluation(idx);`
```
where valid range of `idx` is from the interval `[0; population_size)` like you would expect in C++.

## License
See [LICENSE](https://github.com/xdlaba02/TinyNSGA2/blob/master/LICENSE) for license and copyright information.

## References
[1] K. Deb, A. Pratap, S. Agarwal, and T. Meyarivan, "A fast and elitist multiobjective genetic algorithm: NSGA-II," in IEEE Transactions on Evolutionary Computation, vol. 6, no. 2, pp. 182-197, April 2002, DOI: 10.1109/4235.996017.
