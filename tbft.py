from argparse import Namespace
from argparse import ArgumentParser
import argparse
from operator import contains
import os
import shutil
import subprocess


def main():
    args = create_parser()
    try:
        args.func(args)
    except AttributeError:
        print('Input error!')
        print('Please use tbft config --help or tbft install --help for instructions on tbft installer usage.')

def create_parser() -> Namespace:
    parser = ArgumentParser(
        description='Executable to install and configure TBFT.')
    subparser = parser.add_subparsers()
    create_install_parser(subparser)
    create_config_parser(subparser)

    try:
        return  parser.parse_args()
    except argparse.ArgumentError:
        raise BaseException("Error while parsing the arguments!")

def create_install_parser(subparser):
    subcommands = subparser.add_parser('install')
    subcommands.add_argument("--path-gcc", type=str,
                        required=True, help="Absolute path to GCC Compiler.", dest="gcc")
    subcommands.add_argument("--prefix-gcc", type=str,
                        required=True, help="Absolute path to the directory to install the GCC binaries.", dest="gcc_bin")
    subcommands.add_argument("--platform", type=str, required=True,
                         help="TBFT plataform to install.", choices=['amd', 'intel'])
    subcommands.add_argument("--jobs", type=int, default=1,
                         help="Number of threads to compile TBFT in the make command.", dest="jobs")
    subcommands.add_argument("--shell", type=str, default="bash",
                         help="Defines the actual Shell Interpreter")
    subcommands.set_defaults(func=install_tbft)

def create_config_parser(subparser):
    subcommands = subparser.add_parser('config')
    subcommands.add_argument("--strategy", type=str, required=True,
                        help="Sets the strategy used by TBFT Turbo Engine.", choices=['performance-agressive', 'power-agressive', 'balanced'])
    subcommands.set_defaults(func=config_tbft)

def install_tbft(args):
    print("Compiling TBFT...")
    compile_tbft(args)
    print("Compilation successful!")
    print("Running TBFT test...")
    run_tbft_tests(args)
    print("Tests successfully executed!")
    print("Setting TBFT environment variables...")
    set_environment_variables(args)
    print("Environment variables settings successful...")

def compile_tbft(args):
    shutil.copytree(f'./{args.platform}', f'{args.gcc}/libgomp')
    shutil.copy(f'./{args.platform}/boost.sh', f'{args.gcc_bin}/lib64')
    make_process = subprocess.Popen(f'make -C {args.gcc}  -j {args.jobs} && make install -C {args.gcc}' , stderr=subprocess.STDOUT)
    if make_process.wait() != 0:
        print("Ops, error while compiling GCC!")
        exit(1)

def run_tbft_tests(args):
    print("Not implemented yet")

def set_environment_variables(args):
    shell_rc = f"{args.shell}rc"
    with open(f"~/.{shell_rc}", "r") as file:
        lines = file.readlines()
    with open(f"~/.{shell_rc}", "w") as file:
        for line in lines:
            if "LD_LIBRARY_PATH" not in line.strip("\n") and "OMP_POSEIDON_BOOST_PATH" not in line.strip("\n"):
                file.write(line)
        file.write(f'export LD_LIBRARY_PATH={args.prefix_gcc_bin}/lib64')
        file.write(f'export OMP_POSEIDON_BOOST_PATH={args.prefix_gcc_bin}/lib64')
        
    os.system(f"exec {args.shell}")


def config_tbft(args):
    print("Not implemented yet")


if __name__ == "__main__":
    main()