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
const std::string version = "20191222";
std::chrono::system_clock::time_point clock_start, clock_end;


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
    long mem_size;
    long mem_rss;
    long mem_shared;
    long mem_text;
    long mem_lib;  //always 0
    long mem_data;
    long mem_dt; // always 0
    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages

    { // see http://man7.org/linux/man-pages/man5/proc.5.html
        std::string ignore;
        std::ifstream ifs("/proc/self/statm", std::ios_base::in);
        ifs >> mem_size >> mem_rss >> mem_shared >> mem_text >> mem_lib >> mem_data >> mem_dt;
    }
    return
            " VMemory (kb):"
           " Size: " + std::to_string(static_cast<int>(mem_size)*page_size_kb)+
           "; RSS: " + std::to_string(static_cast<int>(mem_rss)*page_size_kb)+
            "; Shared: " + std::to_string(static_cast<int>(mem_shared)*page_size_kb)+
            "; Exe: " + std::to_string(static_cast<int>(mem_text)*page_size_kb)+
            //"; Lib: " + std::to_string(static_cast<int>(mem_lib)*page_size_kb)+
            "; Data: " + std::to_string(static_cast<int>(mem_data)*page_size_kb);
}


void streamAutomatonToFile(std::istream &autin, std::string copyTofilename){
    //the parser can only load from file , not from a stream.
    std::ofstream aut_file;
    std::string aut_line;
    char * filenamearray = new char [copyTofilename.length()+1];
    strcpy (filenamearray, copyTofilename.c_str());
    std::remove(filenamearray);
    aut_file.open(copyTofilename.c_str());
    while (  getline(autin , aut_line)) {
        aut_file << aut_line << std::endl;
        if (aut_line == "EOF_HOA") {
            aut_file.close();
            break;
        }
    }
}

std::string  loadAutomatonFromFile(spot::bdd_dict_ptr &bdd,spot::parsed_aut_ptr &pa_ptr, const std::string &hoafile)
{
    //loads only the first automaton in the file!
    pa_ptr = parse_aut(hoafile, bdd);
    if (pa_ptr->format_errors(std::cerr))
        return "=== ERROR loading automaton. Syntax error while reading automaton input file";
    if (pa_ptr->aborted) // following can only occur when reading  a HOA file.
        return"=== ERROR loading automaton. 'ABORT' directive found in the HOA file";
    return "";
}


