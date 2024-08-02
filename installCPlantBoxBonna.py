#!/usr/bin/env python3

"""
One click install script for CPlantBox
adapted from installCPlantBox.py and simplace 
m.vianna@fz-juelich.de
"""
import os
import sys
import shutil
import subprocess


def show_message(message):
    print("*" * 120) 
    print(message)
    print("*" * 120)




show_message("This is a quick-install script for BonnaCluster: https://www.hpc.uni-bonn.de/en/systems/bonna")

#################################################################
#################################################################
## (1/3) Check some prerequistes
#################################################################
#################################################################

programs = ['wget', 'git', 'gcc', 'g++', 'cmake', 'pkg-config', 'gfortran', 'pip']#,'clang','python3'] 
show_message("(1/3) Checking linux prerequistes: " + " ".join(programs) + "...")

if not (sys.version_info.major == 3 and sys.version_info.minor == 8 and sys.version_info.micro == 2):
    print("\nYour Python version is not compatible other dependencies in the cluster, before running this installation, load Python 3.8.2 module:\n"+
          "module load Python/3.8.2-GCCcore-9.3.0\n")

# check some prerequistes 
if (sys.version_info.major < 3) or (sys.version_info.minor < 7):
    print("detected python version:",sys.version_info)
    raise Exception("update python to at least version 3.7")
    
error = []
for program in programs:
    try:
        output2 = subprocess.run([program, "--version"], capture_output=True)
    except FileNotFoundError:
        error.append(program)
        
if len(error) > 0:
    print("Program(s) {0} has/have not been found. try running sudo apt-get install {0}".format(" ".join(error)))
    raise Exception('import modules')

# check some pkgs prerequistes
modules = ['numpy', 'scipy', 'matplotlib', 'vtk', 'mpi4py',  'pandas', 'pybind11[global]', 'ipython'] 
show_message("(2/3) Checking python prerequistes: " + " ".join(modules) + "...")

for mymodule in modules:
    subprocess.run(["pip3", "install", "--user", mymodule]) 
      
show_message("(2/3) Step completed. All prerequistes found.")

#################################################################
#################################################################
## (3/3) Clone modules
#################################################################
#################################################################

# Adapted CPlantBox fork in stable master version for BonnaCluster
GitRepo = "https://github.com/Murilodsv/CPlantBox.git"
if not os.path.exists("CPlantBox"):
    subprocess.run(['git', 'clone', '--depth','1','-b', 'master', GitRepo])
    os.chdir("CPlantBox")
else:
    os.chdir("CPlantBox")
    RemoteURL = subprocess.run(["git", "config","--get","remote.origin.url", "."], capture_output=True)
    if not (GitRepo in str(RemoteURL.stdout)):
        raise Exception('The local CPlantBox repository has a different remote origin:\n'+'local: '+str(RemoteURL.stdout.decode())+'desired: '+GitRepo)
    print("-- Skip cloning CPlantBox because the folder already exists.")

# fetch submodules
subprocess.run(['git', 'submodule', 'update',  '--recursive', '--init'])

# check if loaded programs are in correct versions for bonna
bonnamod  = ['VTK/8.2.0-foss-2020a-Python-3.8.2','CMake/3.16.4-GCCcore-9.3.0']
modules   = ['vtkpython','cmake']
mod_ver   = ['3.8.2', '3.16.4']
for i in range(len(modules)):
    try:
        outmod = subprocess.run([modules[i], '--version'], capture_output=True)
        if not (mod_ver[i] in str(outmod.stdout)):
            error.append(bonnamod[i])
    except:
        error.append(bonnamod[i])

if len(error) > 0:
    print("Modules not properly loaded. Please try running 'module load <module_name>' for the following modules: "+" ".join(error))
    raise Exception('missing module')

# cmake
subprocess.run(['cmake', '.']) 
subprocess.run(['make'])  
os.chdir("..")


show_message("(3/3) Step completed. Succesfully configured and built CPlantBox.")

show_message("to test installation, run \n cd CPlantBox/tutorial/examples/ \n python3 example1a_small.py")

show_message("CPlantBox is currently at stable branch, use \n $git switch master \n to obtain the latest version, use cmake . & make to recompile")

