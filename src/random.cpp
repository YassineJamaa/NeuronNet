#include "random.h"

RandomNumbers::RandomNumbers(unsigned long int s):seed(s){
	 if (seed == 0) {
        std::random_device rd;
        seed = rd();
    }
    rng = std::mt19937(seed);
}

void RandomNumbers::uniform_double(std::vector<double>& res, double lower, double upper){
	std::uniform_real_distribution<> unif(lower, upper);
	for(auto i=res.begin() ; i != res.end() ; i++) *i = unif(rng);
}

double RandomNumbers::uniform_double(double lower, double upper){
	std::uniform_real_distribution<> unif(lower, upper);
	return unif(rng);
}
void RandomNumbers::normal(std::vector<double>& res, double mean, double sd){
	std::normal_distribution<> norm(mean, sd);
	for(auto i=res.begin() ; i != res.end() ; i++) *i = norm(rng);
}
double RandomNumbers::normal(double mean, double sd){
	std::normal_distribution<> norm(mean, sd);
	return norm(rng);;
	
}

void RandomNumbers::poisson(std::vector<int>& res, double mean){
	std::poisson_distribution<> pois(mean);
	for(auto i=res.begin() ; i != res.end(); i++) *i= pois(rng);
}
int RandomNumbers::poisson(double mean){
	std::poisson_distribution<> pois(mean);
	return pois(rng);
	
}




