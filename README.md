---

### This is CPlantBox with the installation script for BonnaHPC

This is a stabled forked version of CPlantBox/master Aug-2024 
- https://github.com/Plant-Root-Soil-Interactions-Modelling/CPlantBox/tree/1089fe0421e24e834f69720598b0789cc7079148

To install in **BonnaHPC** navigate to the desired installation folder (e.g. ```~/workspace```) and use the shell script below:

```bash
#!/bin/bash
wget https://raw.githubusercontent.com/Murilodsv/CPlantBox/master/installCPlantBoxBonna.py

module purge
module load Python/3.8.2-GCCcore-9.3.0
module load VTK/8.2.0-foss-2020a-Python-3.8.2
module load CMake/3.16.4-GCCcore-9.3.0

python3 installCPlantBoxBonna.py
```

Examples are provided in "CPlantBox/tutorial/examples/". For instance:
```bash
cd tutorial/examples/python
python3 example1a.py
```

The dependecies are listed in the requirements.txt file.
## Installation on the JSC agrocluster
Refer to the wiki:\
https://github.com/Plant-Root-Soil-Interactions-Modelling/CPlantBox/wiki/CPlantBox-on-the-J%C3%BClich-Supercomputer-cluster
# Folder sructure

`/modelparameter`		Plant parameter files\
`/src`			CPlantBox C++ codes\
`/test`   Python tests for all CPlantBox classes\
`/tutorial` 		learn to use CPlantBox\
`/experimental`		Specific applications (in sub-folders). contrary to scripts in `/tutorial`, might not be kept up to date

# Code documentation

Create the documentation by running doxygen in the folder 
$ doxygen doxy_config

The documentation will be located in the folder /doc. Compile doc/latex/refman.tex to generate the full doxygen documentation in doc/latex/refman.pdf.

Collaboration diagrams give an overview of the code in folder /docs.

# Examples
Simulation videos availabe in Youtube Channel https://www.youtube.com/channel/UCPK-pFfpK94jiamgwHxX32Q




