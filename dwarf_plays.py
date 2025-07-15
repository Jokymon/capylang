import io
import re
import zlib
from elftools.dwarf.dwarfinfo import DWARFInfo, DwarfConfig

DWARF_SECTIONS = [
    "debug_loc",
    "debug_abbrev",
    "debug_info",
    "debug_str",
    "debug_line",
    "debug_ranges",
]


def extract_bin_data(wat_line):
    def replacer(match):
        hex_value = match.group(1)
        return chr(int(hex_value, 16))

    start = wat_line.index('"') + 1
    end = wat_line.index('"', start)
    binary_data = re.sub(r"\\([0-9A-Fa-f]{2})", replacer, wat_line[start:end])

    return (
        binary_data if isinstance(binary_data, bytes)
        else binary_data.encode("utf-8")
    )


class Section:
    def __init__(self, name, content):
        self.name = name
        self.content = content
        try:
            decomp = zlib.decompressobj()
            result = decomp.decompress(content, len(content))
            self.content = result
            print("DECOMPRESSED!!")
        except:
            pass
        self.stream = io.BytesIO(self.content)

    def compressed(self):
        return False

    def data_size(self):
        return len(self.content)

    def data_alignment(self):
        return 4  # TODO: really?

    def data(self):
        # TODO: let's hope it's not compressed
        return self.content

    def is_null(self):
        return False

    @property
    def size(self):
        return self.data_size()


def main():
    dwarf_sections = {}

    with open("test.wat", "r") as wat_in:
        for line in wat_in.readlines():
            line = line.strip()
            if not line.startswith('(@custom ".'):
                continue
            line = line[11:]
            for section_name in DWARF_SECTIONS:
                if line.startswith(section_name):
                    line = line[len(section_name) + 1 :]
                    dwarf_sections[section_name] = extract_bin_data(line)
                    break
    #print(len(dwarf_sections["debug_info"]))

    debug_info = dwarf_sections["debug_info"]
    try:
        decomp = zlib.decompressobj()
        print(debug_info[13:20])
        raw = decomp.decompress(debug_info[13:], len(debug_info)-13)
        print("DECOMPRESSED!!")
    except Exception as e:
        print(f"NOPE, couldn't decompress: {e}")

    
    return

    dwarfinfo = DWARFInfo(
        config=DwarfConfig(
            little_endian=False,
            default_address_size=4,
            machine_arch="WASM"
        ),
        debug_info_sec=Section(".debug_info", dwarf_sections["debug_info"]),
        debug_aranges_sec=None,
        debug_abbrev_sec=Section(".debug_abbrev", dwarf_sections["debug_abbrev"]),
        debug_frame_sec=None,
        eh_frame_sec=None,
        debug_str_sec=Section(".debug_str", dwarf_sections["debug_str"]),
        debug_loc_sec=Section(".debug_loc", dwarf_sections["debug_loc"]),
        debug_ranges_sec=Section(".debug_ranges", dwarf_sections["debug_ranges"]),
        debug_line_sec=Section(".debug_line", dwarf_sections["debug_line"]),
        debug_pubtypes_sec=None,
        debug_pubnames_sec=None,
        debug_addr_sec=None,
        debug_str_offsets_sec=None,
        debug_line_str_sec=None,
        debug_loclists_sec=None,
        debug_rnglists_sec=None,
        debug_sup_sec=None,
        gnu_debugaltlink_sec=None,
        debug_types_sec=None
        )
    #dwarfinfo._parse_CU_at_offset(0)

    for cu in dwarfinfo.iter_CUs():
        print(f"Found compile unit at {cu.cu_offset}, length {cu['unit_length']}")
        top_die = cu.get_top_DIE()
        print(f"Top DIE is a {top_die.tag}")
        for child in top_die.iter_children():
            print(f"  Sub DIE: {child.tag}")
    # locs = dwarfinfo.location_lists()
    # for loc in locs.iter_location_lists():
    #     print(loc)
    #     # print(loc)


if __name__ == "__main__":
    main()
