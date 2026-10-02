# Reading of the OpenDocument spreadsheet input file, replicating ModelData::loadInput() and LLData.
#
# The content.xml file of the ODS archive is parsed with EzXML following exactly the same
# traversal rules of the C++ (Qt QDom based) reader:
# - tables are all the elements with local name "table" (document order)
# - rows are only the direct "table-row" children of a table (number-rows-repeated is ignored)
# - cells are only the direct "table-cell" children of a row, expanded by number-columns-repeated
# - the first row gives the headers (text of the first <text:p> child, "" if none)
# - data rows whose first cell is empty or starts with "#" are skipped
# - float cells give the full stored value (office:value), other cells the displayed text
# - columns with an empty header are dropped

"Low level table data (LLData in the C++ code)"
struct LLData
    tableName::String
    headers::Vector{String}
    records::Vector{Vector{String}}
    hIndex::Dict{String,Int}
end

function LLData(name::AbstractString, headers::Vector{String}, records::Vector{Vector{String}})
    hIndex = Dict{String,Int}()
    for (i, h) in enumerate(headers)
        haskey(hIndex, h) || (hIndex[h] = i)
    end
    return LLData(String(name), headers, records, hIndex)
end

nrecords(t::LLData) = length(t.records)
nheaders(t::LLData) = length(t.headers)

function getData(t::LLData, pos::Integer, header::AbstractString; debugLevel = MSG_CRITICAL_ERROR)
    if pos > length(t.records)
        msgOut(debugLevel, "Requested position $(pos-1) too high! Not enought records !!")
        return ""
    end
    i = get(t.hIndex, header, 0)
    if i == 0
        msgOut(debugLevel, "Header string $header not found!")
        return ""
    end
    return t.records[pos][i]
end

# --- XML helpers (local names, as QDom with namespace processing) ---

function _attr(el::EzXML.Node, name::AbstractString)
    for a in attributes(el)
        nodename(a) == name && return nodecontent(a)
    end
    return nothing
end

function _childElements(el::EzXML.Node, name::AbstractString)
    out = EzXML.Node[]
    for c in eachelement(el)
        nodename(c) == name && push!(out, c)
    end
    return out
end

function _firstChild(el::EzXML.Node, name::AbstractString)
    for c in eachelement(el)
        nodename(c) == name && return c
    end
    return nothing
end

_repeated(cell) = (r = _attr(cell, "number-columns-repeated"); r === nothing ? 1 : s2i(r))

"Value of a data cell as read by the C++ code"
function _cellValue(cell)
    p = _firstChild(cell, "p")
    p === nothing && return ""
    vt = _attr(cell, "value-type")
    if vt === nothing
        msgOut(MSG_ERROR, "Element doens't have attribute value-type")
        vt = ""
    end
    if vt == "float"
        v = _attr(cell, "value")
        if v === nothing
            msgOut(MSG_ERROR, "Element doens't have attribute value")
            return ""
        end
        return v
    end
    return nodecontent(p)
end

"Parse one <table:table> element in an LLData (or nothing if it has less than 2 rows)"
function _parseTable(tableEl)
    tableName = something(_attr(tableEl, "name"), "")
    rows = _childElements(tableEl, "table-row")
    length(rows) < 2 && return nothing

    # headers..
    allHeaders = String[]
    for cell in _childElements(rows[1], "table-cell")
        p = _firstChild(cell, "p")
        h = p === nothing ? "" : nodecontent(p)
        for _ in 1:_repeated(cell)
            push!(allHeaders, h)
        end
    end
    nAll = length(allHeaders)
    keep = [i for i in 1:nAll if allHeaders[i] != ""] # LLData::clean() removes the empty-header columns
    keepPos = Dict(k => j for (j, k) in enumerate(keep))

    records = Vector{Vector{String}}()
    for j in 2:length(rows)
        cells = _childElements(rows[j], "table-cell")
        isempty(cells) && continue
        p0 = _firstChild(cells[1], "p")
        p0 === nothing && continue        # empty line, first column empty
        startswith(nodecontent(p0), "#") && continue  # comment
        record = fill("", length(keep))
        pos = 0
        for cell in cells
            rep = _repeated(cell)
            # only evaluate the cell content if it is needed by a kept column
            needed = false
            for q in 1:rep
                if haskey(keepPos, pos + q)
                    needed = true
                    break
                end
            end
            if needed
                v = _cellValue(cell)
                for q in 1:rep
                    k = get(keepPos, pos + q, 0)
                    k > 0 && (record[k] = v)
                end
            end
            pos += rep
        end
        if pos != nAll
            msgOut(MSG_CRITICAL_ERROR, "Error in the input reading table $tableName. Record $(length(records)) has $pos fields instead of $nAll.")
        end
        push!(records, record)
    end
    return LLData(tableName, allHeaders[keep], records)
end

"ModelData::getTableFromFile(): load a ';' separated csv file as table"
function getTableFromFile(tablename::AbstractString, filename::AbstractString)
    headers = String[]
    records = Vector{Vector{String}}()
    if !isfile(filename)
        msgOut(MSG_ERROR, "Cannot open file $filename for reading.")
        return LLData(tablename, headers, records)
    end
    countRow = 0
    for line in eachline(filename)
        line == "" && continue
        fields = split(line, ';')
        if countRow == 0
            headers = String.(fields)
        else
            startswith(fields[1], "#") && continue
            record = [replace(String(f), ',' => '.') for f in fields]
            all(isempty, record) || push!(records, record)
        end
        countRow += 1
    end
    for (i, r) in enumerate(records)
        if length(r) != length(headers)
            msgOut(MSG_CRITICAL_ERROR, "Error in the input reading table $tablename. Record $(i-1) has $(length(r)) fields instead of $(length(headers)).")
        end
    end
    keep = [i for i in eachindex(headers) if headers[i] != ""]
    return LLData(tablename, headers[keep], [r[keep] for r in records])
end

"""
    loadInput(inputFile, baseDirectory)

Read all the tables of the ODS input file (replacing forData and prodData with the cached csv
versions in `cachedInput/` if these exist, as in the C++ code).
"""
function loadInput(inputFile::AbstractString, baseDirectory::AbstractString)
    msgOut(MSG_INFO, "Loading input files (this can take a few minutes)...")
    isfile(inputFile) || println("File does not exist.\n")
    zr = ZipReader(read(inputFile))
    content = zip_readentry(zr, "content.xml", String)
    doc = parsexml(content)
    forDataCached = joinpath(baseDirectory, "cachedInput", "forData.csv")
    tables = LLData[]
    for tEl in findall("//*[local-name()='table']", root(doc))
        tableName = something(_attr(tEl, "name"), "")
        if (tableName == "forData" || tableName == "prodData") && isfile(forDataCached)
            msgOut(MSG_INFO, "Attenction, using cached data (csv) for $tableName")
            push!(tables, getTableFromFile(tableName, joinpath(baseDirectory, "cachedInput", tableName * ".csv")))
            continue
        end
        t = _parseTable(tEl)
        t === nothing || push!(tables, t)
    end
    return tables
end
