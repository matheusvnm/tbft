from argparse import Namespace
from argparse import ArgumentParser
import argparse
from operator import contains
import os
import shutil
import subprocess







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
    subcommands.add_argument("--prefix-gcc", type=str,
                        required=True, help="GCC Compiler absolute path.", dest="prefix_gcc")
    subcommands.add_argument("--prefix-bin", type=str,
                        required=True, help="GCC Compiler Binaries absolute path.", dest="prefix_gcc_bin")
    subcommands.add_argument("--version", type=str, required=True,
                         help="Sets TBFT version to install.", choices=['amd', 'intel'])
    subcommands.add_argument("--num-threads", type=int, default=1,
                         help="Number of threads to execute the make command.", dest="num_threads")
    subcommands.add_argument("--shell", type=str, default="bash",
                         help="Defines the actual Shell Interpreter")
    subcommands.set_defaults(func=install_tbft)

def create_config_parser(subparser):
    subcommands = subparser.add_parser('config')
    subcommands.add_argument("--strategy", type=str, required=True,
                        help="Sets the strategy used by TBFT Turbo Engine.", choices=['performance-agressive', 'power-agressive', 'balanced'])
    subcommands.set_defaults(func=config_tbft)

def install_tbft(args):
    compile_tbft(args)
    run_tbft_tests(args)
    set_environment_variables(args)

def compile_tbft(args):
    shutil.copytree(f'./{args.version}', f'{args.prefix_gcc}/libgomp')
    shutil.copy(f'./{args.version}/boost.sh', f'{args.prefix_gcc_bin}/lib64')
    make_process = subprocess.Popen(f'make -C {args.prefix_gcc}  -j {args.num_threads} && make install -C {args.prefix_gcc}' , stderr=subprocess.STDOUT)
    if make_process.wait() != 0:
        print("Ops, error while compiling GCC!")

def run_tbft_tests(args):
    print("Not implemented yet")

def set_environment_variables(args):
    with open("~/.bashrc", "r") as file:
        lines = file.readlines()
    with open("~/.bashrc", "w") as file:
        for line in lines:
            if "LD_LIBRARY_PATH" not in line.strip("\n"):
                file.write(line)
        else:
            file.write(f'export LD_LIBRARY_PATH={args.prefix_gcc_bin}/lib64')
    os.system("exec bash")


def config_tbft(args):
    print("Not implemented yet")

def main():
    args = create_parser()
    args.func(args)


if __name__ == "__main__":
    main()