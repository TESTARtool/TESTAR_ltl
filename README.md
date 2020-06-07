# spot_checker


- This LTL modelchecker is a wrapper for usage with TESTAR.
  - The code of the model checker is from the SPOT library (https://spot.lrde.epita.fr/).
  - The wrapper takes in a file with an automaton and (a file with) formulas
  - and returns the results in a format that TESTAR is able to parse.
  - The command is developed on  Ubuntu 18.04 and is inteded to run on Windows Subsystem for Linux (WSL).
  - WSL v1903 or higher is recommended.
  

Program version : 20200224

---
#### Usage:  
spot_checker &nbsp;&nbsp; \--stdin --a *file* \--sf *formula* \--ff *file* \--fonly  --ltlf  *str* \--ltl2f *str* \--witness  *file*


#### Commandline options:

##### \--stdin   
optional.  
*  all input is  via standard input stream: 
*  at first an automaton  [HOA format](http://adl.github.io/hoaf/) . 'EOF_HOA' + newline  marks the end of the automaton.
*  followed by formulas.
*  all other arguments are ignored and output is via stdout.

##### \--a       
mandatory.  
unless --stdin is the argument. filename containing the automaton (HOA format). 


##### \--sf      
optional.  
the single LTL formula/property to check.  


##### \--ff      
optional.  
filename containing multiple formulas/properties. 


##### \--fonly       
optional.  
Does not check against a model, but only verifies the formulas --sf or --ff syntactically and provides LTLF version.    
    (the alive property is taken from --ltlf or --ltl2f and by absence the program  uses '!dead'). this option ignores --a, --witness.

##### \--ltlf    
optional.  
usable for finite LTL (if the automaton contains terminal states): 
          Weaves an atomic proposition(AP) into the formula to label the 'alive' part. 
          e.g. '--ltlf !dead or --ltlf alive' . Note: this AP MUST exist in the automaton as well!!    
          Terminal states in the model shall have a self-loop with 'dead' or '!alive' as the only property
          or always transition to an artificial terminal state with such a self-loop

##### \--ltl2f   
optional.  
same as --ltlf but checks **both** the original formula and the ltlf variant

##### \--witness 
optional.  
generates a trace: counterexample (for FAIL) or witness (for PASS)

---
#### Use-case when only option --a is supplied (without --sf or --ff): 

          The user can supply via stdin a formula/property. Results are returned via stdout.
          The system will ask for a new formula. A blank line will stop the program. 

#### Note:     

          large automatons (number of states and ap's), large formula (size, number of ap's) 
          can make the program unresponsive or even time-out due to lack of memory. 

