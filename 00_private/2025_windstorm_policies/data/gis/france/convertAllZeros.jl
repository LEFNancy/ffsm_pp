using DelimitedFiles
cd(@__DIR__)
ifile = "additionalMortality###2011_input.grd"

 m = readdlm(ifile, skipstart=6)
 
 m2 = [m[r,c] > -10 ? 0.0 : m[r,c] for r in 1:size(m,1), c in 1:size(m,2)]

 m2 = convert(Matrix{Int64},m2)

 writedlm("test.csv",m2)