/** \file
This TESTAR wrapper for interfacing to the SPOT library consists of a single C++ sourcecode file,
with imperative function calls to subroutines.
The usage as a CLI is described in \ref README.md

cseng 2019-2020
 */
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
#include <regex>


// Globals
const std::string version = "20200601"; /**<  version of the application */ // NOLINT(cert-err58-cpp)
std::chrono::system_clock::time_point clock_start, clock_end; /**<  the clock variables are used to measure the runtime of specified actions */ // NOLINT(cert-err58-cpp)

/**
 * Find the position of the matching closing parenthesis ')'
 * @param data      string to search
 * @param openPos   int pointing to the position of the opening parenthesis
 * @return int       position of the matching closing parenthesis
 * inspired by https://stackoverflow.com/questions/12752225/how-do-i-find-the-position-of-matching-parentheses-or-braces-in-a-given-piece-of
 */
int findClosingParenthesis(std::string &data, int openPos) {
    char text[data.size() + 1];
    data.copy(text, data.size() + 1);
    text[data.size() + 1] = '\0';
    int closePos = openPos;
    int counter = 1;
    while (counter > 0) {
        char c = text[++closePos];
        if (c == '(') {
            counter++;
        } else if (c == ')') {
            counter--;
        }
    }
    return closePos;
}
/**
 * Find the position of the matching opening parenthesis '('
 * (search from right to left)
 * @param data      string to search
 * @param closePos   int pointing to the position of the closing parenthesis
 * @return int       position of the matching opening parenthesis
 *
 *
 * inspired by https://stackoverflow.com/questions/12752225/how-do-i-find-the-position-of-matching-parentheses-or-braces-in-a-given-piece-of
 */
int findOpeningParenthesis(std::string &data, int closePos) {
    char text[data.size() + 1];
    data.copy(text, data.size() + 1);
    text[data.size() + 1] = '\0';
    int openPos = closePos;
    int counter = 1;
    while (counter > 0) {
        char c = text[--openPos];
        if (c == '(') {
            counter--;
        } else if (c == ')') {
            counter++;
        }
    }
    return openPos;
}

/**
 * Find any matching substring and replace all occurrences with an other string
 * @param data          string that might contain substrings to search
 * @param toSearch      substring to search for
 * @param replaceStr    replacement string when an occurrence is found
 *
 *
 * inspired by https://thispointer.com/find-and-replace-all-occurrences-of-a-sub-string-in-c/
 */
void findAndReplaceAll(std::string &data, std::string toSearch, std::string replaceStr) {
    // Get the first occurrence
    size_t pos = data.find(toSearch);

    // Repeat till end is reached
    while (pos != std::string::npos) {
        // Replace this occurrence of Sub String
        data.replace(pos, toSearch.size(), replaceStr);
        // Get the next occurrence from the current position
        pos = data.find(toSearch, pos + replaceStr.size());
    }
}

/**
 * Find any matching substring and surround all occurrences with 'replacestring' + 'substring' + 'closing'
 *
 * @param data          string that might contain substrings to search
 * @param toSearch      substring to search for
 * @param replaceStr    prefix of the embedding
 * @param closing       suffix of the embedding
 *
 *
 * inspired by  https://thispointer.com/find-and-replace-all-occurrences-of-a-sub-string-in-c \n
 * customized  for TESTAR
 */

void findClosingParenthesisAndInsert(std::string &data, std::string toSearch, std::string replaceStr, std::string closing) {
    // Get the first occurrence
    size_t pos = data.find(toSearch);

    // Repeat till end is reached
    while (pos != std::string::npos) {
        // Replace this occurrence of Sub String
        //find matching bracket. assume last char in the toSearch is the "("

        int bracketpos = findClosingParenthesis(data, pos + toSearch.size() - 1); // NOLINT(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
        std::string orginalblock = data.substr(pos + toSearch.size() - 1, bracketpos - pos - 1);
        data.replace(pos, toSearch.size() + orginalblock.size() - 1, toSearch + replaceStr + orginalblock + closing);
        // Get the next occurrence from the current position
        pos = data.find(toSearch, pos + toSearch.size() + replaceStr.size() + orginalblock.size() + closing.size());
    }
}
/**
 * Find any matching substring and surround all occurrences with 'replacestring' + 'substring' + 'closing'
 * (search from right to left)
 * @param data          string that might contain substrings to search
 * @param toSearch      substring to search for
 * @param replaceStr    suffix of the embedding
 * @param opening       prefix of the embedding
 *
 *
 * inspired by https://thispointer.com/find-and-replace-all-occurrences-of-a-sub-string-in-c \n
 * customized  for TESTAR
 */
