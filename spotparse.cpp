#include <iostream>
#include <spot/tl/parse.hh>
#include <spot/tl/print.hh>


void do_it(std::string formula){
    spot::formula f = spot::parse_formula(formula);
    std::cout << "showing formula translations for :"<<f << '\n';
    std::cout << "'-----------------------\n";
    std::cout << "LATEX : "; print_latex_psl(std::cout, f) << '\n';
    std::cout << "LBT   : ";print_lbt_ltl(std::cout, f) << '\n';
    std::cout << "SPIN  : ";print_spin_ltl(std::cout, f, true) << '\n';
    std::cout << '\n';
}
int main(int argc, char *argv[])
{
    std::string inputformula;
    std::string testformula;
    std::cout << "Hi,  Greetings from CSS"<< '\n'<< '\n';

    if (argc<2){

        testformula = "[]<>p0 || <>[]p1";
        do_it(testformula);
        testformula = "& & G p0 p1 p2";
        do_it(testformula);


        std::cout << "Enter a formula like the ones  above ("<<testformula<<')'<<'\n';
        std::cout << "and i give some alternative notations as output:"<< '\n';
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