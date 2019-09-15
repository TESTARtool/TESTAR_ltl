//
// Created by css on 15-9-19.
//
#include <iostream>
#include <spot/tl/parse.hh>
#include <spot/tl/ltlf.hh>
#include <spot/twaalgos/translate.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twaalgos/remprop.hh>
#include <spot/tl/print.hh>

int main()
{
    spot::parsed_formula pf = spot::parse_infix_psl("(a U b) & Fc");
    if (pf.format_errors(std::cerr))
        return 1;

    spot::translator trans;
    trans.set_type(spot::postprocessor::BA);
    trans.set_pref(spot::postprocessor::Small);
    spot::twa_graph_ptr aut = trans.run(spot::from_ltlf(pf.f, "!dead"));

    spot::formula f = spot::from_ltlf(pf.f, "!dead");//f = spot::parse_formula(formula);
    std::cout << "showing formula translations for :"<<f << '\n';
    std::cout << "'-----------------------\n";
    std::cout << "LATEX : "; print_latex_psl(std::cout, f) << '\n';
    std::cout << "LBT   : ";print_lbt_ltl(std::cout, f) << '\n';
    std::cout << "SPIN  : ";print_spin_ltl(std::cout, f, true) << '\n';
    std::cout << '\n';
/*

    spot::remove_ap rem;
    rem.add_ap("alive");
    aut = rem.strip(aut);

    spot::postprocessor post;
    post.set_type(spot::postprocessor::BA);
    post.set_pref(spot::postprocessor::Small); // or ::Deterministic
    aut = post.run(aut);
*/

    print_hoa(std::cout, aut) << '\n';
    return 0;
}