void findOpeningParenthesisAndInsert(std::string &data, std::string toSearch, std::string replaceStr, std::string opening) {
    // Get the first occurrence
    size_t pos = data.find(toSearch);

    // Repeat till end is reached
    while (pos != std::string::npos) {
        // Replace this occurrence of Sub String
        //find matching bracket. assume first char in the toSearch is the ")"

        int bracketpos = findOpeningParenthesis(data, pos - toSearch.size() + 1); // NOLINT(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
        std::string orginalblock = data.substr(bracketpos + 0, pos - bracketpos + 1);
        data.replace(bracketpos, toSearch.size() + orginalblock.size() - 1,
                     opening + orginalblock + replaceStr + toSearch);
        // Get the next occurrence from the current position
        pos = data.find(toSearch,
                        bracketpos + opening.size() + orginalblock.size() + replaceStr.size() + toSearch.size());
    }
}

/**
 * Get the system time as a string
 * The preferred date and time representation for the current locale
 * @return date and time  as string ( formated with  'c%' )
 */
std::string getCurrentLocalTime() {
    time_t curr_time;
    tm *curr_tm;
    char date_timestring[50];
    time(&curr_time);
    curr_tm = localtime(&curr_time);
    strftime(date_timestring, 50, "%c", curr_tm);
    return date_timestring;

}
/**
 *
 * @return  Calculate the throughput time in seconds since the start of the program
 */
double getElapsedtime() {

    clock_end = std::chrono::system_clock::now();
    std::chrono::duration<float> elapsed_seconds = clock_end - clock_start;
    return (elapsed_seconds.count());
}
/**
 *
 * @return string with a static text "elapsed_seconds:" and the elapsed time in seconds
 */
std::string log_elapsedtime() {
    return "elapsed_seconds: " + std::to_string(getElapsedtime()) + ";";
}

/**
 * computes the actual memory consumption information consisting of RSS,Shared, EXe and Data allocation
 *
 * @return formatted string of some memory components
 * inspired by https://gist.github.com/thirdwing/da4621eb163a886a03c5
 * see also  http://man7.org/linux/man-pages/man5/proc.5.html
 */
std::string log_mem_usage()

{
    long mem_size;
    long mem_rss;
    long mem_shared;
    long mem_text;
    long mem_lib;  //always 0
    long mem_data;
    long mem_dt; // always 0
    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages

    {
        std::string ignore;
        std::ifstream ifs("/proc/self/statm", std::ios_base::in);
        ifs >> mem_size >> mem_rss >> mem_shared >> mem_text >> mem_lib >> mem_data >> mem_dt;
    }
    return
            " VMemory (kb):"
            " Size: " + std::to_string(static_cast<int>(mem_size) * page_size_kb) +
            "; RSS: " + std::to_string(static_cast<int>(mem_rss) * page_size_kb) +
            "; Shared: " + std::to_string(static_cast<int>(mem_shared) * page_size_kb) +
            "; Exe: " + std::to_string(static_cast<int>(mem_text) * page_size_kb) +
            //"; Lib: " + std::to_string(static_cast<int>(mem_lib)*page_size_kb)+
            "; Data: " + std::to_string(static_cast<int>(mem_data) * page_size_kb);
}

