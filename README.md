
# spot\_checker
This utility program is a wrapper around SPOT library (https://spot.lrde.epita.fr/) for usage with TESTAR.  
The main purpose is to convert LTL formulas to finite LTL (LTLF) variants before they are model checked.  
LTLF can be use full for models with terminal states.

The application weaves an atomic proposition (AP) into the formula to label the 'alive' part of the model.  E.g. '\--ltlf !dead or \--ltlf alive'.  
The wrapper takes in a file with an automaton and (a file with) formulas and returns the results in a format that TESTAR is able to parse.  
The command is developed on  Ubuntu 18.04 and is intended to run on Windows Subsystem for Linux (WSL). WSL v1903 or higher is recommended  

Note: 
- This 'terminal' AP MUST exist in the automaton as well!!  
- Terminal states in the model shall have a self-loop with 'dead' or '!alive'  as the only property that holds
- or Terminal states always transition to an artificial terminal state with such a self-loop. 
 
LTLF variant | use case
------------ | -------- 
LTLf (G&V-2013) | for plain traces or a DAG.  
LTLfs           | for safety properties on models with terminal states.  
LTLfl           | for liveness properties* on models with terminal states. 
\* *Checked in non-trivial SCC's only and <u>NOT</u> in the finite suffix towards a terminal state*
  
Program version : 20200902

---
#### Usage:  
spot_checker &nbsp;&nbsp; \--stdin --a *file* \--sf *formula* \--ff *file* \--fonly  --ltlf  *str* \--ltlxf *str* \--witness    
Help is displayed when the spot_checker is invoked without options.
All output is send to stdout.

#### Commandline options:
Option   | mandatory | Usage
-------- | --------- | -----
\--stdin | optional  | All input is  via console:  <br> first a single automaton in [HOA format](http://adl.github.io/hoaf/) (http://adl.github.io/hoaf/) followed by formulas. <br> When completed, the system will ask for a new formula to model check. <br>A blank line will stop the program and all other arguments are ignored.
\--a     | mandatory unless <br>\--stdin is used |  filename containing the automaton (HOA format) <br> Without \--sf or \--ff, the formula/property is via stdin. 
\--sf    | optional  | the single LTL formula/property to check. Add quotes if the formula contains spaces
\--ff    | optional  | filename containing multiple formulas/properties. 
\--fonly | optional  | Does not check against a model, but only verifies the formulas  syntactically and provides LTLF versions. <br>The alive property to be  provided by \--ltlf or \--ltlxf. By absence the program  uses '!dead'. This option ignores \--a, \--witness.
\--ltlf  | optional  | the alive property: E.g.  '!dead' or 'alive'. 
\--ltlx  | optional  | without \--fonly: model check both the original formula and all the ltlf variants
\--witness| optional  | generates a trace: counterexample (for FAIL) or witness (for PASS)

---

#### Note:     

          large automatons (number of states and ap's) or large formula (size, number of ap's) 
          can make the program unresponsive or even time-out due to lack of memory. 

