using DataFrames

"""
    pivot(df, valuesField, rowFields, colField, op )

Return a new DataFrame pivotted and filtered according to the passed arguments

# Arguments
* `df::DataFrame`: the original dataframe, in stacked version (dim1,dim2,dim3... value)
* `valuesField::Symbol`: column containing the values to represent in the oputput DataFrame
* `rowFields::AbstractVector`: a vector of two-item tuples containing the row field and an optional array of values to retrain
* `colField::AbstractVector`: a vector containing the column field (only one possible) and an optional array of values to retrain
* `op::Symbol=:sum`: the operation to perform on the data, default to summing them (see notes)

# Notes
* filters are optional but the argument must be provided as a tuple
* op can be any supported Julia operation over a single array, for example: sum, :mean, :length, :countnz, :maximum, :minimum, :var, :std, :prod


# Examples
```julia
julia> df = DataFrame(region = ["US","US","US","US","EU","EU","EU","EU"],
               product = ["apple","apple","banana","banana","apple","apple","banana","banana"],
               year = [2010,2011,2010,2011,2010,2011,2010,2011],
               produced = [3.3,3.2,2.3,2.1,2.7,2.8,1.5,1.3],
               consumed = [4.3,7.4,2.5,9.8,3.2,4.3,6.5,3.0])
julia> long = stack(df,[:produced,:consumed])
julia> pivottedDF = pivot(df,:produced,[(:region,)],[:year,[2010,2011]])
2×3 DataFrames.DataFrame
│ Row │ region │ 2010 │ 2011 │
├─────┼────────┼──────┼──────┤
│ 1   │ "EU"   │ 4.2  │ 4.1  │
│ 2   │ "US"   │ 5.6  │ 5.3  │
```
"""
function pivot(df::DataFrame, valuesField::Symbol, rowFields::AbstractVector, colField::AbstractVector, op::Symbol=:sum)
    rowFieldNames   = Symbol[]
    colFieldName    = colField[1]

    for i in rowFields
        push!(rowFieldNames,i[1])
        if length(i) > 1 && ! isempty(i[2])
            df = df[ [j in i[2] for j in df[i[1]] ], :]
        end
    end
    if length(colField) > 1 && ! isempty(colField[2])
        df = df[ [j in colField[2] for j in df[colField[1]] ], :]
    end

    catFields = cat(1,rowFieldNames, colFieldName)
    df = by(df, catFields) do df2
        a = DataFrame()
        a[valuesField] = eval(op)(df2[valuesField])
        a
    end
    df = unstack(df,colFieldName,valuesField)

    return df
end