void print_help(std::ostream &out){
    out << "\n";
    out << "Program version : "<<version<<"\n";
    out << "Usage:  spot_checker --stdin --a <file> --f <formula> --ff <file> --ltlf <ap> --o <file>\n";
    out << "Commandline options:\n";
    out << "--stdin   all input is  via standard input stream: first an automaton (HOA format) followed by formulas. \n";
    out << "          'EOF_HOA' + <enter>  mark the end of the automaton.\n";
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
    out << "\n";
    out << "Use-case when only option --a is supplied (without --f or --ff): \n";
    out << "          The user can supply via stdin a formula/property. Results are returned via stdout.\n";
    out << "          The system will ask for a new formula. A blank line will stop the program. \n";
    out << "\n";
    out << "Note:     large automatons (states,ap's), large formula (size, ap's) \n";
    out << "          can make the program unresponsive or even time-out due to lack of memory. \n";
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


std::string check_property( std::string formula,std::int8_t  tracetodead, std::string ltlf_alive_ap ,spot::bdd_dict_ptr &bdd, spot::twa_graph_ptr& aut) {

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
        spot::translator trans = spot::translator(fbdd);
        //'Low' . consequence: checking intersecting-run can take longer as the f-automaton is not the smallest
        trans.set_level(spot::postprocessor::Low);
        spot::twa_graph_ptr aftemp = trans.run(f);
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
            spot::translator ntrans = spot::translator(bdd);
            ntrans.set_level(spot::postprocessor::Low);
            spot::twa_graph_ptr af = ntrans.run(nf);
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
std::int8_t issingletracetodead( std::string ltlf_alive_ap, spot::bdd_dict_ptr &bdd, spot::twa_graph_ptr& aut) {
    if (ltlf_alive_ap.length() != 0) {
        std::string tracetodead= ltlf_alive_ap +" U G(!"+ltlf_alive_ap+")"; //alive U G(!alive): dead is required
        std::string  formula_result= check_property(tracetodead, false,ltlf_alive_ap,bdd,aut);
        std::size_t found = formula_result.rfind("PASS");
        if (found!=std::string::npos){
            return true;
        }else
            return false;
    }
    else
        return false;

}

void check_collection(std::istream& col_in, spot::bdd_dict_ptr &bdd,spot::parsed_aut_ptr &pa_ptr, std::string ltlf_alive_ap,std::ostream& out){


    std::string formula_result;
    std::string f;
    std::int8_t tracetodead=issingletracetodead(ltlf_alive_ap,bdd,pa_ptr->aut);
    while (getline(col_in, f)) {
        if (f == "") break;
        formula_result= check_property(f,tracetodead,ltlf_alive_ap,bdd,pa_ptr->aut);
        out << formula_result;
    }
    out << "=== Formula\n";  // add closing tag for formulas
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
    spot::parsed_aut_ptr pa;
    spot::bdd_dict_ptr bdd;

    std::string timecopy = getCurrentLocalTime();
    findAndReplaceAll(timecopy," ","_");
    findAndReplaceAll(timecopy,":","_");
    std::string copyofmodel= "temp_model"+timecopy+".txt";// copy in case the input is from stdin

    clock_start = std::chrono::system_clock::now();
    bdd = spot::make_bdd_dict(); //setup_spot
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
                char * filenamearray = new char [outfilename.length()+1];
                strcpy (filenamearray, outfilename.c_str());
                std::remove(filenamearray); // remove old file wit same name
                out_file.open(outfilename.c_str());
                if (fs::exists(outfilename)) {
                    { //see https://www.geeksforgeeks.org/io-redirection-c/
                        // Backup streambuffers of  cout : css 20190728 not actually  needed.
                        //std::streambuf *stream_buffer_cout = std::cout.rdbuf();
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
                char * filenamearray = new char [outfilename.length()+1];
                strcpy (filenamearray, outfilename.c_str());
                std::remove(filenamearray); // remove old file wit same name

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


    if (automaton_filename==""){
        streamAutomatonToFile(std::cin, copyofmodel);
    }
    else {// seems overhead to make a copy, skipping this results in a exit code 139
        std::ifstream infile;
        infile.open(automaton_filename.c_str());
        streamAutomatonToFile(infile, copyofmodel);
    }
    automaton_filename=  copyofmodel;

    std::cout << "=== Automaton\n";
    std::cout << "=== " << log_elapsedtime() << log_mem_usage()<<"\n";
    std::string res  = loadAutomatonFromFile(bdd,pa,automaton_filename);
    if (res=="") {
        std::cout << "=== ";
        custom_print(std::cout, pa->aut, 0);
        if (ltlf_alive_ap.length() != 0) {
            std::cout << "Finite LTL checking  with 'alive' proposition instantiated as \"" << ltlf_alive_ap<<"\"\n";
            std::cout << "If the automaton:\n";
            std::cout << "1. is a single trace, then standard as in  De Giacomo & Vardi is applied. \n";
            std::cout << "2. contains loops, then besides (1.) '!dead W G(dead)' is appended and additionally :'(dead) | ' is weaved in any F,X,U and M \n";
        }
        std::string auttitle = getAutomatonTitle(pa->aut);
        std::cout << "=== " << log_elapsedtime() << log_mem_usage()<<"\n";
        std::cout << "=== Automaton\n";


        if (formulafilename != "") {
            std::ifstream f_in;
            f_in.open(formulafilename.c_str());
            check_collection(f_in, bdd,pa,ltlf_alive_ap,std::cout );
           // std::cout << "Formula file  '" << formulafilename << "' loaded === " << log_elapsedtime() << log_mem_usage()<<"\n";
        } else if (formula != "") {
            std::istringstream s_in;
            s_in.str(formula);
            check_collection(s_in,bdd, pa,ltlf_alive_ap,std::cout);
        } else
            check_collection(std::cin,bdd, pa,ltlf_alive_ap,std::cout);
    }
    else{
        std::cout<<res<<"\n";
        std::cout << "=== " << log_elapsedtime() << log_mem_usage()<<"\n";
        std::cout << "=== Automaton\n";
    }
    if (automaton_filename==copyofmodel){ //input was via stdin, so removing the output file
        char * filenamearray = new char [copyofmodel.length()+1];
        strcpy (filenamearray, copyofmodel.c_str());
        std::remove(filenamearray);
    }


    std::cout <<"=== LTL model-check End\n=== "<< log_elapsedtime()<< log_mem_usage()<<"\n";
    return 0;
}