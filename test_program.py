import os
import subprocess
import re

def run_tests():
    # Define paths
    e4_dir = "./E4"
    t_e4_path = os.path.join(e4_dir, "golden.txt")
    program_path = "./etapa5"
    
    # Define the timeout in seconds
    TIMEOUT_SECONDS = 2

    # Regex to remove ANSI escape codes
    ansi_escape_pattern = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')

    # ---
    ## Load Expected Outputs
    # ---

    expected_outputs = {}
    try:
        with open(t_e4_path, 'r') as f:
            for line in f:
                line = line.strip()
                if line:
                    parts = line.split('(')
                    if len(parts) == 2:
                        first_word = parts[0].strip()
                        file_name = parts[1].replace(')', '').strip()
                        expected_outputs[file_name] = first_word
    except FileNotFoundError:
        print(f"Error: Expected output file '{t_e4_path}' not found.")
        return

    # ---
    ## Run Tests
    # ---

    print("Starting tests with a timeout of {TIMEOUT_SECONDS} seconds per test...".format(TIMEOUT_SECONDS=TIMEOUT_SECONDS))
    print("-" * 30)

    test_files = [f for f in os.listdir(e4_dir) if os.path.isfile(os.path.join(e4_dir, f)) and f != "t_e4.org" and f != "golden.txt"]
    
    timeout_tests = []
    failed_tests = [] # New list to store failed tests

    if not test_files:
        print(f"No test files found in '{e4_dir}' (excluding t_e4.org).")
        return

    for test_file in sorted(test_files):
        print(f"Testing: {test_file}")
        input_file_path = os.path.join(e4_dir, test_file)

        try:
            # Execute the program with a timeout and capture its output
            result = subprocess.run(
                [program_path],
                stdin=open(input_file_path, 'r'),
                capture_output=True,
                text=True,
                check=False,
                timeout=TIMEOUT_SECONDS # Add the timeout here
            )

            raw_program_output = result.stdout.strip()
            
            # Remove ANSI color codes from the output
            cleaned_program_output = ansi_escape_pattern.sub('', raw_program_output).strip()
            
            first_printed_word = cleaned_program_output.split(' ')[0] if cleaned_program_output else ""

            expected_first_word = expected_outputs.get(test_file)

            if expected_first_word is None:
                print(f"  Warning: No expected output defined for '{test_file}'. Skipping comparison.")
            elif first_printed_word == expected_first_word:
                print(f"  Result: PASSED (Expected: '{expected_first_word}', Got: '{first_printed_word}')")
            else: # Test failed
                print(f"  Result: FAILED (Expected: '{expected_first_word}', Got: '{first_printed_word}')")
                failed_tests.append(test_file) # Add to failed list
            
            if result.stderr:
                print(f"  Program Errors (stderr):\n{result.stderr}")

        except FileNotFoundError:
            print(f"  Error: Program '{program_path}' not found. Make sure it's executable and in the correct path.")
            break
        except subprocess.TimeoutExpired:
            print(f"  Result: TIMEOUT (Program exceeded {TIMEOUT_SECONDS} seconds)")
            timeout_tests.append(test_file)
        except Exception as e:
            print(f"  An unexpected error occurred while testing {test_file}: {e}")
        print("-" * 30)

    print("Tests finished.")
    print("\n" + "=" * 30)
    if timeout_tests:
        print("The following tests timed out:")
        for test in timeout_tests:
            print(f"- {test}")
    else:
        print("No tests timed out.")
    
    print("-" * 30) # Separator for clarity
    
    if failed_tests:
        print("The following tests FAILED (output mismatch):")
        for test in failed_tests:
            print(f"- {test}")
    else:
        print("No tests failed due to output mismatch.")
    print("=" * 30)

if __name__ == "__main__":
    run_tests()