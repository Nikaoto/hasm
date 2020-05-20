-- Tests for hasm

local lest = require ("lest")
local print_table = require ("print_table")
local expect = lest.expect
local group = lest.group
local fmt = string.format

local HASM_PATH = arg[1] or "../hasm"
local TEST_DIR = arg[2] or "sandbox"

local function make_hasm_command(in_file, out_file)
   return fmt("%s %s -o %s", HASM_PATH, in_file, out_file)
end

local function list_files_in_dir(dir_path)
   local f = io.popen(fmt("find %s -type f", dir_path))
   local list = f:read("*a")
   f:close()
   return list
end

local function read_file_fully(filename)
   local fh = io.open(filename, "r")
   local content = fh:read("*a")
   fh:close()
   return content
end

-- rm all generated .hack files
local function clean()
   local files = list_files_in_dir(TEST_DIR)
   for filename in string.gmatch(files, "(.-)\n") do
      if string.find(filename, "[^cmp].hack") then
         os.execute(fmt("rm %s", filename))
      end
   end
end

local function test_files(filenames)
   for i, filename in ipairs(filenames) do
      local asm = fmt("%s/%s.asm", TEST_DIR, filename)
      local hack = fmt("%s/%s.hack", TEST_DIR, filename)
      local cmp = fmt("%s/%s.cmp.hack", TEST_DIR, filename)
      local command = make_hasm_command(asm, hack)

      group(fmt("%s", command))

      local error_code = os.execute(command)
      expect(error_code).to_be(0)

      local hack_content, cmp_content, file_exists

      file_exists, cmp_content = pcall(read_file_fully, cmp)
      expect(file_exists).to_be(true)
      if not file_exists then
         print(fmt("ERROR: file %s doesn't exist", cmp))
      end

      file_exists, hack_content = pcall(read_file_fully, hack)
      expect(file_exists).to_be(true)
      if not file_exists then
         print(fmt("ERROR: file %s doesn't exist", hack))
      end

      expect(cmp_content).not_.to_be("")
      expect(hack_content).not_.to_be("")
      expect(hack_content).to_be(cmp_content)
   end
end

clean()

test_files({
   "MaxL",
   "RectL",
   "PongL",
})

lest.print_stats()

--[[
test_files({
   "Add",
   "Max",
   "Rect",
   "Mult",
   "Fill",
   "Pong",
})
--]]
