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

std::string aut_line;
std::ofstream aut_file;
std::ofstream results_file;



std::chrono::system_clock::time_point clock_start, clock_end;
std::string automatonfile;
std::string result;
std::string formula;
std::string formulafile;
spot::parsed_aut_ptr pa;
spot::bdd_dict_ptr bdd;


void setup_spot(){
    bdd = spot::make_bdd_dict();
}
int  load_automaton(const std::string& hoafile)
{
    pa = parse_aut(hoafile, bdd);
    if (pa->format_errors(std::cerr)) {

        std::cerr << "--syntax error while reading automaton input file-- \n";
        return 1;
    }
    if (pa->aborted) // following can only occur when reading  a HOA file.
    {
        std::cerr << "--error ABORT found in the HOA file -- \n";
        return 1;
    }
    return 0; //custom_print(std::cout, pa->aut);
}

std::string log_elapsedtime(){
    clock_end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = clock_end-clock_start;
    return "elapsed time: " + std::to_string(elapsed_seconds.count()) + "s";
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

    std::ostringstream sout;
    sout << "=== start of checking property: '" << formula << "' on automaton '"<<getAutomatonTitle(aut) <<"' === "<<log_elapsedtime()<<"\n";
    spot::formula f = spot::parse_formula(formula);
    spot::formula nf = spot::formula::Not(f);
    spot::twa_graph_ptr af = spot::translator(bdd).run(nf);
    //custom_print(std::cout, af,1);
    spot::twa_run_ptr run;
    run = aut->intersecting_run(af);
    if (run) {
        sout << "FAIL, with counterexample:  \n" << *run; //needs emptiness.hh
    }
    else {
    af = spot::translator(bdd).run(f);
    run = aut->intersecting_run(af);
    sout << "PASS, with witness: \n" << *run;
    }
    sout << "=== end of checking property: '" << formula <<  "' on automaton '"<<getAutomatonTitle(aut) <<"' === "<<log_elapsedtime()<<"\n";
    return sout.str();
}

//*********************************
void custom_print(std::ostream& out, spot::twa_graph_ptr& aut, int verbosity = 0)
{
    // We need the dictionary to print the BDDs that label the edges
    const spot::bdd_dict_ptr& dict = aut->get_dict();

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
    out << "--ff      not operational.  filename containing multiple formulas/properties.) \n\n";
    out << "without a cli formula, the user can supply via stdin a formula/property.) \n";
    out << "the results are returned via stdout and the system will ask for a new formula.) \n";
    out << "a blank line will stop the program.) \n";
}
//*************************************

int main(int argc, char *argv[])

{

    switch(argc) {
        case 2 :
            if(std::string(argv[1])== "--stdin")
                automatonfile=""; //empty implies: stdin must be read
            else{
                std::cerr << "single option is not '--stdin'.\n";
                print_usage(std::cerr);
                return 1;
            }
            break;
        case 3 :
            if(std::string(argv[1])== "--a")
                    automatonfile=argv[2];
            else{
                    std::cerr << "first option out of two is not '--a'.\n";
                    print_usage(std::cerr);
                    return 1;
            }
            break;
        case 5 :
            if(std::string(argv[3]) == "--f")
                    formula=argv[4];
            else if (std::string(argv[3])== "--ff")
                    formulafile=argv[4];
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


    // do stuff;
    clock_start = std::chrono::system_clock::now();
    std::cout << "Start of LTL model-check."<< '\n'<< '\n';
    setup_spot();
    if (automatonfile==""){
        std::cout<<"Supply the model, line by line and end with 'EOF_HOA' and supply formulas: \n";
        std::remove("model.txt");
        aut_file.open("model.txt");
        while (   getline(std::cin , aut_line)) {

            aut_file << aut_line << std::endl;
            if (aut_line=="EOF_HOA") {
                aut_file.close();
                break;
            }
        }
    automatonfile=  "model.txt";
    }
    int res  = load_automaton(automatonfile);
    spot::twa_graph_ptr& aut = pa->aut;
    std::cout << "Automaton properties:\n";
    custom_print(std::cout, aut,0);
    std:: string auttitle =getAutomatonTitle(pa->aut);
    auto name = aut->get_named_prop<std::string>("automaton-name");
    std::string automatontitle;
       if (name!= nullptr){
           automatontitle = *name;
       }
       else {
           automatontitle="";}
    std::cout << "Automaton '" <<automatontitle<<"' loaded === "<<log_elapsedtime()<<'\n';
    std::remove("results.txt");
    results_file.open("results.txt");

   if (formula=="") {
       std::cout << "Enter a formula to verify on automaton " << automatontitle << '\n';
       while (getline(std::cin, formula)) {
           if (formula == "") break;
           result= check_property(formula, pa->aut);
           std::cout << result;
           results_file << result  ;
       }
       results_file.close();
   }
    else
        std::cout<<check_property(formula,pa->aut);
    std::cout <<"End of LTL model-check. status: "<<res<<" === "<< log_elapsedtime()<< "\n";
    return 0;
}