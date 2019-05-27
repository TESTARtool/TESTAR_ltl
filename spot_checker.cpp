#include <iostream>
#include <string>
#include <spot/tl/parse.hh>
#include <spot/tl/print.hh>
#include <spot/misc/version.hh>
#include <spot/parseaut/public.hh>
#include <spot/twaalgos/translate.hh>
#include <spot/twaalgos/emptiness.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twa/bddprint.hh>
#include <chrono>
#include <ctime>
#include <sstream>
#include <fstream>
#include<experimental/filesystem>
namespace fs = std::experimental::filesystem;
#include <unistd.h>






// Globals
const std::string resultfilename = "propertylog.txt";
const std::string modelfilename = "model.txt";
std::chrono::system_clock::time_point clock_start, clock_end;
std::string automaton_filename;
std::string formulafilename;



spot::parsed_aut_ptr pa;
spot::bdd_dict_ptr bdd;
double vm, rss;


void setup_spot(){
    bdd = spot::make_bdd_dict();
}


std::string getCurrentLocalTime(){
    time_t curr_time;
    tm * curr_tm;
    char date_timestring[50];
    time(&curr_time);
    curr_tm = localtime(&curr_time);
    strftime(date_timestring, 50, "%c", curr_tm);
    return date_timestring;

}

void process_mem_usage(double& vm_usage, double& resident_set)
// from https://gist.github.com/thirdwing/da4621eb163a886a03c5
{
    vm_usage     = 0.0;
    resident_set = 0.0;

    // the two fields we want
    unsigned long vsize;
    long rss;
    {
        std::string ignore;
        std::ifstream ifs("/proc/self/stat", std::ios_base::in);
        ifs >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
            >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
            >> ignore >> ignore >> vsize >> rss;
    }

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    vm_usage = vsize / 1024.0;
    resident_set = rss * page_size_kb;
}



void copyAutomatonFile(std::istream& autin, std::string copyTofilename){
    std::ofstream aut_file;
    std::string aut_line;
    std::remove(copyTofilename.c_str());
    aut_file.open(copyTofilename.c_str());
    while (  getline(autin , aut_line)) {
        aut_file << aut_line << std::endl;
        if (aut_line == "EOF_HOA") {
            aut_file.close();
            break;
        }
    }
}
std::string  load_automaton( const std::string& hoafile)
{
    pa = parse_aut(hoafile, bdd);
    if (pa->format_errors(std::cerr))
        return "--syntax error while reading automaton input file-- \n";
    if (pa->aborted) // following can only occur when reading  a HOA file.
        return"--error ABORT found in the HOA file -- \n";
    return "";
}

std::string log_elapsedtime(){
    clock_end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = clock_end-clock_start;
    return "< elapsed time: " + std::to_string(elapsed_seconds.count()) + "s >";
}

void custom_print(std::ostream& out, spot::twa_graph_ptr& aut, int verbosity );   //declare before

std::string getAutomatonTitle(spot::twa_graph_ptr& aut){
    auto name = aut->get_named_prop<std::string>("automaton-name");
    if (name!= nullptr){
        return *name;
    }
    else {
        return "";
    }
}
std::string check_property( std::string formula, spot::twa_graph_ptr& aut) {

    std::ostringstream sout;  //needed for capturing output of run.
    process_mem_usage(vm, rss);
    sout << "=== start checking : '" << formula << "' on automaton '"<<getAutomatonTitle(aut) <<"' === "
    <<log_elapsedtime()<< " [Memory: "<< "VM: " << vm << "; RSS: " << rss <<"]\n";
    //spot::formula f = spot::parse_formula(formula);
    spot::parsed_formula pf =spot::parse_infix_psl(formula);
    spot::formula f=pf.f;
    if (!pf.errors.empty()){
        sout << "ERROR, syntax error while parsing formula.\n";
    }
    else {
        //check if ap's are in the automaton.
        spot::bdd_dict_ptr fbdd = spot::make_bdd_dict();
        spot::twa_graph_ptr aftemp = spot::translator(fbdd).run(f);
        std::vector<spot::formula> v = aut->ap();
        bool apmismatch = false;
        for (spot::formula ap: aftemp->ap())
            if (std::find(v.begin(), v.end(), ap) != v.end()) {} //exists?
            else {
                apmismatch = true;
                break;
            }
        if (apmismatch) {
            sout << "ERROR, atomic propositions in formula are not in automaton.\n";
        }
        else {
            spot::formula nf = spot::formula::Not(f);
            spot::twa_graph_ptr af = spot::translator(bdd).run(nf);
            custom_print(std::cout, af, 1);
            spot::twa_run_ptr run;
            run = aut->intersecting_run(af);
            if (run) {
                sout << "FAIL, with counterexample:  \n" << *run; //needs emptiness.hh
            } else {
                af = spot::translator(bdd).run(f);
                run = aut->intersecting_run(af);
                sout << "PASS, with witness: \n" << *run;
            }
        }
    }
    process_mem_usage(vm, rss);
    sout << "=== end checking : '" << formula <<  "' on automaton '"<<getAutomatonTitle(aut) <<"' === "
    <<log_elapsedtime()<< " [Memory: "<< "VM: " << vm << "; RSS: " << rss <<"]\n";
    return sout.str();
}

