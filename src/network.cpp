#include "network.h"
#include "random.h"

void Network::resize(const size_t &n, double inhib) {
    size_t old = size();
    neurons.resize(n);
    if (n <= old) return;
    size_t nfs(inhib*(n-old)+.5);
    set_default_params({{"FS", nfs}}, old);
}

void Network::set_default_params(const std::map<std::string, size_t> &types,
                                 const size_t start) {
    size_t k(0), ssize(size()-start), kmax(0);
    std::vector<double> noise(ssize);
    _RNG->uniform_double(noise);
    for (auto I : types) 
        if (Neuron::type_exists(I.first)) 
            for (kmax+=I.second; k<kmax && k<ssize; k++) 
                neurons[start+k].set_default_params(I.first, noise[k]);
    for (; k<ssize; k++) neurons[start+k].set_default_params("RS", noise[k]);
}

void Network::set_types_params(const std::vector<std::string> &_types,
                               const std::vector<NeuronParams> &_par,
                               const size_t start) {
    for (size_t k=0; k<_par.size(); k++) {
        neurons[start+k].set_type(_types[k]);
        neurons[start+k].set_params(_par[k]);
    }
}

void Network::set_values(const std::vector<double> &_poten, const size_t start) {
    for (size_t k=0; k<_poten.size(); k++) 
        neurons[start+k].potential(_poten[k]);
}

bool Network::add_link(const size_t &a, const size_t &b, double str) {
    if (a==b || a>=size() || b>=size() || str<1e-6) return false;
    if (links.count({a,b})) return false;
    if (neurons[b].is_inhibitory()) str *= -2.0;
    links.insert({{a,b}, str});
    return true;
}

size_t Network::random_connect(const double &mean_deg, const double &mean_streng) {
    links.clear();
    std::vector<int> degrees(size());
    _RNG->poisson(degrees, mean_deg);
    size_t num_links = 0;
    std::vector<size_t> nodeidx(size());
    std::iota(nodeidx.begin(), nodeidx.end(), 0);
    for (size_t node=0; node<size(); node++) {
        _RNG->shuffle(nodeidx);
        std::vector<double> strength(degrees[node]);
        _RNG->uniform_double(strength, 1e-6, 2*mean_streng);
        int nl = 0;
        for (size_t nn=0; nn<size() && nl<degrees[node]; nn++)
            if (add_link(node, nodeidx[nn], strength[nl])) nl++;
        num_links += nl;
    }
    return num_links; 
}

std::vector<double> Network::potentials() const {
    std::vector<double> vals;
    for (size_t nn=0; nn<size(); nn++)
        vals.push_back(neurons[nn].potential());
    return vals;
}

std::vector<double> Network::recoveries() const {
    std::vector<double> vals;
    for (size_t nn=0; nn<size(); nn++)
        vals.push_back(neurons[nn].recovery());
    return vals;
}

void Network::print_params(std::ostream *_out) {
    (*_out) << "Type\ta\tb\tc\td\tInhibitory\tdegree\tvalence" << std::endl;
    for (size_t nn=0; nn<size(); nn++) {
        std::pair<size_t, double> dI = degree(nn);
        (*_out) << neurons[nn].formatted_params() 
                << '\t' << dI.first << '\t' << dI.second
                << std::endl;
    }
}

void Network::print_head(const std::map<std::string, size_t> &_nt, 
                         std::ostream *_out) {
    size_t total = 0;
    for (auto It : _nt) {
        total += It.second;
        for (auto In : neurons)
            if (In.is_type(It.first)) {
                (*_out) << '\t' << It.first << ".v"
                        << '\t' << It.first << ".u"
                        << '\t' << It.first << ".I";
                break;
            }
    }
    if (total<size())
        for (auto In : neurons) 
            if (In.is_type("RS")) {
                (*_out) << '\t' << "RS.v" << '\t' << "RS.u" << '\t' << "RS.I";
                break;
            }
    (*_out) << std::endl;
}

void Network::print_traj(const int time, const std::map<std::string, size_t> &_nt, 
                         std::ostream *_out) {
    (*_out)  << time;
    size_t total = 0;
    for (auto It : _nt) {
        total += It.second;
        for (auto In : neurons) 
            if (In.is_type(It.first)) {
                (*_out) << '\t' << In.formatted_values();
                break;
            }
    }
    if (total<size())
        for (auto In : neurons) 
            if (In.is_type("RS")) {
                (*_out) << '\t' << In.formatted_values();
                break;
            }
    (*_out) << std::endl;
}



std::vector<std::pair<size_t, double> > Network::neighbors(const size_t& k) const{ //ajout
	std::vector<std::pair<size_t, double> > resultat;
	for (linkmap::const_iterator it =links.lower_bound({k,0}); it!=links.cend() and ((it->first).first == k); ++it){
		resultat.push_back( std::make_pair((it->first).second, it->second));	
	}
	return resultat;	
}
 

std::pair<size_t, double> Network::degree(const size_t &k) const{ //ajout
	std::vector<std::pair<size_t, double> > signal_total( neighbors(k));
	double tot(0.0);
	size_t tot_neuron(0);
	for(size_t i(0); i<signal_total.size() ; i++){//anotation
		tot += signal_total[i].second;
		++tot_neuron;
	}
	std::pair <size_t,double> degree_neuron( std::make_pair( tot_neuron, tot));
	return degree_neuron;
}

std::set<size_t> Network::step(const std::vector<double>& j){
	std::set<size_t> resultat;
	double w;
	double excitateur, inhibiteur;
	for(size_t i(0) ; i < neurons.size() ; ++i){
			if(neurons[i].firing()){ 
				neurons[i].reset();
				resultat.insert(i);
		}
	}
	for(size_t t(0) ; t < neurons.size() ; ++t){
		excitateur = 0.0;
		inhibiteur = 0.0;
		if(neurons[t].is_inhibitory()){
			w= 0.4;
		}else{
			w=1;
		}
		std::vector<std::pair<size_t, double> > voisin = neighbors(t);
		for( size_t h(0) ; h< voisin.size() ; ++h){
			if(neurons[voisin[h].first].is_inhibitory() and resultat.count(h) == 1 ){
				inhibiteur += voisin[h].second;
			}
			if(!neurons[voisin[h].first].is_inhibitory() and resultat.count(h) == 1 ){
			
				excitateur += voisin[h].second;
			}
				
		}
		neurons[t].input(w*j[t] + 0.5*excitateur + inhibiteur);
		neurons[t].step();
	}
		
	
	return resultat;
}