//
/**
 * Simple commandline parser.
 * @param argc number of arguments on the commandline
 * @param argv pointer to the list of arguments
 * @param option the option to search for in the argiument list.
 * @param novalue if the option is a boolean and does require a value to read in
 *          If True the next argv is regarded as the value for the option
 *          If False the option is a boolean and does not require a value form the commandline
 * @return  empty string is the option is not found
 *          if the option is found, it returns the next argment value on the commandline.
 *          if option is found and novalue is True, it returns the remainder of the argument (substracts option)
 *
 *          example: if the argument is '--pleasedothis' and the option is '--pleasedot' the return wil be : 'his'
 * inspired by https://gist.github.com/plasticbox/3708a6cdfbece8cd224487f9ca9794cd
 */
std::string getCmdOption(int argc, char *argv[], const std::string &option, bool novalue = false) {
    std::string cmd;
    for (int i = 0; i < argc; ++i) {
        std::string arg = argv[i];
        if (0 == arg.find(option)) { //match from start
            if (novalue) { //option without a value
                //int a = arg.size();
                size_t b = option.size();
                size_t c = 1;
                cmd = arg.substr(b, c);//, a-b);
                //'+' in stead of ','  and the one-off cost me an evening
                return cmd;
            } else if (i < (argc - 1)) {//take the next argument as value
                cmd = argv[i + 1];
                return cmd;
            }
        }
    }
    return cmd;
}

/**
 * Writes a stream to a file. The stream will stop after the string "EOF_HOA" or the last line
 * @param autin         containing  HOA formatted content
 * @param copyTofilename filename to copy the stream to.
 */
void streamAutomatonToFile(std::istream &autin, std::string copyTofilename) {
    //the parser can only load from file , not from a stream.
    std::ofstream aut_file;
    std::string aut_line;
    char *filenamearray = new char[copyTofilename.length() + 1];
    strcpy(filenamearray, copyTofilename.c_str());
    std::remove(filenamearray);
    aut_file.open(copyTofilename.c_str());
    while (getline(autin, aut_line)) {
        aut_file << aut_line << std::endl;
        if (aut_line == "EOF_HOA") {
            aut_file.close();
            break;
        }
    }
}
/**
 * Loads the file as a SPOT automaton
 * @param bdd binary decision diagram that hosts the atomic propositions of the automaton
 * @param pa_ptr pointer to the automaton to be populated
 * @param hoafile string of the full path of the file
 * @return empty string on success. Error string on failure
 */
std::string loadAutomatonFromFile(spot::bdd_dict_ptr &bdd, spot::parsed_aut_ptr &pa_ptr, const std::string &hoafile) {
    //loads only the first automaton in the file!
    pa_ptr = parse_aut(hoafile, bdd);
    if (pa_ptr->format_errors(std::cerr))
        return "=== ERROR loading automaton. Syntax error while reading automaton input file";
    if (pa_ptr->aborted) // following can only occur when reading  a HOA file.
        return "=== ERROR loading automaton. 'ABORT' directive found in the HOA file";
    return "";
}

/**
 * Prints the Instruction for use of the correct parameters to the stream.
 * @param out stream to write the contents to.
 * (Usually std-out)
 */
