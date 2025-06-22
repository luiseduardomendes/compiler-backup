#!/usr/bin/env python3
import os
import sys
import argparse
import subprocess
from pathlib import Path

# ILOC Simulator Script (assumed to be in the same directory)
ILOC_SIMULATOR = "ilocsim.py"  # (or whatever your simulator script is named)

def file_expects_compilation_error(ll_file: Path) -> bool:
    """Check if the file starts with '// Compilation Error Expected'."""
    with open(ll_file, 'r') as f:
        first_line = f.readline().strip()
    return  first_line.startswith("//")
    

def compile_ll_to_iloc(ll_file: Path, iloc_file: Path) -> bool:
    """Compiles .ll to .iloc using ./etapa4. Returns True if successful."""
    expects_error = file_expects_compilation_error(ll_file)
    
    try:
        with open(ll_file, 'r') as infile, open(iloc_file, 'w') as outfile:
            result = subprocess.run(
                ["./etapa5"],
                stdin=infile,
                stdout=outfile,
                stderr=subprocess.PIPE,
                text=True
            )
        
        if expects_error:
            if result.returncode == 0:
                #print(f"❌ FAIL (expected error, but compilation succeeded): {ll_file}")
                return False
            else:
                #print(f"✅ PASS (expected error and got one): {ll_file}")
                return False  # "Pass" because the error was expected
        else:
            if result.returncode != 0:
                #print(f"❌ FAIL (unexpected compilation error): {ll_file}")
                #print(result.stderr)
                return False
            return True
    except Exception as e:
        print(f"❌ ERROR (compilation crashed): {ll_file}: {e}")
        return False

def run_iloc_simulator(iloc_file: Path, sim_args: list) -> bool:
    """Runs the ILOC simulator on the generated file. Returns True if successful."""
    try:
        result = subprocess.run(
            ["python3", ILOC_SIMULATOR] + sim_args + [str(iloc_file)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        if result.returncode != 0:
            print(f"❌ Simulation failed for {iloc_file}:")
            print(result.stderr)
            return False
        print(f"✅ Successfully executed {iloc_file}")
        print(result.stdout)  # (Optional: Only print if needed)
        return True
    except Exception as e:
        print(f"❌ Error running simulator on {iloc_file}: {e}")
        return False

def test_programs_in_folder(folder: Path, sim_args: list, keep_iloc: bool = False):
    """Tests all .ll files in the given folder."""
    print(f"🔍 Testing .ll files in {folder}...")
    passed = 0
    failed = 0

    execution_queue = sorted(folder.glob("*.ll"))

    for ll_file in execution_queue:
        print(f"\n📄 Processing {ll_file.name}...")
        iloc_file = ll_file.with_suffix(".iloc")

        # Step 1: Compile .ll → .iloc
        if not compile_ll_to_iloc(ll_file, iloc_file):
            failed += 1
            print(f"❌ COULDNT COMPILE : {ll_file}")
            continue
        
        # Step 2: Run ILOC simulator
        if run_iloc_simulator(iloc_file, sim_args):
            passed += 1
        else:
            failed += 1

        # Step 3: Clean up .iloc file (optional)
        if not keep_iloc and iloc_file.exists():
            iloc_file.unlink()

    print(f"\n📊 Results: {passed} passed, {failed} failed.")

def main():
    parser = argparse.ArgumentParser(description="Test LL programs by compiling to ILOC and simulating.")
    parser.add_argument("folder", help="Folder containing .ll files to test")
    parser.add_argument("--keep-iloc", action="store_true", help="Keep generated .iloc files")
    parser.add_argument("--sim-args", nargs="*", default=[], help="Extra args for ILOC simulator (e.g., -t -i)")
    args = parser.parse_args()

    folder = Path(args.folder)
    if not folder.exists():
        print(f"❌ Error: Folder '{folder}' does not exist.")
        sys.exit(1)

    test_programs_in_folder(folder, args.sim_args, args.keep_iloc)

if __name__ == "__main__":
    main()