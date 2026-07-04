#!/usr/bin/env python3

import argparse
import lzma
import os
import sys


def decompress_csv(input_path, output_dir):
    try:
        with open(input_path, "rb") as f:
            data = f.read()

        lzma_data = data[:9] + (b"\x00" * 4) + data[9:]

        decompressed_bytes = lzma.decompress(lzma_data)
        decompressed_text = decompressed_bytes.decode("utf-8")

        # Determine output path
        filename = os.path.basename(input_path)
        output_path = os.path.join(output_dir, filename)

        with open(output_path, "w", encoding="utf-8") as f:
            f.write(decompressed_text)
        print(f"Decompressed: {input_path} -> {output_path}")
    except Exception as e:
        print(f"Error decompressing {input_path}: {e}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output-dir", required=True, help="Directory to write decompressed CSV files."
    )
    parser.add_argument("csvs", nargs="+", help="One or more CSV files to decompress.")

    args = parser.parse_args()

    # Ensure output directory exists
    os.makedirs(args.output_dir, exist_ok=True)

    for csv_path in args.csvs:
        decompress_csv(csv_path, args.output_dir)


if __name__ == "__main__":
    main()
