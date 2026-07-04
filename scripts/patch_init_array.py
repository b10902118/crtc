#!/usr/bin/env python3
import sys
import struct

def patch_init_array(filename):
    print(f"Checking .init_array of {filename} for NULL terminators...")
    try:
        with open(filename, "r+b") as f:
            elf_header = f.read(64)
            if len(elf_header) < 52:
                print("Error: File too small to be a valid ELF.")
                return

            if elf_header[:4] != b"\x7fELF":
                print("Error: Not an ELF file.")
                return

            elf_class = elf_header[4]
            is_64 = (elf_class == 2)
            word_size = 8 if is_64 else 4
            unpack_fmt = "<Q" if is_64 else "<I"
            pack_fmt = "<q" if is_64 else "<i"

            if not is_64:
                e_phoff = struct.unpack("<I", elf_header[28:32])[0]
                e_phentsize = struct.unpack("<H", elf_header[42:44])[0]
                e_phnum = struct.unpack("<H", elf_header[44:46])[0]
            else:
                e_phoff = struct.unpack("<Q", elf_header[32:40])[0]
                e_phentsize = struct.unpack("<H", elf_header[54:56])[0]
                e_phnum = struct.unpack("<H", elf_header[56:58])[0]

            # Find PT_LOAD and PT_DYNAMIC segments
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
                for p_type, p_offset, p_vaddr, _, p_memsz in segments:
                    if p_type == 1:  # PT_LOAD
                        if p_vaddr <= va < p_vaddr + p_memsz:
                            return va - p_vaddr + p_offset
                return None

            # Find PT_DYNAMIC
            dynamic_segment = None
            for p_type, p_offset, p_vaddr, p_filesz, _ in segments:
                if p_type == 2:  # PT_DYNAMIC
                    dynamic_segment = (p_offset, p_filesz)
                    break

            if not dynamic_segment:
                print("Error: PT_DYNAMIC segment not found.")
                return

            p_offset, p_filesz = dynamic_segment
            dyn_entry_size = 16 if is_64 else 8
            num_entries = p_filesz // dyn_entry_size

            DT_INIT_ARRAY = 25
            DT_INIT_ARRAYSZ = 27

            init_array_va = None
            init_arraysz_val = None
            init_arraysz_offset = None

            f.seek(p_offset)
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

                if d_tag == DT_INIT_ARRAY:
                    init_array_va = d_val
                elif d_tag == DT_INIT_ARRAYSZ:
                    init_arraysz_val = d_val
                    init_arraysz_offset = entry_offset + (8 if is_64 else 4)

            if init_array_va is None or init_arraysz_val is None:
                print("Warning: No .init_array or size found in PT_DYNAMIC.")
                return

            init_array_offset = va_to_offset(init_array_va)
            if not init_array_offset:
                print("Error: Could not convert .init_array VA to file offset.")
                return

            print(f"Found .init_array at file offset {init_array_offset} with size {init_arraysz_val} bytes.")

            # Check the last entry
            last_entry_offset = init_array_offset + init_arraysz_val - word_size
            f.seek(last_entry_offset)
            last_entry_bytes = f.read(word_size)
            last_entry_val = struct.unpack(unpack_fmt, last_entry_bytes)[0]

            print(f"Last entry in .init_array is: {hex(last_entry_val)}")

            if last_entry_val == 0:
                print("Last entry is NULL (0x0). Decrementing DT_INIT_ARRAYSZ to skip it...")
                new_size = init_arraysz_val - word_size
                f.seek(init_arraysz_offset)
                f.write(struct.pack(unpack_fmt, new_size))
                print(f"Successfully decremented DT_INIT_ARRAYSZ from {init_arraysz_val} to {new_size} bytes in {filename}.")
            else:
                print("Last entry is not NULL. No patching needed.")

    except Exception as e:
        print(f"Failed to patch: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 patch_init_array.py <so_file>")
        sys.exit(1)
    patch_init_array(sys.argv[1])