void check_collection(std::istream& col_in, std::ostream& out, std::string results_filename){

    std::ofstream results_file;
    std::string formula_result;
    std::string f;
    if (fs::exists(results_filename.c_str()))     std::remove(results_filename.c_str());
    results_file.open(results_filename.c_str());
    while (getline(col_in, f)) {
        if (f == "") break;
        formula_result= check_property(f, pa->aut);
        out << formula_result;
        results_file << formula_result  ;
    }
    results_file.close();

}

//*********************************
void custom_print(std::ostream& out, spot::twa_graph_ptr& aut, int verbosity = 0)
{
    // We need the dictionary to print the BDDs that label the edges
    const spot::bdd_dict_ptr& dict = aut->get_dict();
    std::cout << "Automaton properties:\n";
    // Some meta-data...
    out << "Acceptance: " << aut->get_acceptance() << '\n';
    out << "Number of sets: " << aut->num_sets() << '\n';
    out << "Number of states: " << aut->num_states() << '\n';
    out << "Number of edges: " << aut->num_edges() << '\n';
    out << "Initial state: " << aut->get_init_state_number() << '\n';
    out << "Atomic propositions:";
    for (spot::formula ap: aut->ap())
        out << ' ' << ap << " (=" << dict->varnum(ap) << ')';
    out << '\n';

    // Arbitrary data can be attached to automata, by giving them
    // a type and a name.  The HOA parser and printer both use the
    // "automaton-name" to name the automaton.
    if (auto name = aut->get_named_prop<std::string>("automaton-name"))
        out << "Name: " << *name << '\n';

    // For the following prop_*() methods, the return value is an
    // instance of the spot::trival class that can represent
    // yes/maybe/no.  These properties correspond to bits stored in the
    // automaton, so they can be queried in constant time.  They are
    // only set whenever they can be determined at a cheap cost: for
    // instance an algorithm that always produces deterministic automata
    // would set the deterministic property on its output.  In this
    // example, the properties that are set come from the "properties:"
    // line of the input file.
    out << "Complete: " << aut->prop_complete() << '\n';
    out << "Deterministic: " << (aut->prop_universal()
                                 && aut->is_existential()) << '\n';
    out << "Unambiguous: " << aut->prop_unambiguous() << '\n';
    out << "State-Based Acc: " << aut->prop_state_acc() << '\n';
    out << "Terminal: " << aut->prop_terminal() << '\n';
    out << "Weak: " << aut->prop_weak() << '\n';
    out << "Inherently Weak: " << aut->prop_inherently_weak() << '\n';
    out << "Stutter Invariant: " << aut->prop_stutter_invariant() << '\n';
    if (verbosity!=0) {
        // States are numbered from 0 to n-1
        unsigned n = aut->num_states();
        for (unsigned s = 0; s < n; ++s) {
            out << "State " << s << ":\n";

            // The out(s) method returns a fake container that can be
            // iterated over as if the contents was the edges going
            // out of s.  Each of these edges is a quadruplet
            // (src,dst,cond,acc).  Note that because this returns
            // a reference, the edge can also be modified.
            for (auto &t: aut->out(s)) {
                out << "  edge(" << t.src << " -> " << t.dst << ")\n    label = ";
                spot::bdd_print_formula(out, dict, t.cond);
                out << "\n    acc sets = " << t.acc << '\n';
            }
        }
    }
}
void print_usage(std::ostream& out){
    out << "Usage of the program :  spot_checker --stdin --a <file> --f <formula> --ff <file> \n";
    out << "commandline options:\n";
    out << "--stdin   all input is  via standard input stream: first an automaton followed by formulas. \n";
    out << "          all other arguments are ignored.";
    out << "--a       mandatory unless --stdin is the argument. filename containing the automaton (HOA format). \n";
    out << "--f       optional.  the LTL formula/property to check.  \n";
    out << "--ff      optional.  filename containing multiple formulas/properties.) \n\n";
    out << "without a cli formula, the user can supply via stdin a formula/property.) \n";
    out << "the results are returned via stdout and the system will ask for a new formula.) \n";
    out << "a blank line will stop the program.) \n";
}
//*************************************

