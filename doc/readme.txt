FFSM++ documentation is comprised of:
   • The Reference Manual
   • The Papers


NO: All up-to-date documentation is available at http://www.ffsm.org/doc



*** GIT USERS ***
To create the Reference Manual locally type the following commands from a SHELL/DOS prompt:
(Reference Manual)
doxygen ffsm_pp.doxygen

TODO:
(User Manual)
cd userManual
pdflatex -shell-escape -interaction=nonstopmode regmasUserManual.tex
bibtex regmasUserManual
pdflatex -shell-escape -interaction=nonstopmode regmasUserManual.tex

PS: To generate the documentation you need to have installed Doxygen (http://www.doxygen.org), Graphviz (http://www.graphviz.org) and a modern distribution of LaTeX.


