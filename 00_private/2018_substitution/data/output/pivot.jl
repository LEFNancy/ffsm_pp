using DataFrames

##############################################################################
##
## pivot()
##
##############################################################################
"""
Pivot and optionally filter and sort in a single function

```julia
pivot(df::AbstractDataFrame, rowFields, colField, valuesField; <keyword arguments>)
```

# Arguments
* `df::AbstractDataFrame`: the original dataframe, in stacked version (dim1,dim2,dim3... value)
* `rowFields`:             the field(s) to be used as row categories (also known as IDs or keys)
* `colField::Symbol`:      the field containing the values to be used as column headers
* `valuesField::Symbol`:   the column containing the values to reshape
* `ops=sum`:               the operation(s) to perform on the data, default on summing them (see notes)
* `filter::Dict`:          an optional filter, in the form of a dictionary of column_to_filter => [list of ammissible values]
* `sort`:                  optional row field(s) to sort (see notes)

# Notes
* ops can be any supported Julia operation over a single array, for example: `sum`, `mean`, `length`, `countnz`, `maximum`, `minimum`, `var`, `std`, `prod`.
  Multiple operations can be specified using an array, and in such case an additional column is created to index them.
* filters are optional. Only `in` filter is supported.
* sort is possible only for row fields. You can specify reverse ordering for any column passing a touple (:colname, true) instead of just :colname.
  You can pass multiple columns to be sorted in an array, e.g. [(:col1,true),:col2].

# Examples
```julia
julia> df = DataFrame(region   = ["US","US","US","US","EU","EU","EU","EU","US","US","US","US","EU","EU","EU","EU"],
                      product  = ["apple","apple","banana","banana","apple","apple","banana","banana","apple","apple","banana","banana","apple","apple","banana","banana"],
                      year     = [2010,2011,2010,2011,2010,2011,2010,2011,2010,2011,2010,2011,2010,2011,2010,2011],
                      produced = [3.3,3.2,2.3,2.1,2.7,2.8,1.5,1.3,  4.3,4.2,3.3,2.3,3.7,3.8,2.0,3.3],
                      consumed = [4.3,7.4,2.5,9.8,3.2,4.3,6.5,3.0,  5.3,7.4,3.5,9.8,4.2,6.3,8.5,4.0],
                      category = ['A','A','A','A','A','A','A','A', 'B','B','B','B','B','B','B','B',])
julia> longDf = stack(df,[:produced,:consumed])
julia> pivDf  = pivot(longDf, [:product, :region,], :year, :value,
                      ops    = [mean, var],
                      filter = Dict(:variable => [:consumed]),
                      sort   = [:product, (:region, true)]
                     )
 8×5 DataFrames.DataFrame
 │ Row │ product  │ region │ op     │ 2010 │ 2011 │
 ├─────┼──────────┼────────┼────────┼──────┼──────┤
 │ 1   │ "apple"  │ "US"   │ "mean" │ 4.8  │ 7.4  │
 │ 2   │ "apple"  │ "US"   │ "var"  │ 0.5  │ 0.0  │
 │ 3   │ "apple"  │ "EU"   │ "mean" │ 3.7  │ 5.3  │
 │ 4   │ "apple"  │ "EU"   │ "var"  │ 0.5  │ 2.0  │
 │ 5   │ "banana" │ "US"   │ "mean" │ 3.0  │ 9.8  │
 │ 6   │ "banana" │ "US"   │ "var"  │ 0.5  │ 0.0  │
 │ 7   │ "banana" │ "EU"   │ "mean" │ 7.5  │ 3.5  │
 │ 8   │ "banana" │ "EU"   │ "var"  │ 2.0  │ 0.5  │
```
"""
function pivot(df::AbstractDataFrame, rowFields, colField::Symbol, valuesField::Symbol; ops=sum, filter::Dict=Dict(), sort=[])

    for (k,v) in filter
      df = df[ [i in v for i in df[k]], :]
    end

    sortv = []
    sortOptions = []
    if(isa(sort, Array))
        sortv = sort
    else
        push!(sortv,sort)
    end
    for i in sortv
        if(isa(i, Tuple))
          push!(sortOptions, order(i[1], rev = i[2]))
        else
          push!(sortOptions, order(i))
        end
    end

    catFields::AbstractVector{Symbol} = cat(1,rowFields, colField)
    dfs  = DataFrame[]
    opsv =[]
    if(isa(ops, Array))
        opsv = ops
    else
        push!(opsv,ops)
    end

    for op in opsv
        dft = by(df, catFields) do df2
            a = DataFrame()
            a[valuesField] = op(df2[valuesField])
            if(length(opsv)>1)
                a[:op] = string(op)
            end
            a
        end
        push!(dfs,dft)
    end

    df = vcat(dfs)
    df = unstack(df,colField,valuesField)
    sort!(df, cols = sortOptions)
    return df
end