void print_help(std::ostream &out) {
    out << "\n";
    out << "Program version : " << version << "\n";
    out << "Usage:  spot_checker --stdin --a <file> --sf <formula> --ff <file> --fonly --ltlf <ap> --ltl2f <ap>\n";
    out << "Commandline options:\n";
    out << "--stdin   all input is  via standard input stream: first an automaton (HOA format) followed by formulas.\n";
    out << "          'EOF_HOA' + <enter>  mark the end of the automaton.\n";
    out << "          all other arguments are ignored and output is via stdout.\n";
    out << "--a       mandatory unless --stdin is the argument. filename containing the automaton (HOA format). \n";
    out << "--sf      optional.  the single LTL formula/property to check.  \n";
    out << "--ff      optional.  filename containing multiple formulas/properties. \n";
    out
            << "--fonly   optional.  verifies the syntax of the given formulas and LTLF variant. ignores options --a,--witness\n";
    out << "          uses '!dead' as ltlf value in case there is no --ltl(2)f option supplied.\n";
    out << "--ltlf    optional.  usable for finite LTL (if the automaton contains dead states): \n";
    out << "          Weaves an atomic proposition into the formula to label the 'alive' part. \n";
    out << "          e.g. '--ltlf !dead or --ltlf alive' . Note: this AP MUST exist in the automaton as well!!\n";
    out << "          terminal states in the model shall have a self-loop with AP='dead' or '!alive' or\n";
    out << "          always transition to a(n artificial) dead-state with such a self-loop\n";
    out << "--ltl2f   optional.  same as --ltlf but checks both the original formula and the ltlf variant\n";
    out << "--witness optional.  generates a trace: counterexample( for FAIL)or witness (for PASS)\n";
    //out << "--o       optional.  filename containing output. Without this option, output is via stdout\n\n";
    out << "\n";
    out << "Use-case when only option --a is supplied (without --sf or --ff): \n";
    out << "          The user can supply via stdin a formula/property. Results are returned via stdout.\n";
    out << "          The system will ask for a new formula. A blank line will stop the program. \n";
    out << "\n";
    out << "Note:     large automatons (states,ap's), large formula (size, ap's) \n";
    out << "          can make the program unresponsive or even time-out due to lack of memory. \n";
}

/**
 * Modification of the custom_print function on SPOT website
 * Print automaton properties and statistics to the out-stream
 * Prints information on the LTL on finite traces / models with terminal states
 * Prints info whether the Model is a DAG, directed acyclic graph
 *
 * @param out           stream to write the contents to
 * @param aut           automaton to inspect
 * @param verbosity     provides detail per state. advised to leave this as 0
 * @param ltlf_alive_ap if non empty this will write information on finite/terminal
 * @param dag           if the model is a dag
 */
