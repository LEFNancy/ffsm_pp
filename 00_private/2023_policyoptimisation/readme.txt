obj: find the policy (region, type, year) that cost the less in order to obtain the desired harvesting increase

maximise the following lagrangian in x (policy levels) and lambda: -cost(x) + lambda(harvestinglevels(x)-sesiderHLevels) 


Instead of doing directly, make random levels and run 1-2k simulations, collect the outputs and make 2 ML models (cost and harvesing) and then maximise the approximated function given by the ML model using a global optimizer that doesn't require differentiation (see https://discourse.julialang.org/t/approaches-to-find-optimal-x-on-large-simulation-model-when-you-have-only-x-y-no-gradient-hessian/86925/2)


