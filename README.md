# spot-checker


-This LTL modelchecker is a wrapper for usage with TESTAR.

code is based on SPOT library (https://spot.lrde.epita.fr/) .

Program version : 20200204

#
Usage:  spot_checker --stdin --a `<file>` --sf `<formula>` --ff `<file>` --fonly --ltlf `<ap>` --ltl2f `<ap>` --witness  <file>

Commandline options:

--stdin   
    all input is  via standard input stream: first an automaton (HOA format) followed by formulas.
    'EOF_HOA' + <enter>  mark the end of the automaton.
    all other arguments are ignored and output is via stdout.

--a       
mandatory unless --stdin is the argument. filename containing the automaton (HOA format). 

--sf      
optional.  the single LTL formula/property to check.  

--ff      
optional.  filename containing multiple formulas/properties. 


--fonly 
optional.  does not check against a model, but only verifies the formulas syntactically and
provides ltlf version (by using  --ltlt !dead). outputs all formulas to the output.
this option ignores --a, --ltlf --ltl2f,--witness.

--ltlf    
optional.  usable for finite LTL (if the automaton contains dead states): 
          Weaves an atomic proposition into the formula to label the 'alive' part. 
          e.g. '--ltlf !dead or --ltlf alive' . Note: this AP MUST exist in the automaton as well!!
          terminal states in the model shall have a self-loop with AP='dead' or '!alive' or
          always transition to a(n artificial) dead-state with such a self-loop

--ltl2f   
optional.  same as --ltlf but checks both the original formula and the ltlf variant

--witness 
optional.  generates a trace: counterexample( for FAIL)or witness (for PASS)


Use-case when only option --a is supplied (without --sf or --ff): 

          The user can supply via stdin a formula/property. Results are returned via stdout.
          The system will ask for a new formula. A blank line will stop the program. 

Note:     large automatons (states,ap's), large formula (size, ap's) 
          can make the program unresponsive or even time-out due to lack of memory. 

