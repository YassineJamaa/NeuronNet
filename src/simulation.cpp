#include "globals.h"
#include "random.h"
#include "simulation.h"

Simulation::Simulation(int argc, char **argv) {
    parse(argc, argv);
}

void Simulation::parse(int argc, char **argv) {
    intervalConstr allowedProperty;
    TCLAP::CmdLine cmd(_PRGRM_TEXT_);
    TCLAP::ValueArg<int> sizeArg("N", "number", _SIZE_TEXT_, false, _SIZE_, "int");
    cmd.add(sizeArg);
    TCLAP::ValueArg<int> timeArg("t", "time", _TIME_TEXT_,  false, _TIME_, "int");
    cmd.add(timeArg);
    TCLAP::ValueArg<double> degreeArg("d", "degree", _DEGREE_TEXT_,  false, _DEGREE_, "double");
    cmd.add(degreeArg);
    TCLAP::ValueArg<double> inhibArg("i", "inhibitory", _INHIB_TEXT_,  false, -1, &allowedProperty);
    cmd.add(inhibArg);
    TCLAP::ValueArg<std::string> typesArg("T", "neurontypes", _TYPES_TEXT_,  false, "", "string");
    cmd.add(typesArg);
    TCLAP::ValueArg<std::string> outputArg("o", "output", _OUTPUT_TEXT_,  false, "", "string");
    cmd.add(outputArg);
    TCLAP::ValueArg<double> strengthArg("s", "strength", _STRENG_TEXT_,  false, _STRENG_, "double");
    cmd.add(strengthArg);
    TCLAP::ValueArg<double> thalamArg("n", "thalamic", _THALAM_TEXT_,  false, _THALAM_, "double");
    cmd.add(thalamArg);
    TCLAP::ValueArg<std::string> cfile("c", "config", _CFILE_TEXT_, false, "", "string");
    cmd.add(cfile);

    cmd.parse(argc, argv);

    endtime = timeArg.getValue();
    size = sizeArg.getValue();
    degree = degreeArg.getValue();
    output = outputArg.getValue();
    streng = strengthArg.getValue();
    thalam = thalamArg.getValue();
    inhib = inhibArg.getValue();
    if (inhib<=0. || inhib>1.) inhib = _PROP_INHIB_;
    std::string conf(cfile.getValue()), types(typesArg.getValue());
    if (conf.empty()) {
        net.resize(size, inhib);
        parse_types(types);
    } else load_configuration(conf);
}

void Simulation::parse_types(std::string types) {
    ntypes.clear();
    if (types.empty()) {
        size_t nfs(inhib*size+0.5);
        ntypes["FS"] = nfs;
    } else {
        types.erase(std::remove_if(types.begin(), types.end(), isspace), types.end());
        std::string label;
        std::stringstream ss(types);
        size_t total = 0;
        for (std::string item; total<size && std::getline(ss, item, ','); ) {
            size_t split = item.find(':');
            label = item.substr(0, split);
            double prop = std::min(std::stod(item.substr(split+1)), 1.0);
            if (prop>0 && label!="RS" && Neuron::type_exists(label)) {
                size_t nums(prop*size+0.5);
                if (total+nums>size) nums = size-total;
                ntypes[label] = nums;
                total += nums;
            }
        }
    }
    net.set_default_params(ntypes);
    net.random_connect(degree, streng);
}

size_t Simulation::size_type(const std::string &_s) const {
    if (ntypes.count(_s)) return ntypes.at(_s);
    if (_s != "RS")       return 0;
    size_t nrs = size;
    for (auto I : ntypes) nrs -= I.second;
    return nrs;
}

void Simulation::load_configuration(const std::string &infile) {
    ntypes.clear();
    std::vector<NeuronParams> neurons;
    std::vector<std::string> neurtyp;
    std::map<size_t, size_t> indexmap;
    linkmap linklist;
    std::vector<double> npoten;
    size = 0;
    try {
        std::ifstream confstr(infile);
        std::string item, key, line;
        double value, poten;
        size_t nindex;
        NeuronParams nparams;
        if (confstr.is_open()) {
            while (std::getline(confstr, line)) {
                // This removes all spaces from the string
                line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
                // comment lines start with #
                if (line[0] == '#') continue;
                std::stringstream ss(line);
                std::getline(ss, item, ';'); 
                key = item.substr(0, 4);
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                if (key.compare("link") == 0) {
                    while (std::getline(ss, item, ';')) {
                        size_t split = item.find(','),
                            split2 = item.find(':');
                        size_t from = stoi(item.substr(0, split)), 
                            to = stoi(item.substr(split+1, split2));
                        double str = stod(item.substr(split2+1));
                        linklist.insert({{from, to}, str});
                    }
                } else {
                    nindex = stoi(item);
                    std::getline(ss, item, ';');
                    nparams = Neuron::type_default(item);
                    neurtyp.push_back(item);
                    ntypes[item]++;
                    poten = _REST_VAL_;
                    while (std::getline(ss, item, ';')) {
                        size_t split = item.find('=');
                        key = item.substr(0, split);
                        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                        value = stod(item.substr(split+1));
                        if      (key[0] == 'a') nparams.a = value;
                        else if (key[0] == 'b') nparams.b = value;
                        else if (key[0] == 'c') nparams.c = value;
                        else if (key[0] == 'd') nparams.d = value;
                        else if (key[0] == 'i') nparams.inhib = (value>0);
                        else if (key[0] == 'v') poten = value;
                    }
                    indexmap[nindex] = size;
                    neurons.push_back(nparams);
                    npoten.push_back(poten);
                    size++;
                }
            }
            confstr.close();
        } else throw(CFILE_ERROR("Could not open configuration file " + infile));
    } catch(std::ifstream::failure &e) {
        throw(CFILE_ERROR("Error with configuration file " + infile + ": " + e.what()));
    }
    net.resize(size, inhib);
    net.set_types_params(neurtyp, neurons);
    net.set_values(npoten);
    if (linklist.empty()) net.random_connect(degree, streng);
    else for (auto I : linklist) 
             net.add_link(indexmap[I.first.first], indexmap[I.first.second], I.second);
}

void Simulation::run() {
	
    std::ofstream outf(output), outf2, outf3;
    if (output.size() && outf.bad()) 
        throw(OUTPUT_ERROR(std::string("Cannot write to file ")+output));
    std::ostream *_outf = &std::cout;
    if (outf.is_open()) _outf = &outf;
    if (output.size()) {
        outf2.open(output+"_traj");
        if (outf2.bad())
            throw(OUTPUT_ERROR(std::string("Cannot write to file ")+output+"_traj"));
        outf3.open(output+"_pars");
        if (outf3.bad())
            throw(OUTPUT_ERROR(std::string("Cannot write to file ")+output+"_pars"));
    }
    std::vector<double> thalinput(size);
    net.print_params(&outf3);
    if (outf3.is_open()) outf3.close();
    if (outf2.is_open()) net.print_head(ntypes, &outf2);
    int time = 0;
    while (time<endtime) {
        _RNG->normal(thalinput, 0, thalam);
        std::set<size_t> firs = net.step(thalinput);
        time++;
        (*_outf) << time;
        for (size_t nn=0; nn<size; nn++) (*_outf) << " " << firs.count(nn);
        (*_outf) << std::endl;
        if (outf2.is_open()) net.print_traj(time, ntypes, &outf2);
    }
    if (outf2.is_open()) outf2.close();
    if (outf.is_open()) outf.close();        
}


