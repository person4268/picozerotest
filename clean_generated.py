import os
import sys

def clean_generated(img_dir, generated_dir):
    # Get all image filenames without extensions in the img/ directory
    image_files = {os.path.splitext(f)[0] for f in os.listdir(img_dir)}

    # Get all header files in the src/generated/ directory
    generated_files = {os.path.splitext(f)[0] for f in os.listdir(generated_dir) if f.endswith(".h")}

    # Find generated headers that no longer have corresponding image files
    files_to_remove = generated_files - image_files

    # Remove orphaned header files
    for file in files_to_remove:
        header_path = os.path.join(generated_dir, f"{file}.h")
        print(f"Removing outdated file: {header_path}")
        os.remove(header_path)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: clean_generated.py <img_dir> <generated_dir>")
        sys.exit(1)

    img_dir = sys.argv[1]
    generated_dir = sys.argv[2]
    
    clean_generated(img_dir, generated_dir)
