"""Reading of the OpenDocument spreadsheet input file, replicating ModelData::loadInput() and LLData.

The content.xml file of the ODS archive is parsed with ElementTree following exactly the same traversal
rules of the C++ (Qt QDom based) reader:
- tables are all the elements with local name "table" (document order)
- rows are only the direct "table-row" children of a table (number-rows-repeated is ignored)
- cells are only the direct "table-cell" children of a row, expanded by number-columns-repeated
- the first row gives the headers (text of the first <text:p> child, "" if none)
- data rows whose first cell is empty or starts with "#" are skipped
- float cells give the full stored value (office:value), other cells the displayed text
- columns with an empty header are dropped
"""

import os
import zipfile
import xml.etree.ElementTree as ET

from .utils import msgOut, s2i, MSG_INFO, MSG_ERROR, MSG_CRITICAL_ERROR


class LLData:
    """Low level table data (LLData in the C++ code)"""
    __slots__ = ("tableName", "headers", "records", "hIndex")

    def __init__(self, name, headers, records):
        self.tableName = name
        self.headers = headers
        self.records = records
        self.hIndex = {}
        for i, h in enumerate(headers):
            self.hIndex.setdefault(h, i)


def nrecords(t):
    return len(t.records)


def nheaders(t):
    return len(t.headers)


def getData(t, pos, header, debugLevel=MSG_CRITICAL_ERROR):
    """Value of record `pos` (0-based) in the column `header`"""
    if pos >= len(t.records):
        msgOut(debugLevel, f"Requested position {pos} too high! Not enought records !!")
        return ""
    i = t.hIndex.get(header)
    if i is None:
        msgOut(debugLevel, f"Header string {header} not found!")
        return ""
    return t.records[pos][i]


# --- XML helpers (local names, as QDom with namespace processing) ---

def _local(tag):
    return tag.rsplit("}", 1)[-1] if isinstance(tag, str) else ""


def _attr(el, name):
    for k, v in el.attrib.items():
        if _local(k) == name:
            return v
    return None


def _childElements(el, name):
    return [c for c in el if _local(c.tag) == name]


def _firstChild(el, name):
    for c in el:
        if _local(c.tag) == name:
            return c
    return None


def _text(el):
    return "".join(el.itertext())


def _repeated(cell):
    r = _attr(cell, "number-columns-repeated")
    return 1 if r is None else s2i(r)


def _cellValue(cell):
    """Value of a data cell as read by the C++ code"""
    p = _firstChild(cell, "p")
    if p is None:
        return ""
    vt = _attr(cell, "value-type")
    if vt is None:
        msgOut(MSG_ERROR, "Element doens't have attribute value-type")
        vt = ""
    if vt == "float":
        v = _attr(cell, "value")
        if v is None:
            msgOut(MSG_ERROR, "Element doens't have attribute value")
            return ""
        return v
    return _text(p)


def _parseTable(tableEl):
    """Parse one <table:table> element in an LLData (or None if it has less than 2 rows)"""
    tableName = _attr(tableEl, "name") or ""
    rows = _childElements(tableEl, "table-row")
    if len(rows) < 2:
        return None

    # headers..
    allHeaders = []
    for cell in _childElements(rows[0], "table-cell"):
        p = _firstChild(cell, "p")
        h = "" if p is None else _text(p)
        allHeaders.extend([h] * _repeated(cell))
    nAll = len(allHeaders)
    keep = [i for i in range(nAll) if allHeaders[i] != ""]   # LLData::clean() removes the empty-header columns
    keepPos = {k: j for j, k in enumerate(keep)}

    records = []
    for row in rows[1:]:
        cells = _childElements(row, "table-cell")
        if not cells:
            continue
        p0 = _firstChild(cells[0], "p")
        if p0 is None:                   # empty line, first column empty
            continue
        if _text(p0).startswith("#"):    # comment
            continue
        record = [""] * len(keep)
        pos = 0
        for cell in cells:
            rep = _repeated(cell)
            # only evaluate the cell content if it is needed by a kept column
            ks = [keepPos[pos + q] for q in range(rep) if (pos + q) in keepPos]
            if ks:
                v = _cellValue(cell)
                for k in ks:
                    record[k] = v
            pos += rep
        if pos != nAll:
            msgOut(MSG_CRITICAL_ERROR, f"Error in the input reading table {tableName}. Record {len(records)} has {pos} fields instead of {nAll}.")
        records.append(record)
    return LLData(tableName, [allHeaders[i] for i in keep], records)


def getTableFromFile(tablename, filename):
    """ModelData::getTableFromFile(): load a ';' separated csv file as table"""
    headers = []
    records = []
    if not os.path.isfile(filename):
        msgOut(MSG_ERROR, f"Cannot open file {filename} for reading.")
        return LLData(tablename, headers, records)
    countRow = 0
    with open(filename, encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\r\n")
            if line == "":
                continue
            fields = line.split(";")
            if countRow == 0:
                headers = fields
            else:
                if fields[0].startswith("#"):
                    continue
                record = [x.replace(",", ".") for x in fields]
                if any(record):
                    records.append(record)
            countRow += 1
    for i, r in enumerate(records):
        if len(r) != len(headers):
            msgOut(MSG_CRITICAL_ERROR, f"Error in the input reading table {tablename}. Record {i - 1} has {len(r)} fields instead of {len(headers)}.")
    keep = [i for i, h in enumerate(headers) if h != ""]
    return LLData(tablename, [headers[i] for i in keep], [[r[i] for i in keep] for r in records])


def loadInput(inputFile, baseDirectory):
    """Read all the tables of the ODS input file (replacing forData and prodData with their cached csv
    versions cachedInput/forData.csv and cachedInput/prodData.csv when these exist)."""
    msgOut(MSG_INFO, "Loading input files (this can take a few minutes)...")
    if not os.path.isfile(inputFile):
        print("File does not exist.\n")
    with zipfile.ZipFile(inputFile) as z:
        content = z.read("content.xml")
    root = ET.fromstring(content)
    tables = []
    for tEl in root.iter():
        if _local(tEl.tag) != "table":
            continue
        tableName = _attr(tEl, "name") or ""
        cached = os.path.join(baseDirectory, "cachedInput", tableName + ".csv")
        if tableName in ("forData", "prodData") and os.path.isfile(cached):
            msgOut(MSG_INFO, f"Attenction, using cached data (csv) for {tableName}")
            tables.append(getTableFromFile(tableName, cached))
            continue
        t = _parseTable(tEl)
        if t is not None:
            tables.append(t)
    return tables
