#include <iostream>
#include <spot/tl/parse.hh>
#include <spot/tl/print.hh>
#include <spot/tl/length.hh>
#include <spot/twaalgos/translate.hh>


void do_it(std::string formula){
    spot::formula f = spot::parse_formula(formula);
    std::cout <<"LTL formula parser version 20191221\n";
    std::cout <<"Showing formula translations for << "<<formula <<" >> which has a length of "<< spot::length(f)<< ":\n";
    std::cout << "LATEX : "; print_latex_psl(std::cout, f) << '\n';
    std::cout << "LBT   : ";print_lbt_ltl(std::cout, f) << '\n';
    std::cout << "SPIN  : ";print_spin_ltl(std::cout, f, true) << '\n';
    std::cout << "PSL   : ";spot::print_psl(std::cout, f, true) << '\n';

}
int main(int argc, char *argv[])
{
    std::string inputformula;
    std::string testformula;
    if (argc<2){
        testformula = "G(a->Fb)";
        do_it(testformula);
        std::cout << "Enter a formula in one of the syntaxes above:"<< '\n';
        getline(std::cin , inputformula);
        do_it(inputformula);
        std::cout << '\n'<<"Thanks. (You can also add the formula as a commandline option)"<<'\n';
    }else{
        for (int i = 1; i<argc; i++) {
            inputformula += argv[i];
            if (i != argc-1) //this check prevents adding a space after last argument.
                inputformula += " ";
        }
        do_it(inputformula);
    }
    return 0;
}