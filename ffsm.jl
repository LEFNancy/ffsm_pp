# Command line entry point of the Julia port of FFSM++, e.g.:
#   julia --project=. ffsmj.jl -s default
#   julia --project=. ffsmj.jl -i data/ffsmInput.ods -s debug

using FFSMj
exit(FFSMj.main(ARGS))