int main(int argc, char *argv[])

{
    // do stuff;
    std:: string formula;
    process_mem_usage(vm, rss);
    clock_start = std::chrono::system_clock::now();
    std::cout << "Start of LTL model-check. === "<<getCurrentLocalTime()<<" [Memory: "<< "VM: " << vm << "; RSS: " << rss <<"]"<<"\n" ;

    switch(argc) {
        case 2 :
            if(std::string(argv[1])== "--stdin")
                automaton_filename=""; //empty implies: stdin must be read
            else{
                std::cerr << "single option is not '--stdin'.\n";
                print_usage(std::cerr);
                return 1;
            }
            break;
        case 3 :
            if(std::string(argv[1])== "--a")
                    automaton_filename=std::string(argv[2]);
            else{
                    std::cerr << "first option is not '--a'.\n";
                    print_usage(std::cerr);
                    return 1;
            }
            break;
        case 5 :
            if(std::string(argv[1])== "--a")
                automaton_filename=std::string(argv[2]);
            else{
                std::cerr << "first option is not '--a'.\n";
                print_usage(std::cerr);
                return 1;
            }
            if(std::string(argv[3]) == "--f")
                    formula=std::string(argv[4]);
            else if (std::string(argv[3])== "--ff")
                if (fs::exists(argv[4]))
                    formulafilename=std::string(argv[4]);
                else {
                    std::cerr << "formula file not found for option '--ff'.\n";
                    print_usage(std::cerr);
                    return 1;
                }
                 else {
                std::cerr << "second option  is not '--f or --ff'.\n";
                print_usage(std::cerr);
                return 1;
            }
            break;
        default :
            print_usage(std::cerr);
            return 1;
    }



    setup_spot();
    if (automaton_filename==""){
        copyAutomatonFile(std::cin,modelfilename);
        automaton_filename=  modelfilename;
    }
    else if (automaton_filename!=modelfilename){// copy only if different from the default
        std::ifstream infile;
        infile.open(automaton_filename.c_str());
        copyAutomatonFile(infile,modelfilename);
    }
    std::string res  = load_automaton(automaton_filename);
    if (res=="") {
        custom_print(std::cout, pa->aut, 0);
        std::string auttitle = getAutomatonTitle(pa->aut);
        process_mem_usage(vm, rss);
        std::cout << "Automaton '" << auttitle << "' loaded === " << log_elapsedtime() << " [Memory: " << "VM: " << vm
                  << "; RSS: " << rss << "]\n";
        if (formulafilename != "") {
            std::ifstream infile;
            infile.open(formulafilename.c_str());
            check_collection(infile, std::cout, resultfilename);
            std::cout << "Formula file  '" << formulafilename << "' loaded === " << log_elapsedtime() << " [Memory: "
                      << "VM: " << vm << "; RSS: " << rss << "]\n";
        } else if (formula != "") {
            std::istringstream s_in;
            s_in.str(formula);
            check_collection(s_in, std::cout, resultfilename);
        } else
            check_collection(std::cin, std::cout, resultfilename);
    }
    else{
        std::cout<<res<<"\n";
    }
    process_mem_usage(vm, rss);
    std::cout <<"End of LTL model-check. === "<< log_elapsedtime()<< " [Memory: "<< "VM: " << vm << "; RSS: " << rss <<"]\n";
    return 0;
}