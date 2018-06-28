using DataArrays, DataFrames, DataFramesMeta, Plots, StatPlots, DataStructures, LAJuliaUtils

function fillAll(iF,oF,d=",")
    open(iF, "r") do i
        open(oF, "w") do o # "w" for writing
            headerRow = strip(readline(i))
            headers = split(headerRow,d)
            nCols =  length(headers)
            write(o, headerRow*"\n")
            for ln in eachline(i)
                nFields = length(split(strip(ln),d))
                write(o, strip(ln))
                [write(o,d) for y in 1:nCols-nFields] # write delimiters
                write(o,"\n")
            end
        end
    end
end

fillAll("debugs/debugPxValues_debug.csv","debugs/debugPxValues_out.csv",";")





#delete!(debugData, [:freeDim])
