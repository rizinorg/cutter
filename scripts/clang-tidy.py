#!/usr/bin/env python3
import argparse
import subprocess
import sys

DEFAULT_REGEX = r".*src/(?!(themes|bindings|fonts|img|translations|Cutter_autogen)).*\.(cpp|h)$"

def run_tidy(args):
    cmd = [args.run_clang_tidy, "-p", args.build_path]
    
    if args.jobs:
        cmd.append(f"-j {args.jobs}")
        
    if args.fix:
        cmd.append("-fix")
        
    if args.quiet:
        cmd.append("-quiet")

    cmd.append(f"'{args.regex}'")

    print(f"Executing: {' '.join(cmd)}")

    result = subprocess.run(" ".join(cmd), shell=True)
    
    sys.exit(result.returncode)

def main():
    parser = argparse.ArgumentParser(description="clang-tidy regex wrapper")
    
    parser.add_argument("-T", "--run-clang-tidy", 
                        default="run-clang-tidy",
                        help="Path of run-clang-tidy binary")
    
    parser.add_argument("-p", "--build-path", default="build", 
                        help="Path to the build directory")
    
    parser.add_argument("-j", "--jobs", type=int, default=0,
                        help="Number of parallel execution jobs")
    
    parser.add_argument("-r", "--regex", default=DEFAULT_REGEX,
                        help="Regex pattern for filtering files")
    
    parser.add_argument("-i", "--fix", action="store_true", help="Apply fixes automatically")
    parser.add_argument("-q", "--quiet", action="store_true", default=False, help="Suppress configuration logs")

    args = parser.parse_args()
    run_tidy(args)

if __name__ == "__main__":
    main()
