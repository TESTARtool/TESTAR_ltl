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
#include <iomanip>      // std::setprecision
#include <spot/tl/ltlf.hh>
#include <nlohmann/json.hpp>



// Globals
const std::string version = "20191208";
const std::string resultfilename = "propertylog.txt";
const std::string modelfilename = "model.txt";  // internal copy
std::chrono::system_clock::time_point clock_start, clock_end;
spot::parsed_aut_ptr pa;
spot::bdd_dict_ptr bdd;


void setup_spot(){
    bdd = spot::make_bdd_dict();

}

void testjson(){
    nlohmann::json j;

}
//https://thispointer.com/find-and-replace-all-occurrences-of-a-sub-string-in-c/
void findAndReplaceAll(std::string & data, std::string toSearch, std::string replaceStr)
{
    // Get the first occurrence
    size_t pos = data.find(toSearch);

    // Repeat till end is reached
    while( pos != std::string::npos)
    {
        // Replace this occurrence of Sub String
        data.replace(pos, toSearch.size(), replaceStr);
        // Get the next occurrence from the current position
        pos =data.find(toSearch, pos + replaceStr.size());
    }
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
double getElapsedtime(){

    clock_end = std::chrono::system_clock::now();
    std::chrono::duration<float> elapsed_seconds = clock_end-clock_start;
    return (elapsed_seconds.count()) ;
}

std::string log_elapsedtime(){
    return "elapsed_seconds: " + std::to_string(getElapsedtime()) +";";
}



std::string log_mem_usage()
// inspired by https://gist.github.com/thirdwing/da4621eb163a886a03c5
{

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
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(0) << (vsize / 1024.0);
    std::string vm_str = ss.str();
    return " VM_mem_usage_kb: " + vm_str+"; RSS_mem_usage_kb: " + std::to_string(rss * page_size_kb);
}






void streamAutomatonToFile(std::istream &autin, std::string copyTofilename){
    //the parser can only load from file , not from a stream.
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

std::string  loadAutomatonFromFile(const std::string &hoafile)
{
    //loads only the first automaton in the file!
    pa = parse_aut(hoafile, bdd);
    if (pa->format_errors(std::cerr))
        return "=== ERROR loading automaton. Syntax error while reading automaton input file";
    if (pa->aborted) // following can only occur when reading  a HOA file.
        return"=== ERROR loading automaton. 'ABORT' directive found in the HOA file";
    return "";
}


void print_help(std::ostream &out){
    out << "\n";
    out << "Program version : "<<version<<"\n";
    out << "Usage:  spot_checker --stdin --a <file> --f <formula> --ff <file> --ltlf <ap> --o <file>\n";
    out << "Commandline options:\n";
    out << "--stdin   all input is  via standard input stream: first an automaton followed by formulas. \n";
    out << "          all other arguments are ignored and output is via stdout.\n";
    out << "--a       mandatory unless --stdin is the argument. filename containing the automaton (HOA format). \n";
    out << "--f       optional.  the LTL formula/property to check.  \n";
    out << "--ff      optional.  filename containing multiple formulas/properties. \n";
    out << "--ltlf    optional.  usable for finite LTL (if the automaton contains dead states): \n";
    out << "          Weaves an atomic proposition into the formula to label the 'alive' part. \n";
    out << "          e.g. '--ltlf !dead or --ltlf alive' . Note: this AP MUST exist in the automaton as well!!\n";
    out << "          terminal states in the model shall have a self-loop with AP='dead' or '!alive' or\n";
    out << "          always transition to an artificial dead-state with such a self-loop\n";
    out << "--o       optional.  filename containing output. Without this option, output is via stdout\n\n";
    out << "Use-case when only option --a is supplied (without --f or --ff): \n";
    out << "  The user can supply via stdin a formula/property. Results are returned via stdout.\n";
    out << "  The system will ask for a new formula. A blank line will stop the program.) \n";
}

//void custom_print(std::ostream& out, spot::twa_graph_ptr& aut, int verbosity );   //declare before

void custom_print(std::ostream& out, spot::twa_graph_ptr& aut, int verbosity = 0)
{
    // We need the dictionary to print the BDDs that label the edges
    const spot::bdd_dict_ptr& dict = aut->get_dict();
    std::cout << "Properties of Automaton:\n";
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

std::string getAutomatonTitle(spot::twa_graph_ptr& aut){
    auto name = aut->get_named_prop<std::string>("automaton-name");
    if (name!= nullptr){
        return *name;
    }
    else {
        return "";
    }
}


std::string check_property( std::string formula,std::int8_t  tracetodead, std::string ltlf_alive_ap , spot::twa_graph_ptr& aut) {

    std::ostringstream sout;  //needed for capturing output of run.
    sout << "=== Formula\n";
    sout << "=== "+formula;     //sout << "=== Start\n=== " << log_elapsedtime() << log_mem_usage()<<"\n";
    spot::parsed_formula pf = spot::parse_infix_psl(formula);
    if (ltlf_alive_ap.length() != 0) {
        spot::formula finitef = spot::from_ltlf(pf.f, ltlf_alive_ap.c_str());
        std::string ltlf_string= str_psl(finitef);
        if (tracetodead) {
            pf = spot::parse_infix_psl(ltlf_string);
            sout << "    [LTLF tracevariant: " + ltlf_string + "]";
        }
        else{
            std::string lastUntil = " U ";
            std::string weakUntil = " W ";
            std::size_t found = ltlf_string.rfind(lastUntil);
            if (found != std::string::npos) { //equality should not occur
                ltlf_string.replace(found, lastUntil.length(), weakUntil);
                // check liveness in scc's, but allow dangling requests in final trace
                findAndReplaceAll(ltlf_string,"F(","F((!"+ltlf_alive_ap+")|"); //F(...) => F((!!dead)|(..)
                findAndReplaceAll(ltlf_string,"X(","X((!"+ltlf_alive_ap+")|");
                findAndReplaceAll(ltlf_string,"U (","U ((!"+ltlf_alive_ap+")|");
                findAndReplaceAll(ltlf_string,") M","|(!"+ltlf_alive_ap+")) M");


                pf = spot::parse_infix_psl(ltlf_string);
                sout << "    [LTLF modelvariant: " + ltlf_string + "]";

            }
        }
    }
    sout <<"\n";
    spot::formula f = pf.f;


    if (!pf.errors.empty()) {
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
            //custom_print(std::cout, af, 1);
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
    //sout << "=== End\n=== " << log_elapsedtime() << log_mem_usage()<<"\n";
    //sout << "=== Formula\n";
    return sout.str();
}
std::int8_t issingletracetodead( std::string ltlf_alive_ap, spot::twa_graph_ptr& aut) {
    if (ltlf_alive_ap.length() != 0) {
        std::string tracetodead= ltlf_alive_ap +" U G(!"+ltlf_alive_ap+")"; //alive U G(!alive): dead is required
        std::string  formula_result= check_property(tracetodead, false,ltlf_alive_ap,pa->aut);
        std::size_t found = formula_result.rfind("PASS");
        if (found!=std::string::npos){
            return true;
        }else
            return false;
    }
    else
        return false;

}

void check_collection(std::istream& col_in, std::string ltlf_alive_ap,std::ostream& out, std::string results_filename){

    std::ofstream results_file;
    std::string formula_result;
    std::string f;
    if (fs::exists(results_filename.c_str()))     std::remove(results_filename.c_str());
    results_file.open(results_filename.c_str());
    std::int8_t tracetodead=issingletracetodead(ltlf_alive_ap,pa->aut);
    while (getline(col_in, f)) {
        if (f == "") break;

        formula_result= check_property(f,tracetodead,ltlf_alive_ap,pa->aut);
        out << formula_result;
        results_file << formula_result  ;
    }
    out << "=== Formula\n";  // add closing tag for formulas
    results_file.close();

}



int main(int argc, char *argv[])

{
    // do stuff;
    std::string automaton_filename;
    std::string formulafilename;
    std::string outfilename;
    std::ofstream out_file;
    std:: string formula;
    std::string ltlf_alive_ap;

    clock_start = std::chrono::system_clock::now();
    std::string startLog = "=== LTL model-check Start Program version : "+version+"\n=== "+  getCurrentLocalTime()+ log_mem_usage()+"\n" ;

    switch(argc) {
        case 2 :
            if (std::string(argv[1]) == "--stdin")
                automaton_filename = ""; //empty implies: stdin must be read
            else {
                std::cerr << "single option is not '--stdin'.\n";
                print_help(std::cerr);
                return 1;
            }
            break;
        case 3 :
            if (std::string(argv[1]) == "--a")
                automaton_filename = std::string(argv[2]);
            else {
                std::cerr << "first option is not '--a'.\n";
                print_help(std::cerr);
                return 1;
            }
            break;
        case 5 :
            if (std::string(argv[1]) == "--a")
                automaton_filename = std::string(argv[2]);
            else {
                std::cerr << "first option is not '--a'.\n";
                print_help(std::cerr);
                return 1;
            }
            if (std::string(argv[3]) == "--f")
                formula = std::string(argv[4]);
            else if (std::string(argv[3]) == "--ff")
                if (fs::exists(argv[4]))
                    formulafilename = std::string(argv[4]);
                else {
                    std::cerr << "formula file not found for option '--ff'.\n";
                    print_help(std::cerr);
                    return 1;
                }
            else {
                std::cerr << "second option  is not '--f or --ff'.\n";
                print_help(std::cerr);
                return 1;
            }
            break;



//case7
        case 7 :
            if (std::string(argv[1]) == "--a")
                automaton_filename = std::string(argv[2]);
            else {
                std::cerr << "first option is not '--a'.\n";
                print_help(std::cerr);
                return 1;
            }
            if (std::string(argv[3]) == "--f")
                formula = std::string(argv[4]);
            else if (std::string(argv[3]) == "--ff")
                if (fs::exists(argv[4]))
                    formulafilename = std::string(argv[4]);
                else {
                    std::cerr << "formula file not found for option '--ff'.\n";
                    print_help(std::cerr);
                    return 1;
                }
            else {
                std::cerr << "second option  is not '--f or --ff'.\n";
                print_help(std::cerr);
                return 1;
            }
            if (std::string(argv[5]) == "--ltlf")
                ltlf_alive_ap = std::string(argv[6]);
            else if (std::string(argv[5]) == "--o") {
                outfilename = std::string(argv[6]);
                std::remove(outfilename.c_str());
                out_file.open(outfilename.c_str());
                if (fs::exists(outfilename)) {
                    { //see https://www.geeksforgeeks.org/io-redirection-c/
                        // Backup streambuffers of  cout  css 20190728 not actually  needed.
                        std::streambuf *stream_buffer_cout = std::cout.rdbuf();
                        //std::streambuf* stream_buffer_cin = cin.rdbuf();
                        // Get the streambuffer of the file
                        std::streambuf *stream_buffer_file = out_file.rdbuf();
                        // Redirect cout to file !!!
                        std::cout.rdbuf(stream_buffer_file);
                        // Redirect cout back to screen
                        //std::cout.rdbuf(stream_buffer_cout);
                    }

                } else {
                    std::cerr << "output file error'.\n";
                    print_help(std::cerr);
                    return 1;
                }
            } else {
                std::cerr << "third option  is not '--ltlf' or '--o'.\n";
                print_help(std::cerr);
                return 1;
            }
            break;
//case7
//case9
        case 9 :
            if (std::string(argv[1]) == "--a")
                automaton_filename = std::string(argv[2]);
            else {
                std::cerr << "first option is not '--a'.\n";
                print_help(std::cerr);
                return 1;
            }
            if (std::string(argv[3]) == "--f")
                formula = std::string(argv[4]);
            else if (std::string(argv[3]) == "--ff")
                if (fs::exists(argv[4]))
                    formulafilename = std::string(argv[4]);
                else {
                    std::cerr << "formula file not found for option '--ff'.\n";
                    print_help(std::cerr);
                    return 1;
                }
            else {
                std::cerr << "second option  is not '--f or --ff'.\n";
                print_help(std::cerr);
                return 1;
            }

            if (std::string(argv[5]) == "--ltlf")
                ltlf_alive_ap = std::string(argv[6]);
            else {
                std::cerr << "third option is not '--a'.\n";
                print_help(std::cerr);
                return 1;
            }

            if (std::string(argv[7]) == "--o")
            {
                outfilename = std::string(argv[8]);
                std::remove(outfilename.c_str());
                out_file.open(outfilename.c_str());
                if (fs::exists(outfilename)) {
                    { //see https://www.geeksforgeeks.org/io-redirection-c/
                        // Backup streambuffers of  cout  css 20190728 not actually  needed.
                        std::streambuf* stream_buffer_cout = std::cout.rdbuf();
                        //std::streambuf* stream_buffer_cin = cin.rdbuf();
                        // Get the streambuffer of the file
                        std::streambuf* stream_buffer_file = out_file.rdbuf();
                        // Redirect cout to file !!!
                        std::cout.rdbuf(stream_buffer_file);
                        // Redirect cout back to screen
                        //std::cout.rdbuf(stream_buffer_cout);
                    }

                } else {
                    std::cerr << "output file error'.\n";
                    print_help(std::cerr);
                    return 1;
                }
            }
            else {
                std::cerr << "fourth option  is not '--o'.\n";
                print_help(std::cerr);
                return 1;
            }
            break;
//case9



        default :
            print_help(std::cerr);
            return 1;
    }
     std::cout << startLog;
    setup_spot();


    if (automaton_filename==""){
        streamAutomatonToFile(std::cin, modelfilename);
        automaton_filename=  modelfilename;
    }
    else if (automaton_filename!=modelfilename){// copy only if different from the default
        std::ifstream infile;
        infile.open(automaton_filename.c_str());
        streamAutomatonToFile(infile, modelfilename);
    }

    std::cout << "=== Automaton\n";
    std::cout << "=== " << log_elapsedtime() << log_mem_usage()<<"\n";

    std::string res  = loadAutomatonFromFile(automaton_filename);
    if (res=="") {
        std::cout << "=== ";
        custom_print(std::cout, pa->aut, 0);
        if (ltlf_alive_ap.length() != 0) {
            std::cout << "Finite LTL checking with 'alive' proposition instantiated as \"" << ltlf_alive_ap<<"\"\n";
            std::cout << "If the automaton is just a single trace, then extension '!dead U G(dead)' is applied \n";
            std::cout << "If the automaton contains one or more loops, then extension '!dead W G(dead)' is applied \n";
        }
        std::string auttitle = getAutomatonTitle(pa->aut);
        std::cout << "=== " << log_elapsedtime() << log_mem_usage()<<"\n";
        std::cout << "=== Automaton\n";


        if (formulafilename != "") {
            std::ifstream f_in;
            f_in.open(formulafilename.c_str());
            check_collection(f_in, ltlf_alive_ap,std::cout, resultfilename);
           // std::cout << "Formula file  '" << formulafilename << "' loaded === " << log_elapsedtime() << log_mem_usage()<<"\n";
        } else if (formula != "") {
            std::istringstream s_in;
            s_in.str(formula);
            check_collection(s_in, ltlf_alive_ap,std::cout, resultfilename);
        } else
            check_collection(std::cin, ltlf_alive_ap,std::cout, resultfilename);
    }
    else{
        std::cout<<res<<"\n";
        std::cout << "=== " << log_elapsedtime() << log_mem_usage()<<"\n";
        std::cout << "=== Automaton\n";
    }

    std::cout <<"=== LTL model-check End\n=== "<< log_elapsedtime()<< log_mem_usage()<<"\n";
    return 0;
}