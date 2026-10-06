# Command line entry point of the Julia port of FFSM++, e.g.:
#   julia --project=. ffsm.jl -s default
#   julia --project=. ffsm.jl -i data/ffsmInput.ods -s debug

using FFSM
exit(FFSM.main(ARGS))
