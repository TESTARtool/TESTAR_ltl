/** \file
converts a list of LTL formulas to graphviz dot format
cseng 2020
 */
#include <iostream>
#include <string>
#include <spot/tl/parse.hh>
#include <spot/tl/print.hh>
#include <spot/tl/dot.hh>
#include <sstream>
#include <fstream>
#include<experimental/filesystem>
namespace fs = std::experimental::filesystem;
#include <unistd.h>

// Globals
const std::string version = "20200925"; /**<  version of the application */ // NOLINT(cert-err58-cpp)

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
 * Simple commandline parser.
 * @param argc number of arguments on the commandline
 * @param argv pointer to the list of arguments
 * @param option the option to search for in the argument list.
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
 * Prints the Instruction for use of the correct parameters to the stream.
 * @param out stream to write the contents to.
 * (Usually std-out)
 */
void print_help(std::ostream &out) {
    out << "\n";
    out << "Program version : " << version << "\n";
    out << "Usage:  spot_formula_to_dot  --sf <formula> --ff <file> \n";
    out << "        converts a formula to DOT format (~digraph), which resembles the AST\n";
    out << "        the AST is reduced: all redundant nodes are collapsed into a single node with multiple edges.\n";
    out << "Commandline options:\n";
    out << "--sf      the single LTL formula/property to convert.  \n";
    out << "--ff      filename containing multiple formulas/properties. (file shall not contain empty lines!)\n";
    out << "\n";
    out << "Use-case when only option --a is supplied (without --sf or --ff): \n";
    out << "          The user can supply via stdin a formula/property. Results are returned via stdout.\n";
    out << "          The system will ask for a new formula. A blank line will stop the program. \n";
    out << "\n";
    out << "Note:     large automatons (states,ap's) or large formula (size, ap's) \n";
    out << "          can make the program unresponsive or even time-out due to lack of memory. \n";
}

/**
 * Converts a collection of LTL formulas to DOT format
 * @param col_in            stream containing a collection of formulas
 * @param out               stream for collection the results
 *
 * delegates checking of individual formulas to \see convert_todot
 */
void collection_todot(std::istream &col_in, std::ostream &out) {
    std::string formula_result;
    std::string f;
    int i = 1;
    while (getline(col_in, f)) {
        if (f.empty()) break;
            std::ostringstream sout;
            spot::parsed_formula pf = spot::parse_infix_psl(f);
            spot::print_dot_psl(sout, pf.f);
            formula_result = sout.str();
            std::stringstream ss;
            ss << i;
            i++;
            findAndReplaceAll(formula_result, "digraph G",
                             "digraph \"AST_for_formula_"+ss.str()+": " + str_psl(pf.f)  + "\"");
            out << formula_result;
    }
}


/**
 * Entry point of the application:  Standard C/C++ routine \n
 * Schematically:\n
 * Parse the commandline\n
 * Outputs with print_help if there was an error and terminates\n
 * makes a dot output for every formula (formula must be syntactically valid \n
 *
 * @param argc
 * @param argv
 * @return      zero if the program terminates succesfully
 */
int main(int argc, char *argv[]) {
    // do stuff;
    std::string singleformula = getCmdOption(argc, argv, "--sf");
    std::string formulafile = getCmdOption(argc, argv, "--ff");

    if (singleformula.empty()) {
        if (formulafile.empty()) {
            std::cerr << "no formulas supplied via '--sf' nor '--ff'.\n";
            print_help(std::cerr);
            return 1;
        } else if (not fs::exists(formulafile)) {
            std::cerr << "formula file not found for option '--ff'.\n";
            print_help(std::cerr);
            return 1;
        } else {
            std::ifstream f_in;
            f_in.open(formulafile.c_str());
            collection_todot(f_in, std::cout);
        }

    } else {
        std::istringstream s_in;
        s_in.str(singleformula);
        collection_todot(s_in, std::cout);

    }
    return 0;
}