void custom_print(std::ostream &out, spot::twa_graph_ptr &aut, int verbosity = 0, std::string ltlf_alive_ap = "", bool dag = false) {
    // from SPOT website. We need the dictionary to print the BDDs that label the edges
    const spot::bdd_dict_ptr &dict = aut->get_dict();
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
    if (verbosity != 0) {
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
    if (ltlf_alive_ap.length() != 0) {
        std::cout << "Finite LTL checking  with 'alive' proposition instantiated as \"" << ltlf_alive_ap
                  << "\"\n";
        std::cout << "  1. The logic of De Giacomo & Vardi 2013,2014 is applied for LTLf checking.\n";
        std::cout << "     key is the operators enrichment: X (p) => X(p & !dead) , p U Q => p U (q & !dead)\n";
        std::cout << "     and the the termination requirement !dead U G(dead)\n";
        std::cout
                << "  2. If the automaton also contains cycles (~ is not a DAG), then  the following is applied: \n";
        std::cout << "     key is the operators enrichment: X (p) => X(p | dead) , p U Q => p U (q | dead)\n";
        std::cout << "     and the the termination requirement !dead W G(dead)\n";
        std::cout << "     technically accomplished by: \n";
        std::cout << "       a. applying the logic of  G&V-2013\n";
        std::cout << "       b. the last U, which was induced by G&V-2013 is replaced by W \n";
        std::cout << "       c. and :'((dead) |(.....) )' is weaved/appended for any F,X,U \n";
        std::cout << "       d. and :'( (.....)  | (dead))' is weaved/prepended for any  M \n";
        std::cout << "       e. G,R and W operators do not require any modification \n";
        std::cout << "     Example for translating G(p0 -> F(p1)):\n";
        std::cout << "       first pass (G&V-2013): !dead & G(dead |(p0->F(p1 & !dead))) & (!dead U G(dead))\n";
        std::cout << "       and final pass       : !dead & G(dead |(p0->F((dead) | (p1 & !dead)))) & (!dead W G(dead))\n";
        std::cout << "       equivalent to        : !dead & G(dead |(p0->F(p1 |dead))) & (!dead W G(dead))\n";
        std::cout << "     Semantically, this adaption facilitates liveness checks in SCC's \n";
        std::cout << "     while allowing a dangling request in the finite suffix of a trace to a terminal state\n";
        std::cout << "  (This automaton has " << (dag ? "no" : "") << "cycles in the 'alive' part)\n";
    }
}
/**
 * Gets the title from the SPOT automaton
 * @param aut automaton
 * @return will be empty string when no title is provided
 */
std::string getAutomatonTitle(spot::twa_graph_ptr &aut) {
    auto name = aut->get_named_prop<std::string>("automaton-name");
    if (name != nullptr) {
        return *name;
    } else {
        return "";
    }
}

/**
 * Model checks a single LTL formula on the Buchi automaton
 * This function checks the syntax of the formula and
 * whether the atomic propositions of the formula occur in the automaton.
 * for 'incomplete models the formulas are converted to their LTLf variant \see custom_print
 *
 * @param formula           LTL formula to check
 * @param tracetodead       equivalent to model == dag
 * @param witness           provide a witness (if formula PASSes or counterexample if formula FAILs )
 * @param ltlf_alive_ap     the identifier  of the property that is TRUE on the alive part of the model.
 * @param bdd               binary decision diagram that hosts the atomic propositions of the automaton
 * @param aut               buchi automaton
 * @return                  multiline stroing with PASS or FAIL infomration, timing and counterexample traces.
 */
std::string check_property(std::string formula, bool tracetodead, bool witness, std::string ltlf_alive_ap,
                           spot::bdd_dict_ptr &bdd,
                           spot::twa_graph_ptr &aut) {

    std::ostringstream sout;  //needed for capturing output of run.
    sout << "=== Formula\n";
    sout << "=== " + formula;     //sout << "=== Start\n=== " << log_elapsedtime() << log_mem_usage()<<"\n"
    spot::parsed_formula pf = spot::parse_infix_psl(formula);
    if (ltlf_alive_ap.length() != 0) {
        spot::formula finitef = spot::from_ltlf(pf.f, ltlf_alive_ap.c_str());
        std::string ltlf_string = str_psl(finitef);
        if (tracetodead) {
            pf = spot::parse_infix_psl(ltlf_string);
            sout << "    [LTLF G&V-2013 variant: " + ltlf_string + "]";
        } else {
            std::string lastUntil = " U ";
            std::string weakUntil = " W ";
            std::size_t found = ltlf_string.rfind(lastUntil);
            if (found != std::string::npos) { //equality should not occur
                ltlf_string.replace(found, lastUntil.length(), weakUntil);
                // check liveness in scc's, but allow dangling requests in final trace
                if (ltlf_alive_ap.at(0) == '!') {
                    ltlf_alive_ap=ltlf_alive_ap.substr(1);
                }else{
                    ltlf_alive_ap='!'+ltlf_alive_ap;}
                findClosingParenthesisAndInsert(ltlf_string, "F(", "(" + ltlf_alive_ap + ")|", ")");
                findClosingParenthesisAndInsert(ltlf_string, "X(", "(" + ltlf_alive_ap + ")|", ")");
                findClosingParenthesisAndInsert(ltlf_string, "U (", "(" + ltlf_alive_ap + ")|", ")");
                findOpeningParenthesisAndInsert(ltlf_string, ") M", "|(" + ltlf_alive_ap + ")", "(");
                pf = spot::parse_infix_psl(ltlf_string);
                sout << "    [LTLF Modelvariant: " + ltlf_string + "]";

            }
        }
    }
    sout << "\n";
    spot::formula f = pf.f;
    sout << "=== ";
    bool syntaxOK = pf.errors.empty();
    if (!syntaxOK) {
        sout << "ERROR, syntax error while parsing last formula.\n";
    } else {
        //do the real modelcheck
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
        } else {

            spot::formula nf = spot::formula::Not(f);
            spot::translator ntrans = spot::translator(bdd);
            ntrans.set_level(spot::postprocessor::Low);

            spot::twa_graph_ptr af = ntrans.run(nf);
            bool nonempty;
            spot::twa_run_ptr run;
            if (!witness) {
                nonempty = aut->intersects(af);
                if (nonempty) {
                    sout << "FAIL, no counterexample requested.\n";
                } else {
                    sout << "PASS, no witness requested.\n";
                }
            } else {
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
    }
    return sout.str();
}
/**
 * Verifies whether the formula has a valid LTL syntax
 *
 * @param formula       formula to verify
 * @param ltlf_alive_ap the identifier  of the property that is TRUE on the alive part of the model.
 *                      if non empty: rewrite the formula to LTLf variants
 * @return              multiline string with the original formula,
 *                      LTLf variants and verdict if the formula is valid or not.
 */
std::string check_formulaproperty(std::string formula, std::string ltlf_alive_ap) {

    std::ostringstream sout;  //needed for capturing output of run.
    sout << "=== Formula\n";
    sout << "=== " + formula;     //sout << "=== Start\n=== " << log_elapsedtime() << log_mem_usage()<<"\n"
    spot::parsed_formula pf = spot::parse_infix_psl(formula);
    if (ltlf_alive_ap.length() != 0) {
        spot::formula finitef = spot::from_ltlf(pf.f, ltlf_alive_ap.c_str());
        std::string ltlf_string = str_psl(finitef);
        pf = spot::parse_infix_psl(ltlf_string);
        sout << "    [LTLF G&V-2013 variant: " + ltlf_string + "]";
        std::string lastUntil = " U ";
        std::string weakUntil = " W ";
        std::size_t found = ltlf_string.rfind(lastUntil);
        if (found != std::string::npos) { //equality should not occur
            ltlf_string.replace(found, lastUntil.length(), weakUntil);
            // used to check liveness in scc's, but allow dangling requests in final trace
            if (ltlf_alive_ap.at(0) == '!') {
                ltlf_alive_ap=ltlf_alive_ap.substr(1);
            }else{
                ltlf_alive_ap='!'+ltlf_alive_ap;}

            findClosingParenthesisAndInsert(ltlf_string, "F(", "(" + ltlf_alive_ap + ")|", ")");
            findClosingParenthesisAndInsert(ltlf_string, "X(", "(" + ltlf_alive_ap + ")|", ")");
            findClosingParenthesisAndInsert(ltlf_string, "U (", "(" + ltlf_alive_ap + ")|", ")");
            findOpeningParenthesisAndInsert(ltlf_string, ") M", "|(" + ltlf_alive_ap + ")", "(");
            pf = spot::parse_infix_psl(ltlf_string);
            sout << "    [LTLF Modelvariant: " + ltlf_string + "]";

        }

    }
    sout << "\n";
    spot::formula f = pf.f;
    sout << "=== ";
    bool syntaxOK = pf.errors.empty();
    if (!syntaxOK) {
        sout << "ERROR, syntax error while parsing last formula.\n";
    } else {
        sout << "Syntax is valid for last formula.\n";
    }
    return sout.str();
}

/**
 * Checks whether the Model is a DAG with only a selfloop for terminal states
 *
 * @param ltlf_alive_ap     the identifier  of the property that is TRUE on the alive part of the model.
 * @param bdd               binary decision diagram that hosts the atomic propositions of the automaton
 * @param aut               buchi automaton
 * @return                  False if ltlf_alive_ap is empty otherwise it wil check for DAG property
 */
bool model_has_noloops(std::string ltlf_alive_ap, spot::bdd_dict_ptr &bdd, spot::twa_graph_ptr &aut) {
    if (ltlf_alive_ap.length() != 0) {
        //alive U G(!alive): the 'U' makes that dead is required in all paths
        std::string tracetodead = ltlf_alive_ap + " U G(!" + ltlf_alive_ap + ")";
        std::string formula_result = check_property(tracetodead, false, false, "", bdd, aut);
        std::size_t found = formula_result.rfind("PASS");
        return (found != std::string::npos);
    } else
        return false;

}
/**
 * Model check a collection of LTL formulas
 * @param col_in            stream containing a collection of formulas
 * @param bdd               binary decision diagram that hosts the atomic propositions of the automaton
 * @param pa_ptr            pointer to the automaton
 * @param ltlf_alive_ap     the identifier  of the property that is TRUE on the alive part of the model.
 * @param originalandltlf   perform the check twice on both original and LTLf or just LTLf?
 * @param witness           ask for a witness (if formula PASSes or counterexample if formula FAILs )
 * @param out               stream for collection the results
 *
 * delegates checking of individual formulas to \see check_property
 */
void check_collection(std::istream &col_in, spot::bdd_dict_ptr &bdd, spot::parsed_aut_ptr &pa_ptr,
                      std::string ltlf_alive_ap, bool originalandltlf, bool witness, std::ostream &out) {
    std::string formula_result;
    std::string f;
    bool tracetodead = model_has_noloops(ltlf_alive_ap, bdd, pa_ptr->aut);
    while (getline(col_in, f)) {
        if (f.empty()) break;
        if (originalandltlf) {
            formula_result = check_property(f, tracetodead, witness, "", bdd, pa_ptr->aut);
            out << formula_result;
        }
        formula_result = check_property(f, tracetodead, witness, ltlf_alive_ap, bdd, pa_ptr->aut);
        out << formula_result;

    }
    out << "=== Formula\n";  // add closing tag for formulas
}
/**
 *  Verifies whether a collection of  formulas has a valid LTL syntax
 * @param col_in            stream containing a collection of formulas
 * @param ltlf_alive_ap     the identifier  of the property that is TRUE on the alive part of a model.
 * @param out               stream for collection the results
 *
 *  delegates checking of individual formulas to \see check_formula
 */
void check_formulacollection(std::istream &col_in, std::string ltlf_alive_ap, std::ostream &out) {
    std::string formula_result;
    std::string f;
    while (getline(col_in, f)) {
        if (f.empty()) break;
        formula_result = check_formulaproperty(f, ltlf_alive_ap);
        out << formula_result;
    }
    out << "=== Formula\n";  // add closing tag for formulas
}

/**
 * Entry point of the application:  Standard C/C++ routine \n
 * Schematically:\n
 * Starts the clock\n
 * Parse the commandline\n
 * Outputs with print_help if there was an error and terminates\n
 * Copy the automaton to a local file\n
 * Model-check the formula(s) or just validates formula(s)\n
 * Removes temporary files: automaton and property file\n
 * Terminates with exit code zero\n
 *
 * @param argc
 * @param argv
 * @return      zero if the progrma terminates succesfully
 */
int main(int argc, char *argv[]) {
    // do stuff;
    std::string automaton_filename;
    std::string formulafilename;
    std::string outfilename;
    std::ofstream out_file;
    std::string formula;
    std::string ltlf_alive_ap;
    bool dowitness;
    bool onlyformulasyntax;
    spot::parsed_aut_ptr pa;
    spot::bdd_dict_ptr bdd;

    std::string timecopy = getCurrentLocalTime();
    findAndReplaceAll(timecopy, " ", "_");
    findAndReplaceAll(timecopy, ":", "_");
    std::string copyofmodel = "temp_model" + timecopy + ".txt";// copy in case the input is from stdin

    clock_start = std::chrono::system_clock::now();
    bdd = spot::make_bdd_dict(); //setup_spot
    std::string startLog = "=== LTL model-check Start Program version : " + version + "\n=== " + getCurrentLocalTime() +
                           log_mem_usage() + "\n";

    std::string stdinput = getCmdOption(argc, argv, "--stdi", true); // use 'n' as value
    std::string automaton = getCmdOption(argc, argv, "--a");
    std::string singleformula = getCmdOption(argc, argv, "--sf");
    std::string formulafile = getCmdOption(argc, argv, "--ff");
    std::string ltlf = getCmdOption(argc, argv, "--ltlf");
    std::string ltl2f = getCmdOption(argc, argv, "--ltl2f");
    std::string witness = getCmdOption(argc, argv, "--witnes", true);//deliberate missing last char
    std::string checkonlyformulas = getCmdOption(argc, argv, "--fonl", true);
    //std::string outfile = "" ; //getCmdOption(argc, argv, "--o");

    onlyformulasyntax = (checkonlyformulas == "y");
    if (not onlyformulasyntax) {
        if (stdinput == "n") {
            automaton_filename = ""; //empty implies: stdin must be read
        } else if (automaton.empty()) {
            std::cerr << "no automaton supplied via '--a'.\n";
            print_help(std::cerr);
            return 1;
        } else if (not fs::exists(automaton)) {
            std::cerr << "automaton file not found for option '--a'.\n";
            print_help(std::cerr);
            return 1;
        } else
            automaton_filename = automaton;
    }

    if (singleformula.empty()) {
        if (formulafile.empty()) {
            std::cerr << "no formulas supplied via '--sf' nor '--ff'.\n";
            print_help(std::cerr);
            return 1;
        } else if (not fs::exists(formulafile)) {
            std::cerr << "formula file not found for option '--ff'.\n";
            print_help(std::cerr);
            return 1;
        } else
            formulafilename = formulafile;
    } else
        formula = singleformula;

    dowitness = (witness == "s");
    ltlf_alive_ap = "";
    if (not ltl2f.empty()) {
        ltlf_alive_ap = ltl2f;
    } else if (not ltlf.empty()) {
        ltlf_alive_ap = ltlf;
    } else if (onlyformulasyntax) {
        ltlf_alive_ap = "!dead";
    }

    // commandline sanitation done

    std::cout << startLog;

    if (not onlyformulasyntax) {
        if (automaton_filename.empty()) {
            streamAutomatonToFile(std::cin, copyofmodel);
        } else {// seems overhead to make a copy, but skipping this will end up in an exit code 139
            std::ifstream infile;
            infile.open(automaton_filename.c_str());
            streamAutomatonToFile(infile, copyofmodel);
        }
        automaton_filename = copyofmodel;

        std::cout << "=== Automaton\n";
        std::cout << "=== " << log_elapsedtime() << log_mem_usage() << "\n";
        std::string res = loadAutomatonFromFile(bdd, pa, automaton_filename);
        if (res.empty()) {
            std::cout << "=== ";
            bool dag = model_has_noloops(ltlf_alive_ap, bdd, pa->aut);
            custom_print(std::cout, pa->aut, 0, ltlf_alive_ap,dag);

            std::string auttitle = getAutomatonTitle(pa->aut);
            std::cout << "=== " << log_elapsedtime() << log_mem_usage() << "\n";
            std::cout << "=== Automaton\n";


            if (!formulafilename.empty()) {
                std::ifstream f_in;
                f_in.open(formulafilename.c_str());
                check_collection(f_in, bdd, pa, ltlf_alive_ap, not ltl2f.empty(), dowitness,
                                 std::cout);
            } else if (!formula.empty()) {
                std::istringstream s_in;
                s_in.str(formula);
                check_collection(s_in, bdd, pa, ltlf_alive_ap, not ltl2f.empty(), dowitness,
                                 std::cout);
            } else
                check_collection(std::cin, bdd, pa, ltlf_alive_ap, not ltl2f.empty(), dowitness,
                                 std::cout);
        } else {
            std::cout << res << "\n";
            std::cout << "=== " << log_elapsedtime() << log_mem_usage() << "\n";
            std::cout << "=== Automaton\n";
        }
        if (automaton_filename == copyofmodel) { //input was via stdin, so removing the output file
            char *fnamearray = new char[copyofmodel.length() + 1];
            strcpy(fnamearray, copyofmodel.c_str());
            std::remove(fnamearray);
        }
    } else {

        if (!formulafilename.empty()) {
            std::ifstream f_in;
            f_in.open(formulafilename.c_str());
            check_formulacollection(f_in, ltlf_alive_ap, std::cout);
        } else if (!formula.empty()) {
            std::istringstream s_in;
            s_in.str(formula);
            check_formulacollection(s_in, ltlf_alive_ap, std::cout);
        } else
            check_formulacollection(std::cin, ltlf_alive_ap, std::cout);
    }
    std::cout << "=== LTL model-check End\n=== " << log_elapsedtime() << log_mem_usage() << "\n";
    return 0;
}