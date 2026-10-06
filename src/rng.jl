# Random numbers of the model.
#
# All the random draws of a run (the per-pixel spatial modifiers, expectation types and risk aversion, and the
# optional initial shuffle of the layers) come from one generator, `m.rng`: a `StableRNG` (StableRNGs.jl),
# whose streams are guaranteed not to change between Julia versions, used by the Distributions.jl samplers (whose
# version is fixed by the Manifest). It is seeded with the `randomSeed` setting, so that two runs of the same
# scenario give the same results, unless `newRandomSeed` is true: then the seed is drawn from the system entropy and every run is different (the seed is printed in the log, to
# reproduce the run by setting it as `randomSeed`).

"""
    initRng!(m)

Seed the random number generator of the model: with the `randomSeed` setting, or with a random seed when the
`newRandomSeed` setting is true (in that case a random number is also appended to the scenario name, as in the
C++ code, so that the outputs of different runs don't overwrite each other).
"""
function initRng!(m)
    if getBoolSetting(m, "newRandomSeed")
        seed = rand(Random.RandomDevice(), UInt32)
        m.rng = StableRNG(seed)
        m.scenarioName = m.scenarioName * "_" * i2s(rand(m.rng, 1:1_000_000))
    else
        seed = getIntSetting(m, "randomSeed")
        seed < 0 && msgOut(MSG_CRITICAL_ERROR, "The randomSeed setting must be a non negative integer (got $seed).")
        m.rng = StableRNG(seed)
    end
    msgOut(MSG_INFO, "Random seed: $seed")
    return m.rng
end

"""
    truncatedNormalDraw(rng, μ, σ, lower, upper)

A draw from a normal distribution truncated to [lower, upper] (Distributions.jl). With σ = 0 it returns μ
(clamped to the bounds): Distributions' sampler never ends for a degenerate distribution on a bound
(e.g. `truncated(Normal(0, 0), 0, 1)`).
"""
truncatedNormalDraw(rng, μ::Real, σ::Real, lower::Real, upper::Real) =
    σ == 0 ? clamp(float(μ), lower, upper) : rand(rng, truncated(Normal(μ, σ), lower, upper))
