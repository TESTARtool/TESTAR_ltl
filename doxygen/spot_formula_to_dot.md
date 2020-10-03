# spot_formula_to_dot
- This utility program is a CLI to convert  LTL formulas to Graphviz .dot format.
- The code built on top of the 'print_to_dot' function in the SPOT library (https://spot.lrde.epita.fr/).
- The command is developed on  Ubuntu 18.04 and is intended to run on Windows Subsystem for Linux (WSL).
- WSL v1903 or higher is recommended.
  
Program version : 20200925

#### Use-case: 

A LTL formula is converted to an Abstract Syntax Tree (AST). The AST, actually a reduced tree (a
 DAG) is generated in .dot format.   
The dot format facilitates the user to print (PDF, PNG) or analyze the AST with Graphviz commands.  
The output of the utility is a single .dot file, but the content can consist of  multiple graphs!  

---
#### Usage:  
spot_formula_to_dot &nbsp;&nbsp;  \--sf *formula* \--ff *file*  
Help is displayed when the application is invoked without options.
All output is send to stdout.


#### Commandline options:
Option   | mandatory | Usage
-------- | --------- | -----
\--sf    | optional  | the single LTL formula/property to check. Add quotes if the formula contains spaces
\--ff    | optional  | filename containing multiple formulas/properties. <br>The file requires Unix style line separators (LineFeed) and can ***NOT*** contain empty lines.

---
