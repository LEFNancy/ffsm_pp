using  DelimitedFiles

noValue = -9999
noValueFloat = -9999.0
smPx_x = 136
smPx_y = 47

# Distances in pixel units of each weight group
dists = [0,1,2,3,4,5,6,7,8,9]
# Weigths for each pixel, as function of the distance expressed in pixel units, for the diffused scenario:
weigthsDiff = [1.2,1.15,1.1, 1.08,1.06,1.04,1.02,1.01,1,0.98]
# Weigths for each pixel, as function of the distance expressed in pixel units, for the concentrated scenario:
weigthsConc = [1.4,1.25,1.15,1.12,1.06,1.02,1.00,0.98,1,0.97]

header = "north: 3168000
south: 1992000
east: 4512000
west: 2976000
rows: 147
cols: 192
"
avalCoef_baseFileName         = "spatialParameters/avalCoef###.grd"
avalCoef_diffusedFileName     = "spatialParametersWithSawmill_diffused/avalCoef#Fut_Hetre##.grd"
avalCoef_concentratedFileName = "spatialParametersWithSawmill_concentrated/avalCoef#Fut_Hetre##.grd"

origAvCoef = readdlm(avalCoef_baseFileName, Float64, skipstart=6)
# Manually handing of missing, it's quicker in this simple case!
#origAvCoef = convert(Array{Union{Float64,Missing},2},origAvCoef)
#[origAvCoef[i,j] = missing for i in 1:size(origAvCoef)[1], j in 1:size(origAvCoef)[2] if origAvCoef[i,j] == novalue]
diffAvCoef = copy(origAvCoef)
concAvCoef = copy(origAvCoef)

for y in 1:size(origAvCoef)[1], x in 1:size(origAvCoef)[2]
    dist = ((x-smPx_x)^2+(y-smPx_y)^2)^(1/2)
    wD = weigthsDiff[end]
    wC = weigthsConc[end]
    for (i,d) in enumerate(dists)
        if dist < d
            wD = weigthsDiff[i-1]
            wC = weigthsConc[i-1]
            break
        end
    end
    if(origAvCoef[y,x] != noValue)
        diffAvCoef[y,x] = min(origAvCoef[y,x]*wD,1.0)
        concAvCoef[y,x] = min(origAvCoef[y,x]*wC,1.0)
    end

    # Debug..
    #if x in 134:138 && y in 45:50
    #    println("x: $x, y: $y, dist: $dist, wD: $wD, wC: $wC, origAvCoef[y,x]: $(origAvCoef[y,x]), diffAvCoef[y,x]: $(diffAvCoef[y,x])")
    #end
end


open(avalCoef_diffusedFileName, "w") do io
             write(io, header)
             writedlm(io,  diffAvCoef, ' ')
end
open(avalCoef_concentratedFileName, "w") do io
             write(io, header)
             writedlm(io,  concAvCoef, ' ')
end
txt = read(avalCoef_diffusedFileName, String)
open(avalCoef_diffusedFileName, "w") do f
   write(f, replace(txt, "$noValueFloat" => "$noValue"))
end
txt = read(avalCoef_concentratedFileName, String)
open(avalCoef_concentratedFileName, "w") do f
   write(f, replace(txt, "$noValueFloat" => "$noValue"))
end
