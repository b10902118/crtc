#!/usr/bin/env python3
import sys
import struct

def patch_elf(filename):
    print(f"Patching {filename} to strip symbol version requirements...")
    try:
        with open(filename, "r+b") as f:
            elf_header = f.read(64)
            if len(elf_header) < 52:
                print("Error: File too small to be a valid ELF.")
                return

            # Check ELF magic
            if elf_header[:4] != b"\x7fELF":
                print("Error: Not an ELF file.")
                return

            # Check class: 1 = 32-bit, 2 = 64-bit
            elf_class = elf_header[4]
            if elf_class not in (1, 2):
                print("Error: Invalid ELF class.")
                return

            is_64 = (elf_class == 2)
            print(f"ELF Class: {'64-bit' if is_64 else '32-bit'}")

            # Parse program header offset, entry size, and count
            if not is_64:
                e_phoff = struct.unpack("<I", elf_header[28:32])[0]
                e_phentsize = struct.unpack("<H", elf_header[42:44])[0]
                e_phnum = struct.unpack("<H", elf_header[44:46])[0]
                e_shoff = struct.unpack("<I", elf_header[32:36])[0]
                e_shentsize = struct.unpack("<H", elf_header[46:48])[0]
                e_shnum = struct.unpack("<H", elf_header[48:50])[0]
            else:
                e_phoff = struct.unpack("<Q", elf_header[32:40])[0]
                e_phentsize = struct.unpack("<H", elf_header[54:56])[0]
                e_phnum = struct.unpack("<H", elf_header[56:58])[0]
                e_shoff = struct.unpack("<Q", elf_header[40:48])[0]
                e_shentsize = struct.unpack("<H", elf_header[58:60])[0]
                e_shnum = struct.unpack("<H", elf_header[60:62])[0]

            print(f"Program headers: offset={e_phoff}, size={e_phentsize}, count={e_phnum}")
            print(f"Section headers: offset={e_shoff}, size={e_shentsize}, count={e_shnum}")

            # Collect PT_LOAD segments for VA-to-offset conversion
            segments = []
            f.seek(e_phoff)
            for i in range(e_phnum):
                ph_data = f.read(e_phentsize)
                if len(ph_data) < e_phentsize:
                    break
                
                if not is_64:
                    p_type = struct.unpack("<I", ph_data[0:4])[0]
                    p_offset = struct.unpack("<I", ph_data[4:8])[0]
                    p_vaddr = struct.unpack("<I", ph_data[8:12])[0]
                    p_filesz = struct.unpack("<I", ph_data[16:20])[0]
                    p_memsz = struct.unpack("<I", ph_data[20:24])[0]
                else:
                    p_type = struct.unpack("<I", ph_data[0:4])[0]
                    p_offset = struct.unpack("<Q", ph_data[8:16])[0]
                    p_vaddr = struct.unpack("<Q", ph_data[16:24])[0]
                    p_filesz = struct.unpack("<Q", ph_data[32:40])[0]
                    p_memsz = struct.unpack("<Q", ph_data[40:48])[0]

                segments.append((p_type, p_offset, p_vaddr, p_filesz, p_memsz))

            def va_to_offset(va):
                for p_type, p_offset, p_vaddr, p_filesz, p_memsz in segments:
                    if p_type == 1:  # PT_LOAD
                        if p_vaddr <= va < p_vaddr + p_memsz:
                            return va - p_vaddr + p_offset
                return None

            # 1. Zero out DT_VERNEED and DT_VERNEEDNUM in PT_DYNAMIC
            dynamic_segment = None
            for p_type, p_offset, p_vaddr, p_filesz, p_memsz in segments:
                if p_type == 2:  # PT_DYNAMIC
                    dynamic_segment = (p_offset, p_filesz)
                    break

            dt_versym_va = None
            dt_hash_va = None
            dt_gnu_hash_va = None

            if dynamic_segment:
                p_offset, p_filesz = dynamic_segment
                print(f"PT_DYNAMIC segment found at offset {p_offset} with size {p_filesz} bytes.")

                f.seek(p_offset)
                dyn_entry_size = 16 if is_64 else 8
                num_entries = p_filesz // dyn_entry_size

                DT_VERNEED = 0x6ffffffe
                DT_VERNEEDNUM = 0x6fffffff
                DT_VERSYM = 0x6ffffff0
                DT_HASH = 4
                DT_GNU_HASH = 0x6ffffef5
                DT_NULL = 0

                patched_count = 0

                for i in range(num_entries):
                    entry_offset = p_offset + i * dyn_entry_size
                    f.seek(entry_offset)
                    entry_data = f.read(dyn_entry_size)
                    if len(entry_data) < dyn_entry_size:
                        break

                    if not is_64:
                        d_tag = struct.unpack("<i", entry_data[0:4])[0]
                        d_val = struct.unpack("<I", entry_data[4:8])[0]
                    else:
                        d_tag = struct.unpack("<q", entry_data[0:8])[0]
                        d_val = struct.unpack("<Q", entry_data[8:16])[0]

                    # Convert to unsigned
                    d_tag_u = d_tag & 0xffffffff if not is_64 else d_tag & 0xffffffffffffffff

                    if d_tag_u in (DT_VERNEED, DT_VERNEEDNUM):
                        f.seek(entry_offset)
                        if not is_64:
                            f.write(struct.pack("<i", DT_NULL))
                        else:
                            f.write(struct.pack("<q", DT_NULL))
                        print(f"  Zeroed out tag {hex(d_tag_u)} at file offset {entry_offset}")
                        patched_count += 1
                    elif d_tag_u == DT_VERSYM:
                        dt_versym_va = d_val
                    elif d_tag_u == DT_HASH:
                        dt_hash_va = d_val
                    elif d_tag_u == DT_GNU_HASH:
                        dt_gnu_hash_va = d_val

                if patched_count > 0:
                    print(f"Successfully patched {patched_count} dynamic tags.")
            else:
                print("Warning: PT_DYNAMIC segment not found.")

            # 2. Clear symbol versions (set all versym entries to 1, except first which is 0)
            versym_offset = None
            versym_size = None

            # First, try to locate versym via Section Headers
            if e_shoff != 0 and e_shnum != 0:
                f.seek(e_shoff)
                for i in range(e_shnum):
                    sh_data = f.read(e_shentsize)
                    if len(sh_data) < e_shentsize:
                        break
                    if not is_64:
                        sh_type = struct.unpack("<I", sh_data[4:8])[0]
                        sh_offset = struct.unpack("<I", sh_data[16:20])[0]
                        sh_size = struct.unpack("<I", sh_data[20:24])[0]
                    else:
                        sh_type = struct.unpack("<I", sh_data[4:8])[0]
                        sh_offset = struct.unpack("<Q", sh_data[24:32])[0]
                        sh_size = struct.unpack("<Q", sh_data[32:40])[0]

                    if sh_type == 0x6fffffff:  # SHT_GNU_versym
                        versym_offset = sh_offset
                        versym_size = sh_size
                        print(f"Found SHT_GNU_versym section via section headers: offset={sh_offset}, size={sh_size}")
                        break

            # Fallback to DT_VERSYM and DT_HASH / DT_GNU_HASH if section headers aren't available
            if not versym_offset and dt_versym_va:
                print(f"Section headers not found/used. Using DT_VERSYM VA: {hex(dt_versym_va)}")
                versym_offset = va_to_offset(dt_versym_va)
                if versym_offset:
                    # Determine symbol count
                    num_symbols = None
                    if dt_hash_va:
                        hash_offset = va_to_offset(dt_hash_va)
                        if hash_offset:
                            f.seek(hash_offset + 4)  # Skip nbucket (4 bytes)
                            nchain = struct.unpack("<I", f.read(4))[0]
                            num_symbols = nchain
                            print(f"Found DT_HASH at VA {hex(dt_hash_va)} (offset {hash_offset}), num_symbols (nchain) = {num_symbols}")
                    
                    if not num_symbols and dt_gnu_hash_va:
                        gnu_hash_offset = va_to_offset(dt_gnu_hash_va)
                        if gnu_hash_offset:
                            # GNU hash has: nbuckets (4), symoffset (4), bloom_size (4), bloom_shift (4)
                            f.seek(gnu_hash_offset)
                            nbuckets, symoffset, bloom_size, bloom_shift = struct.unpack("<IIII", f.read(16))
                            try:
                                bloom_bytes = bloom_size * (8 if is_64 else 4)
                                buckets_offset = gnu_hash_offset + 16 + bloom_bytes
                                f.seek(buckets_offset)
                                buckets = struct.unpack(f"<{nbuckets}I", f.read(4 * nbuckets))
                                max_sym_idx = max(buckets) if buckets else 0
                                if max_sym_idx > 0:
                                    chains_offset = buckets_offset + 4 * nbuckets + (max_sym_idx - symoffset) * 4
                                    f.seek(chains_offset)
                                    curr_idx = max_sym_idx
                                    while True:
                                        chain_val = struct.unpack("<I", f.read(4))[0]
                                        curr_idx += 1
                                        if (chain_val & 1) == 1:
                                            break
                                    num_symbols = curr_idx
                                    print(f"Found DT_GNU_HASH at VA {hex(dt_gnu_hash_va)}, estimated num_symbols = {num_symbols}")
                            except Exception as e:
                                print(f"Error parsing DT_GNU_HASH: {e}")

                    if num_symbols:
                        versym_size = num_symbols * 2
                    else:
                        print("Warning: Could not determine symbol count. Defaulting to 4096 symbols.")
                        versym_size = 4096 * 2

            if versym_offset and versym_size:
                print(f"Clearing symbol versions in .gnu.version (offset={versym_offset}, size={versym_size} bytes)...")
                # Each entry is 16-bit.
                # Entry 0 is always 0 (local/undefined).
                # All other entries are set to 1 (global/unversioned).
                f.seek(versym_offset)
                num_entries = versym_size // 2
                
                # Write 0 for index 0
                f.write(struct.pack("<H", 0))
                # Write 1 for indices 1 to num_entries-1
                if num_entries > 1:
                    chunk_size = 1024
                    ones_chunk = struct.pack("<H", 1) * chunk_size
                    remaining = num_entries - 1
                    while remaining > 0:
                        to_write = min(remaining, chunk_size)
                        f.write(ones_chunk[:to_write * 2])
                        remaining -= to_write
                print(f"Successfully cleared {num_entries} symbol versions in {filename}.")
            else:
                print(f"Warning: Could not locate .gnu.version section in {filename}.")

            print(f"Successfully patched {filename}.\n")

    except Exception as e:
        print(f"Failed to patch {filename}: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 strip_verneed.py <so_file1> [so_file2 ...]")
        sys.exit(1)
    for filename in sys.argv[1:]:
        patch_elf(filename)
