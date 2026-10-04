# Porting FFSM++ to Python: feasibility

**Verdict (before the port):** feasible, at roughly the speed of the C++ binary (not 5–6× faster like the Julia
port). The reasons to have it are the wider familiarity of Python among the model users and a realistic GUI
path (PySide6 is the official Qt binding, the C++ GUI is Qt). The Julia port remains the main port.

The port was then done (`ffsmp/`, see [port_status.md](port_status.md)); the measured figures are at the end.

## Where the time goes

Profile of the Julia port on the `default` scenario (whole France, 6 simulated years):

| Part | Share | Consequence for Python |
|---|---|---|
| pixel-level modules (`computeCumulativeData`, biological, management, exogenous cache, regional sums) | ~66% | interpreted loops: the slow part |
| market (model building, derivatives, Ipopt) | ~27% | compiled code (Ipopt and the AD tool): about the same speed |
| initialisation, input | ~7–11% | XML and arrays from C libraries |

A transcription of the hottest loop (`computeCumulativeData`, 8,580 pixels × 6 forest types × 14 diameter classes
= 720,720 iterations per year on `default`) ran at 0.97 µs per iteration in Julia and 3.5 µs in pure CPython
3.14: the loop is dominated by keyed look-ups, which CPython dicts do well. With more arithmetic per look-up in the
other modules, a factor of 3–10 on the pixel part was expected, i.e. about 15–40 minutes for `default` (C++: 33
minutes, Julia: 5).

The `default` market problem has about 1,900 variables and 2,100 constraints: building it in Python every year is
cheap.

## Components

| Component | Python | Compared with Julia |
|---|---|---|
| ODS reading | `zipfile` + `ElementTree` | same |
| bit-exact maths | CPython's `math` calls the system libm (glibc), as C++ | simpler (Julia needed a binding to glibc) |
| random numbers | replica of `std::mt19937`, `generate_canonical`, `normal_distribution` | same |
| market | Pyomo + Ipopt (or CasADi) | equivalent to JuMP |
| maps, png, csv | `array`, `zlib`, string formatting | same |
| GUI | PySide6 could translate the Qt GUI almost one to one (~1–2 weeks) | much better |
| contributors | far more people know Python | better |
| robustness | dynamic typing, more memory | worse |

## Market library: CasADi or Pyomo

The port was first done with [CasADi](https://web.casadi.org) (symbolic expressions and algorithmic
differentiation, pip wheels that include Ipopt and MUMPS, the problem built once with the yearly data as
parameters). It was then rewritten with [Pyomo](https://www.pyomo.org) for readability: an indexed algebraic model
(sets of regions and products, named constraints) close to the original GAMS formulation and known by many more
modellers, at the cost of an external `ipopt` executable and of rebuilding the model every year (~2 s per year on
`default`). Both versions matched the C++ outputs equally well.

## Measured after the port

| | C++ | Julia | Python (Pyomo) |
|---|---|---|---|
| `default`, 100 years | 35m17s | 5m12s | 25m01s |
| test scenario (2 regions, 8 years) | ~45 s | ~6 s | ~27 s |
| test scenario with Hartman payments | ~45 s | ~15–25 s | ~70–80 s |

The Python code has about the same size as the Julia one (4,640 and 4,600 lines without comments and docstrings;
870 more lines of tests).

A cheaper alternative, if Python users only need to script the model, is to call the Julia port from Python
with `juliacall` (`from juliacall import Main as jl; jl.seval("using FFSMj")`).
