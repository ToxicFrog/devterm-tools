local function blank_col(bm, col)
  for _,row in ipairs(bm) do
    if row[col] == 1 then return false end
  end
  return true
end

local function prune_col(bm, col)
  for _,row in ipairs(bm) do
    table.remove(row, col)
  end
end

local function add_col(bm, col)
  for _,row in ipairs(bm) do
    table.insert(row, 0, col)
  end
end

local function trim_sides(bm)
  for i=20,1,-1 do
    if blank_col(bm, i) then
      prune_col(bm, i)
    else
      break
    end
  end
  for i=1,#bm[1] do
    if blank_col(bm, 1) then
      prune_col(bm, 1)
    else
      break
    end
  end
end

local function fingerprint_col(bm, n)
  local fp = 0
  for _,row in ipairs(bm) do
    fp = fp*2 + row[n]
  end
  return fp
end

local function smart_prune_cols(bm, n)
  if n == 0 then return end
  local fp,prev = -1,-1
  -- start by pruning cols that are identical to the ones they are adjacent to
  for col=#bm[1],1,-1 do
    fp = fingerprint_col(bm, col)
    if fp == prev then
      prune_col(bm, col)
      return smart_prune_cols(bm, n-1)
    end
  end
  -- then prune interior empty cols
  for col=#bm[1],1,-1 do
    if blank_col(bm, 1) then
      prune_col(bm, col)
      return smart_prune_cols(bm, n-1)
    end
  end
  -- finally delete every nth col
  for col=#bm[1],1.5,-(#bm[1]/(n-1)) do
    prune_col(bm, math.ceil(col))
  end
  prune_col(bm, 1)
  do return end
  -- finally shave the sides
  -- if #bm[1] % 2 == 1 then
  --   prune_col(bm, nil)
  -- else
  --   prune_col(bm, 1)
  -- end
  return smart_prune_cols(bm, n-1)
end

local function finalize_bitmap(bm)
  trim_sides(bm)
  local width = #bm[1]
  if width < 10 then
    -- too narrow? pad with blank cols, add more to right side if uneven
    for i=1,math.floor((10-width)/2) do add_col(bm, 1) end
    for i=1,math.ceil((10-width)/2) do add_col(bm, nil) end
  elseif width > 10 then
    smart_prune_cols(bm, width-10)
    -- for i=1,math.floor((width-10)/2) do prune_col(bm, 1) end
    -- for i=1,math.ceil((width-10)/2) do prune_col(bm, nil) end
  end
end

local function dump_bitmap(bm)
  for _,row in pairs(bm) do
    local n = 0
    for _,bit in ipairs(row) do
      n = n * 2 + bit
    end
    -- Add 6 bits of padding to take us to 16 bits even
    n = n * 2^6
    print(string.format('%04X', n))
  end
end

local function process_bitmap(iter)
  local bm = {}
  for line in iter do
    if line == 'ENDCHAR' then
      finalize_bitmap(bm)
      dump_bitmap(bm)
      print('ENDCHAR')
      return
    end
    line = tonumber(line, 16) / 2^4 -- shift out the rightmost 4 padding bits
    local raster = {}
    while line > 0 do
      table.insert(raster, 1, line % 2)
      line = math.floor(line/2)
    end
    while #raster < 20 do
      table.insert(raster, 1, 0)
    end
    table.insert(bm, raster)
  end
end

local iter = io.lines()
for line in iter do
  if line:match('^BBX ') then
    print('BBX 10 20 0 0')
  elseif line:match('^DWIDTH ') then
    print('DWIDTH 10 0')
  elseif line:match('^FONTBOUNDINGBOX ') then
    print('FONTBOUNDINGBOX 10 20 0 0')
  elseif line == 'BITMAP' then
    print('BITMAP')
    process_bitmap(iter)
  else
    print(line)
  end
end
