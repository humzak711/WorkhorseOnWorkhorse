#!/usr/bin/env python3

from elftools.elf.elffile import ELFFile

def main():
    with open("guest/build/workhorse.elf", "rb") as f:
        e = ELFFile(f)

        entry = e['e_entry']

        segments = [seg for seg in e.iter_segments() if seg['p_type'] == 'PT_LOAD']

        if not segments:
            raise ValueError("no pt load segments found")
        
        min_vaddr = min(seg['p_vaddr'] for seg in segments)
        max_vaddr = max((seg['p_vaddr'] + seg['p_memsz']) for seg in segments)


        if min_vaddr >= 0xffffff8000000000:
            max_vaddr -= 0xffffff8000000000

        if max_vaddr >= 0xffffff8000000000:
            max_vaddr -= 0xffffff8000000000

        size = (max_vaddr + 4095) &~0xfff

        print(f"elf entry: {hex(entry)}")
        print(f"base: {hex(min_vaddr)}")
        print(f"size {size}")

        buf = bytearray(size)

        for seg in segments:
             
            v_start = seg['p_vaddr']
            v_size = seg['p_filesz']

            if v_start >= 0xffffff8000000000:
                v_start -= 0xffffff8000000000

            data = seg.data()

            buf[v_start : v_start + v_size] = data[:v_size]

        hex_buf = []
        for i in range(0, len(buf), 16):
            chunk = buf[i:i+16]
            hex_string = ", ".join(f"0x{b:02x}" for b in chunk)
            hex_buf.append(f"    {hex_string}")

        c_bin = ',\n'.join(hex_buf)

        source = f"""
#ifndef _WORKHORSE_BIN_H_
#define _WORKHORSE_BIN_H_

#include <compiler.h>
#include <stdint.h>

unsigned char ATTR_ALIGNED(4096) image[] = {{{c_bin}}};

const 
uintptr_t vaBase = {hex(min_vaddr)};

const
uintptr_t entry = {hex(entry)};

#endif
"""
        with open("WorkhorseRT/plugins/workhorse.bin", 'w') as fz:
            fz.write(source)

        print("successfully generated workhorse.bin")

if __name__ == "__main__":
        main()